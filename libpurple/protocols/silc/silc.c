/*

  silcpurple.c

  Author: Pekka Riikonen <priikone@silcnet.org>

  Copyright (C) 2004 - 2007 Pekka Riikonen

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

*/

#include "internal.h"
#include <purple.h>

PURPLE_BEGIN_IGNORE_CAST_ALIGN
#include "silc.h"
PURPLE_END_IGNORE_CAST_ALIGN
#include "silcclient.h"
#include "silcpurple.h"
#include "wb.h"

extern SilcClientOperations ops;

static PurpleProtocol *my_protocol = NULL;
static GSList *cmds = NULL;

/* Error log message callback */

static SilcBool silcpurple_log_error(SilcLogType type, char *message,
				     void *context)
{
	silc_say(NULL, NULL, SILC_CLIENT_MESSAGE_ERROR, message);
	return TRUE;
}

static void
silcpurple_free(SilcPurple sg)
{
	g_return_if_fail(sg != NULL);

	g_cancellable_cancel(sg->cancellable);
	g_clear_object(&sg->cancellable);
	g_clear_object(&sg->sockconn);

	g_clear_pointer(&sg->client, silc_client_free);
	g_clear_pointer(&sg->sha1hash, silc_hash_free);
	g_clear_pointer(&sg->mimeass, silc_mime_assembler_free);

	g_clear_pointer(&sg->public_key, silc_pkcs_public_key_free);
	g_clear_pointer(&sg->private_key, silc_pkcs_private_key_free);

	silc_free(sg);
}

static const char *
silcpurple_list_icon(PurpleAccount *a, PurpleBuddy *b)
{
	return (const char *)"silc";
}

static GList *
silcpurple_away_states(PurpleAccount *account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, SILCPURPLE_STATUS_ID_AVAILABLE, NULL, TRUE, TRUE, FALSE);
	types = g_list_append(types, type);
	type = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, SILCPURPLE_STATUS_ID_HYPER, _("Hyper Active"), TRUE, TRUE, FALSE);
	types = g_list_append(types, type);
	type = purple_status_type_new_full(PURPLE_STATUS_AWAY, SILCPURPLE_STATUS_ID_AWAY, NULL, TRUE, TRUE, FALSE);
	types = g_list_append(types, type);
	type = purple_status_type_new_full(PURPLE_STATUS_UNAVAILABLE, SILCPURPLE_STATUS_ID_BUSY, _("Busy"), TRUE, TRUE, FALSE);
	types = g_list_append(types, type);
	type = purple_status_type_new_full(PURPLE_STATUS_AWAY, SILCPURPLE_STATUS_ID_INDISPOSED, _("Indisposed"), TRUE, TRUE, FALSE);
	types = g_list_append(types, type);
	type = purple_status_type_new_full(PURPLE_STATUS_AWAY, SILCPURPLE_STATUS_ID_PAGE, _("Wake Me Up"), TRUE, TRUE, FALSE);
	types = g_list_append(types, type);
	type = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, SILCPURPLE_STATUS_ID_OFFLINE, NULL, TRUE, TRUE, FALSE);
	types = g_list_append(types, type);

	return types;
}

static void
silcpurple_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc = purple_account_get_connection(account);
	SilcPurple sg = NULL;
	SilcUInt32 mode;
	SilcBuffer idp;
	unsigned char mb[4];
	const char *state;

	if (gc != NULL)
		sg = purple_connection_get_protocol_data(gc);

	if (status == NULL)
		return;

	state = purple_status_get_id(status);

	if (state == NULL)
		return;

	if ((sg == NULL) || (sg->conn == NULL))
		return;

	mode = sg->conn->local_entry->mode;
	mode &= ~(SILC_UMODE_GONE |
		  SILC_UMODE_HYPER |
		  SILC_UMODE_BUSY |
		  SILC_UMODE_INDISPOSED |
		  SILC_UMODE_PAGE);

	if (purple_strequal(state, "hyper"))
		mode |= SILC_UMODE_HYPER;
	else if (purple_strequal(state, "away"))
		mode |= SILC_UMODE_GONE;
	else if (purple_strequal(state, "busy"))
		mode |= SILC_UMODE_BUSY;
	else if (purple_strequal(state, "indisposed"))
		mode |= SILC_UMODE_INDISPOSED;
	else if (purple_strequal(state, "page"))
		mode |= SILC_UMODE_PAGE;

	/* Send UMODE */
	idp = silc_id_payload_encode(sg->conn->local_id, SILC_ID_CLIENT);
	SILC_PUT32_MSB(mode, mb);
	silc_client_command_send(sg->client, sg->conn, SILC_COMMAND_UMODE,
				 silcpurple_command_reply, NULL, 2,
				 1, idp->data, silc_buffer_len(idp),
				 2, mb, sizeof(mb));
	silc_buffer_free(idp);
}


/*************************** Connection Routines *****************************/

static void
silcpurple_keepalive(PurpleConnection *gc)
{
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	silc_packet_send(sg->conn->stream, SILC_PACKET_HEARTBEAT, 0,
			 NULL, 0);
}

typedef struct {
  SilcPurple sg;
  SilcUInt32 fd;
  guint tag;
} *SilcPurpleTask;

/* A timeout occurred.  Call SILC scheduler. */

static gboolean
silcpurple_scheduler_timeout(gpointer context)
{
	SilcPurpleTask task = (SilcPurpleTask)context;
	silc_client_run_one(task->sg->client);
	silc_dlist_del(task->sg->tasks, task);
	silc_free(task);
	return FALSE;
}

/* An fd task event occurred.  Call SILC scheduler. */

static void
silcpurple_scheduler_fd(gpointer data, gint fd, PurpleInputCondition cond)
{
	SilcClient client = (SilcClient)data;
	silc_client_run_one(client);
}

/* SILC Scheduler notify callback.  This is called whenever task is added to
   or deleted from SILC scheduler.  It's also called when fd task events
   change.  Here we add same tasks to glib's main loop. */

static void
silcpurple_scheduler(SilcSchedule schedule,
		     SilcBool added, SilcTask task,
		     SilcBool fd_task, SilcUInt32 fd,
		     SilcTaskEvent event,
		     long seconds, long useconds,
		     void *context)
{
	SilcClient client = (SilcClient)context;
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcPurpleTask ptask = NULL;

	if (added) {
	  if (fd_task) {
	    /* Add fd or change fd events */
	    PurpleInputCondition e = 0;

	    silc_dlist_start(sg->tasks);
	    while ((ptask = silc_dlist_get(sg->tasks)))
	      if (ptask->fd == fd) {
		purple_input_remove(ptask->tag);
		break;
	      }

	    if (event & SILC_TASK_READ)
	      e |= PURPLE_INPUT_READ;
	    if (event & SILC_TASK_WRITE)
	      e |= PURPLE_INPUT_WRITE;

	    if (e) {
	      if (!ptask) {
		ptask = silc_calloc(1, sizeof(*ptask));
		ptask->fd = fd;
		silc_dlist_add(sg->tasks, ptask);
	      }
	      ptask->tag = purple_input_add(fd, e, silcpurple_scheduler_fd,
					    client);
	    } else if (ptask) {
	      silc_dlist_del(sg->tasks, ptask);
	      silc_free(ptask);
	    }
	  } else {
	    /* Add timeout */
	    ptask = silc_calloc(1, sizeof(*ptask));
	    ptask->sg = sg;
	    ptask->tag = g_timeout_add((seconds * 1000) +
					    (useconds / 1000),
					    silcpurple_scheduler_timeout,
					    ptask);
	    silc_dlist_add(sg->tasks, ptask);
	  }
	} else {
	  if (fd_task) {
	    /* Remove fd */
	    silc_dlist_start(sg->tasks);
	    while ((ptask = silc_dlist_get(sg->tasks)))
	      if (ptask->fd == fd) {
		purple_input_remove(ptask->tag);
		silc_dlist_del(sg->tasks, ptask);
		silc_free(ptask);
		break;
	      }
	  }
	}
}

static void
silcpurple_connect_cb(SilcClient client, SilcClientConnection conn,
		      SilcClientConnectionStatus status, SilcStatus error,
		      const char *message, void *context)
{
	PurpleConnection *gc = context;
	SilcPurple sg;
	SilcUInt32 mask;
	char tz[16];
	PurpleImage *img;
#ifdef HAVE_SYS_UTSNAME_H
	struct utsname u;
#endif

	sg = purple_connection_get_protocol_data(gc);

	switch (status) {
	case SILC_CLIENT_CONN_SUCCESS:
	case SILC_CLIENT_CONN_SUCCESS_RESUME:
		sg->conn = conn;

		/* Connection created successfully */
		purple_connection_set_state(gc, PURPLE_CONNECTION_CONNECTED);

		/* Send the server our buddy list */
		silcpurple_send_buddylist(gc);

		g_unlink(silcpurple_session_file(purple_account_get_username(sg->account)));

		/* Send any UMODEs configured for account */
		if (purple_account_get_bool(sg->account, "block-ims", FALSE)) {
			silc_client_command_call(sg->client, sg->conn, NULL,
						 "UMODE", "+P", NULL);
		}

		/* Set default attributes */
		mask = SILC_ATTRIBUTE_MOOD_NORMAL;
		silc_client_attribute_add(client, conn,
					  SILC_ATTRIBUTE_STATUS_MOOD,
					  SILC_32_TO_PTR(mask),
					  sizeof(SilcUInt32));
		mask = SILC_ATTRIBUTE_CONTACT_CHAT;
		silc_client_attribute_add(client, conn,
					  SILC_ATTRIBUTE_PREFERRED_CONTACT,
					  SILC_32_TO_PTR(mask),
					  sizeof(SilcUInt32));
#ifdef HAVE_SYS_UTSNAME_H
		if (!uname(&u)) {
			SilcAttributeObjDevice dev;
			memset(&dev, 0, sizeof(dev));
			dev.type = SILC_ATTRIBUTE_DEVICE_COMPUTER;
			dev.version = u.release;
			dev.model = u.sysname;
			silc_client_attribute_add(client, conn,
						  SILC_ATTRIBUTE_DEVICE_INFO,
						  (void *)&dev, sizeof(dev));
		}
#endif
		silc_timezone(tz, sizeof(tz));
		silc_client_attribute_add(client, conn,
					  SILC_ATTRIBUTE_TIMEZONE,
					  (void *)tz, strlen(tz));

		/* Set our buddy icon */
		img = purple_buddy_icons_find_account_icon(sg->account);
		silcpurple_buddy_set_icon(gc, img);
		g_object_unref(img);

		return;
		break;

	case SILC_CLIENT_CONN_DISCONNECTED:
		/* Disconnected */
		if (sg->resuming && !sg->detaching)
		  g_unlink(silcpurple_session_file(purple_account_get_username(sg->account)));

		/* Close the connection */
		if (!sg->detaching)
		  purple_connection_error(gc,
		                                 PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
		                                 _("Disconnected by server"));
		else
		  /* TODO: Does this work correctly? Maybe we need to set wants_to_die? */
		  purple_account_disconnect(purple_connection_get_account(gc));
		break;

	case SILC_CLIENT_CONN_ERROR:
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
		                             _("Error connecting to SILC Server"));
		g_unlink(silcpurple_session_file(purple_account_get_username(sg->account)));
		break;

	case SILC_CLIENT_CONN_ERROR_KE:
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_ENCRYPTION_ERROR,
		                             _("Key Exchange failed"));
		break;

	case SILC_CLIENT_CONN_ERROR_AUTH:
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
		                             _("Authentication failed"));
		break;

	case SILC_CLIENT_CONN_ERROR_RESUME:
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                             _("Resuming detached session failed. "
		                               "Press Reconnect to create new connection."));
		g_unlink(silcpurple_session_file(purple_account_get_username(sg->account)));
		break;

	case SILC_CLIENT_CONN_ERROR_TIMEOUT:
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
		                             _("Connection timed out"));
		break;
	}

	/* Error */
	sg->conn = NULL;
}

static void
silcpurple_stream_created(SilcSocketStreamStatus status, SilcStream stream,
			  void *context)
{
	PurpleConnection *gc = context;
	SilcPurple sg;
	SilcClient client;
	SilcClientConnectionParams params;
	const char *dfile;

	sg = purple_connection_get_protocol_data(gc);

	if (status != SILC_SOCKET_OK) {
		purple_connection_error(gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			_("Connection failed"));
		silcpurple_free(sg);
		purple_connection_set_protocol_data(gc, NULL);
		return;
	}

	client = sg->client;

	/* Get session detachment data, if available */
	memset(&params, 0, sizeof(params));
	dfile = silcpurple_session_file(purple_account_get_username(sg->account));
	params.detach_data = (unsigned char *)silc_file_readfile(dfile, &params.detach_data_len);
	if (params.detach_data)
		params.detach_data[params.detach_data_len] = 0;
	params.ignore_requested_attributes = FALSE;
	params.pfs = purple_account_get_bool(sg->account, "pfs", FALSE);

	/* Progress */
	if (params.detach_data) {
		purple_connection_update_progress(gc, _("Resuming session"), 2, 5);
		sg->resuming = TRUE;
	} else {
		purple_connection_update_progress(gc, _("Performing key exchange"), 2, 5);
	}

	/* Perform SILC Key Exchange. */
	silc_client_key_exchange(client, &params, sg->public_key,
				 sg->private_key, stream, SILC_CONN_SERVER,
				 silcpurple_connect_cb, gc);

	silc_free(params.detach_data);
}

static void
silcpurple_login_connected(GObject *source, GAsyncResult *res, gpointer data)
{
	PurpleConnection *gc = data;
	SilcPurple sg;
	GSocketConnection *conn;
	GSocket *socket;
	gint fd;
	GError *error = NULL;

	g_return_if_fail(gc != NULL);

	sg = purple_connection_get_protocol_data(gc);

	conn = g_socket_client_connect_to_host_finish(G_SOCKET_CLIENT(source),
			res, &error);
	if (conn == NULL) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			purple_connection_error(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
						     _("Connection failed"));
			silcpurple_free(sg);
			purple_connection_set_protocol_data(gc, NULL);
		}

		g_clear_error(&error);
		return;
	}

	socket = g_socket_connection_get_socket(conn);
	g_assert(socket != NULL);

	fd = g_socket_get_fd(socket);
	sg->sockconn = conn;

	silc_hash_alloc((unsigned char *)"sha1", &sg->sha1hash);

	/* Wrap socket to TCP stream */
	silc_socket_tcp_stream_create(fd, TRUE, FALSE,
				      sg->client->schedule,
				      silcpurple_stream_created, gc);
}

static void silcpurple_continue_running(SilcPurple sg)
{
	PurpleConnection *gc = sg->gc;
	PurpleAccount *account = purple_connection_get_account(gc);
	GSocketClient *client;

	client = purple_gio_socket_client_new(account, &error);
	if (client == NULL) {
		/* Assume it's a proxy error */
		purple_notify_error(NULL, NULL, _("Invalid proxy settings"),
			error->message,
			purple_request_cpar_from_account(account));
		purple_connection_take_error(gc, error);
		purple_connection_set_protocol_data(gc, NULL);
		silcpurple_free(sg);
		return;
	}

	/* Connect to the SILC server */
	g_socket_client_connect_to_host_async(client,
				 purple_account_get_string(account, "server",
							   "silc.silcnet.org"),
				 purple_account_get_int(account, "port", 706),
				 sg->cancellable, silcpurple_login_connected, gc);
	g_object_unref(client);
}

static void silcpurple_got_password_cb(PurpleConnection *gc, PurpleRequestFields *fields)
{
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	PurpleAccount *account = purple_connection_get_account(gc);
	char pkd[256], prd[256];
	const char *password;
	gboolean remember;

	/* TODO: the password prompt dialog doesn't get disposed if the account disconnects */
	PURPLE_ASSERT_CONNECTION_IS_VALID(gc);

	password = purple_request_fields_get_string(fields, "password");
	remember = purple_request_fields_get_bool(fields, "remember");

	if (!password || !*password)
	{
		purple_notify_error(gc, NULL,
			_("Password is required to sign on."), NULL,
			purple_request_cpar_from_connection(gc));
		purple_connection_set_protocol_data(gc, NULL);
		silcpurple_free(sg);
		return;
	}

	if (remember)
		purple_account_set_remember_password(account, TRUE);

	purple_account_set_password(account, password, NULL, NULL);

	/* Load SILC key pair */
	g_snprintf(pkd, sizeof(pkd), "%s" G_DIR_SEPARATOR_S "public_key.pub", silcpurple_silcdir());
	g_snprintf(prd, sizeof(prd), "%s" G_DIR_SEPARATOR_S "private_key.prv", silcpurple_silcdir());
	if (!silc_load_key_pair((char *)purple_account_get_string(account, "public-key", pkd),
				(char *)purple_account_get_string(account, "private-key", prd),
				password,
				&sg->public_key, &sg->private_key)) {
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                             _("Unable to load SILC key pair"));
		purple_connection_set_protocol_data(gc, NULL);
		silcpurple_free(sg);
		return;
	}
	silcpurple_continue_running(sg);
}

static void silcpurple_no_password_cb(PurpleConnection *gc, PurpleRequestFields *fields)
{
	SilcPurple sg;

	/* TODO: the password prompt dialog doesn't get disposed if the account disconnects */
	PURPLE_ASSERT_CONNECTION_IS_VALID(gc);

	sg = purple_connection_get_protocol_data(gc);
	purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
			_("Unable to load SILC key pair"));
	purple_connection_set_protocol_data(gc, NULL);
	silcpurple_free(sg);
}

static void silcpurple_running(SilcClient client, void *context)
{
	SilcPurple sg = context;
	PurpleConnection *gc = sg->gc;
	PurpleAccount *account = purple_connection_get_account(gc);
	char pkd[256], prd[256];


	/* Progress */
	purple_connection_update_progress(gc, _("Connecting to SILC Server"), 1, 5);

	/* Load SILC key pair */
	g_snprintf(pkd, sizeof(pkd), "%s" G_DIR_SEPARATOR_S "public_key.pub", silcpurple_silcdir());
	g_snprintf(prd, sizeof(prd), "%s" G_DIR_SEPARATOR_S "private_key.prv", silcpurple_silcdir());
	if (!silc_load_key_pair((char *)purple_account_get_string(account, "public-key", pkd),
				(char *)purple_account_get_string(account, "private-key", prd),
				(purple_connection_get_password(gc) == NULL) ? "" : purple_connection_get_password(gc),
				&sg->public_key, &sg->private_key)) {
		if (!purple_connection_get_password(gc)) {
			purple_account_request_password(account, G_CALLBACK(silcpurple_got_password_cb),
											G_CALLBACK(silcpurple_no_password_cb), gc);
			return;
		}
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                             _("Unable to load SILC key pair"));
		purple_connection_set_protocol_data(gc, NULL);
		silcpurple_free(sg);
		return;
	}
	silcpurple_continue_running(sg);
}

static void
silcpurple_login(PurpleAccount *account)
{
	SilcClient client;
	PurpleConnection *gc;
	SilcPurple sg;
	SilcClientParams params;
	const char *cipher, *hmac;
	char *username, *hostname, *realname, **up;
	int i;

	gc = purple_account_get_connection(account);
	if (!gc)
		return;
	purple_connection_set_protocol_data(gc, NULL);

	memset(&params, 0, sizeof(params));
	strcat(params.nickname_format, "%n#a");

	/* Allocate SILC client */
	client = silc_client_alloc(&ops, &params, gc, NULL);
	if (!client) {
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                             _("Out of memory"));
		return;
	}

	/* Get username, real name and local hostname for SILC library */
	if (!purple_account_get_username(account))
		purple_account_set_username(account, silc_get_username());

	username = (char *)purple_account_get_username(account);
	up = g_strsplit(username, "@", 2);
	username = g_strdup(up[0]);
	g_strfreev(up);

	if (!purple_account_get_user_info(account)) {
		purple_account_set_user_info(account, silc_get_real_name());
		if (!purple_account_get_user_info(account))
			purple_account_set_user_info(account,
						     "John T. Noname");
	}
	realname = (char *)purple_account_get_user_info(account);
	hostname = silc_net_localhost();

	purple_connection_set_display_name(gc, username);

	/* Register requested cipher and HMAC */
	cipher = purple_account_get_string(account, "cipher",
					   SILC_DEFAULT_CIPHER);
	for (i = 0; silc_default_ciphers[i].name; i++)
		if (purple_strequal(silc_default_ciphers[i].name, cipher)) {
			silc_cipher_register(&(silc_default_ciphers[i]));
			break;
		}
	hmac = purple_account_get_string(account, "hmac", SILC_DEFAULT_HMAC);
	for (i = 0; silc_default_hmacs[i].name; i++)
		if (purple_strequal(silc_default_hmacs[i].name, hmac)) {
			silc_hmac_register(&(silc_default_hmacs[i]));
			break;
		}

	sg = silc_calloc(1, sizeof(*sg));
	if (!sg)
		return;
	sg->cancellable = g_cancellable_new();
	sg->client = client;
	sg->gc = gc;
	sg->account = account;
	purple_connection_set_protocol_data(gc, sg);

	/* Init SILC client */
	if (!silc_client_init(client, username, hostname, realname,
			      silcpurple_running, sg)) {
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                             _("Unable to initialize SILC protocol"));
		purple_connection_set_protocol_data(gc, NULL);
		silcpurple_free(sg);
		silc_free(hostname);
		g_free(username);
		return;
	}
	silc_free(hostname);
	g_free(username);

	/* Check the ~/.silc dir and create it, and new key pair if necessary. */
	if (!silcpurple_check_silc_dir(gc)) {
		purple_connection_error(gc, PURPLE_CONNECTION_ERROR_OTHER_ERROR,
		                             _("Error loading SILC key pair"));
		purple_connection_set_protocol_data(gc, NULL);
		silcpurple_free(sg);
		return;
	}

	/* Run SILC scheduler */
	sg->tasks = silc_dlist_init();
	silc_schedule_set_notify(client->schedule, silcpurple_scheduler,
				 client);
	silc_client_run_one(client);
}

static int
silcpurple_close_final(gpointer *context)
{
	SilcPurple sg = (SilcPurple)context;

	purple_debug_info("silc", "Finalizing SilcPurple %p\n", sg);

	silc_client_stop(sg->client, NULL, NULL);

	silcpurple_free(sg);
	return 0;
}

static void
silcpurple_close(PurpleConnection *gc)
{
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcPurpleTask task;
	GHashTable *ui_info;
	const char *ui_name = NULL, *ui_website = NULL;
	char *quit_msg;

	g_return_if_fail(sg != NULL);

	ui_info = purple_core_get_ui_info();

	if(ui_info) {
		ui_name = g_hash_table_lookup(ui_info, "name");
		ui_website = g_hash_table_lookup(ui_info, "website");
	}

	if(!ui_name || !ui_website) {
		ui_name = "Pidgin";
		ui_website = PURPLE_WEBSITE;
	}
	quit_msg = g_strdup_printf(_("Download %s: %s"),
							   ui_name, ui_website);

	/* Send QUIT */
	silc_client_command_call(sg->client, sg->conn, NULL,
				 "QUIT", quit_msg,
				 NULL);
	g_free(quit_msg);

	if (sg->conn)
		silc_client_close_connection(sg->client, sg->conn);

	if (sg->conn)
	  silc_client_run_one(sg->client);
	silc_schedule_set_notify(sg->client->schedule, NULL, NULL);

	silc_dlist_start(sg->tasks);
	while ((task = silc_dlist_get(sg->tasks))) {
	  purple_input_remove(task->tag);
	  silc_free(task);
	}
	silc_dlist_uninit(sg->tasks);

	if (sg->scheduler)
		g_source_remove(sg->scheduler);

	purple_debug_info("silc", "Scheduling destruction of SilcPurple %p\n", sg);
	g_timeout_add(1, (GSourceFunc)silcpurple_close_final, sg);
}


/****************************** Protocol Actions *****************************/

static void
silcpurple_attrs_cancel(PurpleConnection *gc, PurpleRequestFields *fields)
{
	/* Nothing */
}

static void
silcpurple_attrs_cb(PurpleConnection *gc, PurpleRequestFields *fields)
{
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcClient client = sg->client;
	SilcClientConnection conn = sg->conn;
	PurpleRequestField *f;
	char *tmp;
	SilcUInt32 tmp_len, mask;
	SilcAttributeObjService service;
	SilcAttributeObjDevice dev;
	SilcVCardStruct vcard;
	const char *val;

	sg = purple_connection_get_protocol_data(gc);
	if (!sg)
		return;

	memset(&service, 0, sizeof(service));
	memset(&dev, 0, sizeof(dev));
	memset(&vcard, 0, sizeof(vcard));

	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_USER_INFO, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_SERVICE, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_STATUS_MOOD, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_STATUS_FREETEXT, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_STATUS_MESSAGE, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_PREFERRED_LANGUAGE, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_PREFERRED_CONTACT, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_TIMEZONE, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_GEOLOCATION, NULL);
	silc_client_attribute_del(client, conn,
				  SILC_ATTRIBUTE_DEVICE_INFO, NULL);

	/* Set mood */
	mask = 0;
	f = purple_request_fields_get_field(fields, "mood_normal");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_NORMAL;
	f = purple_request_fields_get_field(fields, "mood_happy");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_HAPPY;
	f = purple_request_fields_get_field(fields, "mood_sad");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_SAD;
	f = purple_request_fields_get_field(fields, "mood_angry");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_ANGRY;
	f = purple_request_fields_get_field(fields, "mood_jealous");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_JEALOUS;
	f = purple_request_fields_get_field(fields, "mood_ashamed");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_ASHAMED;
	f = purple_request_fields_get_field(fields, "mood_invincible");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_INVINCIBLE;
	f = purple_request_fields_get_field(fields, "mood_inlove");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_INLOVE;
	f = purple_request_fields_get_field(fields, "mood_sleepy");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_SLEEPY;
	f = purple_request_fields_get_field(fields, "mood_bored");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_BORED;
	f = purple_request_fields_get_field(fields, "mood_excited");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_EXCITED;
	f = purple_request_fields_get_field(fields, "mood_anxious");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_MOOD_ANXIOUS;
	silc_client_attribute_add(client, conn,
				  SILC_ATTRIBUTE_STATUS_MOOD,
				  SILC_32_TO_PTR(mask),
				  sizeof(SilcUInt32));

	/* Set preferred contact */
	mask = 0;
	f = purple_request_fields_get_field(fields, "contact_chat");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_CONTACT_CHAT;
	f = purple_request_fields_get_field(fields, "contact_email");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_CONTACT_EMAIL;
	f = purple_request_fields_get_field(fields, "contact_call");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_CONTACT_CALL;
	f = purple_request_fields_get_field(fields, "contact_sms");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_CONTACT_SMS;
	f = purple_request_fields_get_field(fields, "contact_mms");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_CONTACT_MMS;
	f = purple_request_fields_get_field(fields, "contact_video");
	if (f && purple_request_field_bool_get_value(f))
		mask |= SILC_ATTRIBUTE_CONTACT_VIDEO;
	if (mask)
		silc_client_attribute_add(client, conn,
					  SILC_ATTRIBUTE_PREFERRED_CONTACT,
					  SILC_32_TO_PTR(mask),
					  sizeof(SilcUInt32));

	/* Set status text */
	val = NULL;
	f = purple_request_fields_get_field(fields, "status_text");
	if (f)
		val = purple_request_field_string_get_value(f);
	if (val && *val)
		silc_client_attribute_add(client, conn,
					  SILC_ATTRIBUTE_STATUS_FREETEXT,
					  (void *)val, strlen(val));

	/* Set vcard */
	val = NULL;
	f = purple_request_fields_get_field(fields, "vcard");
	if (f)
		val = purple_request_field_string_get_value(f);
	if (val && *val) {
		purple_account_set_string(sg->account, "vcard", val);
		tmp = silc_file_readfile(val, &tmp_len);
		if (tmp) {
			tmp[tmp_len] = 0;
			if (silc_vcard_decode((unsigned char *)tmp, tmp_len, &vcard))
				silc_client_attribute_add(client, conn,
							  SILC_ATTRIBUTE_USER_INFO,
							  (void *)&vcard,
							  sizeof(vcard));
		}
		silc_vcard_free(&vcard);
		silc_free(tmp);
	} else {
		purple_account_set_string(sg->account, "vcard", "");
	}

#ifdef HAVE_SYS_UTSNAME_H
	/* Set device info */
	f = purple_request_fields_get_field(fields, "device");
	if (f && purple_request_field_bool_get_value(f)) {
		struct utsname u;
		if (!uname(&u)) {
			dev.type = SILC_ATTRIBUTE_DEVICE_COMPUTER;
			dev.version = u.release;
			dev.model = u.sysname;
			silc_client_attribute_add(client, conn,
						  SILC_ATTRIBUTE_DEVICE_INFO,
						  (void *)&dev, sizeof(dev));
		}
	}
#endif

	/* Set timezone */
	val = NULL;
	f = purple_request_fields_get_field(fields, "timezone");
	if (f)
		val = purple_request_field_string_get_value(f);
	if (val && *val)
		silc_client_attribute_add(client, conn,
					  SILC_ATTRIBUTE_TIMEZONE,
					  (void *)val, strlen(val));
}

static void
silcpurple_attrs(PurpleProtocolAction *action)
{
	PurpleConnection *gc = action->connection;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcClient client = sg->client;
	SilcClientConnection conn = sg->conn;
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *g;
	PurpleRequestField *f;
	SilcHashTable attrs;
	SilcAttributePayload attr;
	gboolean mnormal = TRUE, mhappy = FALSE, msad = FALSE,
		mangry = FALSE, mjealous = FALSE, mashamed = FALSE,
		minvincible = FALSE, minlove = FALSE, msleepy = FALSE,
		mbored = FALSE, mexcited = FALSE, manxious = FALSE;
	gboolean cemail = FALSE, ccall = FALSE, csms = FALSE,
		cmms = FALSE, cchat = TRUE, cvideo = FALSE;
#ifdef HAVE_SYS_UTSNAME_H
	gboolean device = TRUE;
#endif
	char status[1024], tz[16];

	sg = purple_connection_get_protocol_data(gc);
	if (!sg)
		return;

	memset(status, 0, sizeof(status));

	attrs = silc_client_attributes_get(client, conn);
	if (attrs) {
		if (silc_hash_table_find(attrs,
					 SILC_32_TO_PTR(SILC_ATTRIBUTE_STATUS_MOOD),
					 NULL, (void *)&attr)) {
			SilcUInt32 mood = 0;
			silc_attribute_get_object(attr, &mood, sizeof(mood));
			mnormal = !mood;
			mhappy = (mood & SILC_ATTRIBUTE_MOOD_HAPPY);
			msad = (mood & SILC_ATTRIBUTE_MOOD_SAD);
			mangry = (mood & SILC_ATTRIBUTE_MOOD_ANGRY);
			mjealous = (mood & SILC_ATTRIBUTE_MOOD_JEALOUS);
			mashamed = (mood & SILC_ATTRIBUTE_MOOD_ASHAMED);
			minvincible = (mood & SILC_ATTRIBUTE_MOOD_INVINCIBLE);
			minlove = (mood & SILC_ATTRIBUTE_MOOD_INLOVE);
			msleepy = (mood & SILC_ATTRIBUTE_MOOD_SLEEPY);
			mbored = (mood & SILC_ATTRIBUTE_MOOD_BORED);
			mexcited = (mood & SILC_ATTRIBUTE_MOOD_EXCITED);
			manxious = (mood & SILC_ATTRIBUTE_MOOD_ANXIOUS);
		}

		if (silc_hash_table_find(attrs,
					 SILC_32_TO_PTR(SILC_ATTRIBUTE_PREFERRED_CONTACT),
					 NULL, (void *)&attr)) {
			SilcUInt32 contact = 0;
			silc_attribute_get_object(attr, &contact, sizeof(contact));
			cemail = (contact & SILC_ATTRIBUTE_CONTACT_EMAIL);
			ccall = (contact & SILC_ATTRIBUTE_CONTACT_CALL);
			csms = (contact & SILC_ATTRIBUTE_CONTACT_SMS);
			cmms = (contact & SILC_ATTRIBUTE_CONTACT_MMS);
			cchat = (contact & SILC_ATTRIBUTE_CONTACT_CHAT);
			cvideo = (contact & SILC_ATTRIBUTE_CONTACT_VIDEO);
		}

		if (silc_hash_table_find(attrs,
					 SILC_32_TO_PTR(SILC_ATTRIBUTE_STATUS_FREETEXT),
					 NULL, (void *)&attr))
			silc_attribute_get_object(attr, &status, sizeof(status));

#ifdef HAVE_SYS_UTSNAME_H
		if (!silc_hash_table_find(attrs,
					  SILC_32_TO_PTR(SILC_ATTRIBUTE_DEVICE_INFO),
					  NULL, (void *)&attr))
			device = FALSE;
#endif
	}

	fields = purple_request_fields_new();

	g = purple_request_field_group_new(NULL);
	f = purple_request_field_label_new("l3", _("Your Current Mood"));
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_normal", _("Normal"), mnormal);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_happy", _("Happy"), mhappy);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_sad", _("Sad"), msad);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_angry", _("Angry"), mangry);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_jealous", _("Jealous"), mjealous);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_ashamed", _("Ashamed"), mashamed);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_invincible", _("Invincible"), minvincible);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_inlove", _("In love"), minlove);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_sleepy", _("Sleepy"), msleepy);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_bored", _("Bored"), mbored);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_excited", _("Excited"), mexcited);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("mood_anxious", _("Anxious"), manxious);
	purple_request_field_group_add_field(g, f);

	f = purple_request_field_label_new("l4", _("\nYour Preferred Contact Methods"));
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("contact_chat", _("Chat"), cchat);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("contact_email", _("Email"), cemail);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("contact_call", _("Phone"), ccall);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("contact_sms", _("SMS"), csms);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("contact_mms", _("MMS"), cmms);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_bool_new("contact_video", _("Video conferencing"), cvideo);
	purple_request_field_group_add_field(g, f);
	purple_request_fields_add_group(fields, g);

	g = purple_request_field_group_new(NULL);
	f = purple_request_field_string_new("status_text", _("Your Current Status"),
					  status[0] ? status : NULL, TRUE);
	purple_request_field_group_add_field(g, f);
	purple_request_fields_add_group(fields, g);

	g = purple_request_field_group_new(NULL);
#ifdef HAVE_SYS_UTSNAME_H
	f = purple_request_field_bool_new("device",
					_("Let others see what computer you are using"),
					device);
	purple_request_field_group_add_field(g, f);
#endif
	purple_request_fields_add_group(fields, g);

	g = purple_request_field_group_new(NULL);
	f = purple_request_field_string_new("vcard", _("Your VCard File"),
					  purple_account_get_string(sg->account, "vcard", ""),
					  FALSE);
	purple_request_field_group_add_field(g, f);

	silc_timezone(tz, sizeof(tz));
	f = purple_request_field_string_new("timezone", _("Timezone (UTC)"), tz, FALSE);
	purple_request_field_group_add_field(g, f);
	purple_request_fields_add_group(fields, g);

	purple_request_fields(gc, _("User Online Status Attributes"),
			    _("User Online Status Attributes"),
			    _("You can let other users see your online status information "
			      "and your personal information. Please fill the information "
			      "you would like other users to see about yourself."),
			    fields,
			    _("OK"), G_CALLBACK(silcpurple_attrs_cb),
			    _("Cancel"), G_CALLBACK(silcpurple_attrs_cancel),
			    purple_request_cpar_from_connection(gc), gc);
}

static void
silcpurple_detach(PurpleProtocolAction *action)
{
	PurpleConnection *gc = action->connection;
	SilcPurple sg;

	if (!gc)
		return;
	sg = purple_connection_get_protocol_data(gc);
	if (!sg)
		return;

	/* Call DETACH */
	silc_client_command_call(sg->client, sg->conn, "DETACH");
	sg->detaching = TRUE;
}

static void
silcpurple_view_motd(PurpleProtocolAction *action)
{
	PurpleConnection *gc = action->connection;
	SilcPurple sg;
	char *tmp;

	if (!gc)
		return;
	sg = purple_connection_get_protocol_data(gc);
	if (!sg)
		return;

	if (!sg->motd) {
		purple_notify_error(gc, _("Message of the Day"), _("No Message "
			"of the Day available"), _("There is no Message of the "
			"Day associated with this connection"),
			purple_request_cpar_from_connection(gc));
		return;
	}

	tmp = g_markup_escape_text(sg->motd, -1);
	purple_notify_formatted(gc, NULL, _("Message of the Day"), NULL,
			      tmp, NULL, NULL);
	g_free(tmp);
}

static void
silcpurple_create_keypair_cancel(PurpleConnection *gc, PurpleRequestFields *fields)
{
	/* Nothing */
}

static void
silcpurple_create_keypair_cb(PurpleConnection *gc, PurpleRequestFields *fields)
{
	SilcPurple sg;
	PurpleRequestField *f;
	const char *val, *pkfile = NULL, *prfile = NULL;
	const char *pass1 = NULL, *pass2 = NULL, *un = NULL, *hn = NULL;
	const char *rn = NULL, *e = NULL, *o = NULL, *c = NULL;
	char *identifier;
	int keylen = SILCPURPLE_DEF_PKCS_LEN;
	SilcPublicKey public_key;

	sg = purple_connection_get_protocol_data(gc);
	if (!sg)
		return;

	val = NULL;
	f = purple_request_fields_get_field(fields, "pass1");
	if (f)
		val = purple_request_field_string_get_value(f);
	if (val && *val)
		pass1 = val;
	else
		pass1 = "";
	val = NULL;
	f = purple_request_fields_get_field(fields, "pass2");
	if (f)
		val = purple_request_field_string_get_value(f);
	if (val && *val)
		pass2 = val;
	else
		pass2 = "";

	if (!purple_strequal(pass1, pass2)) {
		purple_notify_error(gc, _("Create New SILC Key Pair"),
			_("Passphrases do not match"), NULL,
			purple_request_cpar_from_connection(gc));
		return;
	}

	val = NULL;
	f = purple_request_fields_get_field(fields, "key");
	if (f)
		val = purple_request_field_string_get_value(f);
	if (val && *val)
		keylen = atoi(val);
	f = purple_request_fields_get_field(fields, "pkfile");
	if (f)
		pkfile = purple_request_field_string_get_value(f);
	f = purple_request_fields_get_field(fields, "prfile");
	if (f)
		prfile = purple_request_field_string_get_value(f);

	f = purple_request_fields_get_field(fields, "un");
	if (f)
		un = purple_request_field_string_get_value(f);
	f = purple_request_fields_get_field(fields, "hn");
	if (f)
		hn = purple_request_field_string_get_value(f);
	f = purple_request_fields_get_field(fields, "rn");
	if (f)
		rn = purple_request_field_string_get_value(f);
	f = purple_request_fields_get_field(fields, "e");
	if (f)
		e = purple_request_field_string_get_value(f);
	f = purple_request_fields_get_field(fields, "o");
	if (f)
		o = purple_request_field_string_get_value(f);
	f = purple_request_fields_get_field(fields, "c");
	if (f)
		c = purple_request_field_string_get_value(f);

	identifier = silc_pkcs_silc_encode_identifier((char *)un, (char *)hn,
						      (char *)rn, (char *)e,
						      (char *)o, (char *)c,
						      NULL);

	/* Create the key pair */
	if (!silc_create_key_pair(SILCPURPLE_DEF_PKCS, keylen, pkfile, prfile,
				  identifier, pass1, &public_key, NULL,
				  FALSE)) {
		purple_notify_error(gc, _("Create New SILC Key Pair"),
			_("Key Pair Generation failed"), NULL,
			purple_request_cpar_from_connection(gc));
		return;
	}

	silcpurple_show_public_key(sg, NULL, public_key, NULL, NULL);

	silc_pkcs_public_key_free(public_key);
	silc_free(identifier);
}

static void
silcpurple_create_keypair(PurpleProtocolAction *action)
{
	PurpleConnection *gc = action->connection;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *g;
	PurpleRequestField *f;
	const char *username, *realname;
	char *hostname, **u;
	char tmp[256], pkd[256], pkd2[256], prd[256], prd2[256];

	username = purple_account_get_username(sg->account);
	u = g_strsplit(username, "@", 2);
	username = u[0];
	realname = purple_account_get_user_info(sg->account);
	hostname = silc_net_localhost();
	g_snprintf(tmp, sizeof(tmp), "%s@%s", username, hostname);

	g_snprintf(pkd2, sizeof(pkd2), "%s" G_DIR_SEPARATOR_S"public_key.pub", silcpurple_silcdir());
	g_snprintf(prd2, sizeof(prd2), "%s" G_DIR_SEPARATOR_S"private_key.prv", silcpurple_silcdir());
	g_snprintf(pkd, sizeof(pkd) - 1, "%s",
		   purple_account_get_string(purple_connection_get_account(gc), "public-key", pkd2));
	g_snprintf(prd, sizeof(prd) - 1, "%s",
		   purple_account_get_string(purple_connection_get_account(gc), "private-key", prd2));

	fields = purple_request_fields_new();

	g = purple_request_field_group_new(NULL);
	f = purple_request_field_string_new("key", _("Key length"), "2048", FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("pkfile", _("Public key file"), pkd, FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("prfile", _("Private key file"), prd, FALSE);
	purple_request_field_group_add_field(g, f);
	purple_request_fields_add_group(fields, g);

	g = purple_request_field_group_new(NULL);
	f = purple_request_field_string_new("un", _("Username"), username ? username : "", FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("hn", _("Hostname"), hostname ? hostname : "", FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("rn", _("Real name"), realname ? realname : "", FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("e", _("Email"), tmp, FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("o", _("Organization"), "", FALSE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("c", _("Country"), "", FALSE);
	purple_request_field_group_add_field(g, f);
	purple_request_fields_add_group(fields, g);

	g = purple_request_field_group_new(NULL);
	f = purple_request_field_string_new("pass1", _("Passphrase"), "", FALSE);
	purple_request_field_string_set_masked(f, TRUE);
	purple_request_field_group_add_field(g, f);
	f = purple_request_field_string_new("pass2", _("Passphrase (retype)"), "", FALSE);
	purple_request_field_string_set_masked(f, TRUE);
	purple_request_field_group_add_field(g, f);
	purple_request_fields_add_group(fields, g);

	purple_request_fields(gc, _("Create New SILC Key Pair"),
			      _("Create New SILC Key Pair"), NULL, fields,
			      _("Generate Key Pair"), G_CALLBACK(silcpurple_create_keypair_cb),
			      _("Cancel"), G_CALLBACK(silcpurple_create_keypair_cancel),
			      purple_request_cpar_from_connection(gc), gc);

	g_strfreev(u);
	silc_free(hostname);
}

static void
silcpurple_change_pass(PurpleProtocolAction *action)
{
	PurpleConnection *gc = action->connection;
	purple_account_request_change_password(purple_connection_get_account(gc));
}

static void
silcpurple_change_passwd(PurpleConnection *gc, const char *old, const char *new)
{
	char prd[256];
	g_snprintf(prd, sizeof(prd), "%s" G_DIR_SEPARATOR_S "private_key.pub", silcpurple_silcdir());
	silc_change_private_key_passphrase(purple_account_get_string(purple_connection_get_account(gc),
								     "private-key",
								     prd), old ? old : "", new ? new : "");
}

static void
silcpurple_show_set_info(PurpleProtocolAction *action)
{
	PurpleConnection *gc = action->connection;
	purple_account_request_change_user_info(purple_connection_get_account(gc));
}

static void
silcpurple_set_info(PurpleConnection *gc, const char *text)
{
}

static GList *
silcpurple_get_actions(PurpleConnection *gc)
{
	GList *list = NULL;
	PurpleProtocolAction *act;

	act = purple_protocol_action_new(_("Online Status"),
			silcpurple_attrs);
	list = g_list_append(list, act);

	act = purple_protocol_action_new(_("Detach From Server"),
			silcpurple_detach);
	list = g_list_append(list, act);

	act = purple_protocol_action_new(_("View Message of the Day"),
			silcpurple_view_motd);
	list = g_list_append(list, act);

	act = purple_protocol_action_new(_("Create SILC Key Pair..."),
			silcpurple_create_keypair);
	list = g_list_append(list, act);

	act = purple_protocol_action_new(_("Change Password..."),
			silcpurple_change_pass);
	list = g_list_append(list, act);

	act = purple_protocol_action_new(_("Set User Info..."),
			silcpurple_show_set_info);
	list = g_list_append(list, act);

	return list;
}


/******************************* IM Routines *********************************/

typedef struct {
	char *nick;
	char *message;
	SilcUInt32 message_len;
	SilcMessageFlags flags;
	PurpleMessageFlags gflags;
} *SilcPurpleIM;

static void
silcpurple_send_im_resolved(SilcClient client,
			    SilcClientConnection conn,
			    SilcStatus status,
			    SilcDList clients,
			    void *context)
{
	PurpleConnection *gc = client->application;
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcPurpleIM im = context;
	PurpleIMConversation *convo;
	char tmp[256];
	SilcClientEntry client_entry;
	SilcDList list;
	gboolean free_list = FALSE;

	convo = purple_conversations_find_im_with_account(im->nick,
						      sg->account);
	if (!convo)
		return;

	if (!clients)
		goto err;

	if (silc_dlist_count(clients) > 1) {
		/* Find the correct one. The im->nick might be a formatted nick
		   so this will find the correct one. */
		clients = silc_client_get_clients_local(client, conn,
							im->nick, FALSE);
		if (!clients)
			goto err;

		free_list = TRUE;
	}

	silc_dlist_start(clients);
	client_entry = silc_dlist_get(clients);

	/* Check for images */
	if (im->gflags & PURPLE_MESSAGE_IMAGES) {
		list = silcpurple_image_message(im->message, &im->flags);
		if (list) {
			/* Send one or more MIME message.  If more than one, they
			   are MIME fragments due to over large message */
			SilcBuffer buf;

			silc_dlist_start(list);
			while ((buf = silc_dlist_get(list)) != SILC_LIST_END)
				silc_client_send_private_message(client, conn,
								 client_entry, im->flags, sg->sha1hash,
								 buf->data,
								 silc_buffer_len(buf));
			silc_mime_partial_free(list);
			purple_conversation_write_message(PURPLE_CONVERSATION(convo),
				purple_message_new_outgoing(
					conn->local_entry->nickname, im->message, 0));
			goto out;
		}
	}

	/* Send the message */
	silc_client_send_private_message(client, conn, client_entry, im->flags,
					 sg->sha1hash, (unsigned char *)im->message, im->message_len);
	purple_conversation_write_message(PURPLE_CONVERSATION(convo),
		purple_message_new_outgoing(conn->local_entry->nickname, im->message, 0));
	goto out;

 err:
	g_snprintf(tmp, sizeof(tmp),
		   _("User <I>%s</I> is not present in the network"), im->nick);
	purple_conversation_write_system_message(
		PURPLE_CONVERSATION(convo), tmp, 0);

 out:
	if (free_list) {
		silc_client_list_free(client, conn, clients);
	}
	g_free(im->nick);
	g_free(im->message);
	silc_free(im);
}

static int
silcpurple_send_im(PurpleConnection *gc, PurpleMessage *pmsg)
{
	SilcPurple sg = purple_connection_get_protocol_data(gc);
	SilcClient client = sg->client;
	SilcClientConnection conn = sg->conn;
	SilcDList clients;
	SilcClientEntry client_entry;
	SilcMessageFlags mflags;
	char *msg, *tmp;
	int ret = 0;
	gboolean sign = purple_account_get_bool(sg->account, "sign-verify", FALSE);
	SilcDList list;
	const gchar *rcpt = purple_message_get_recipient(pmsg);
	const gchar *message = purple_message_get_contents(pmsg);
	PurpleMessageFlags flags = purple_message_get_flags(pmsg);

	if (!rcpt || purple_message_is_empty(pmsg))
		return 0;

	mflags = SILC_MESSAGE_FLAG_UTF8;

	tmp = msg = purple_unescape_html(message);

	if (!g_ascii_strncasecmp(msg, "/me ", 4)) {
		msg += 4;
		if (!*msg) {
			g_free(tmp);
			return 0;
		}
		mflags |= SILC_MESSAGE_FLAG_ACTION;
	} else if (strlen(msg) > 1 && msg[0] == '/') {
		if (!silc_client_command_call(client, conn, msg + 1))
			purple_notify_error(gc, _("Call Command"),
					    _("Cannot call command"),
					    _("Unknown command"),
					    purple_request_cpar_from_connection(gc));
		g_free(tmp);
		return 0;
	}

	if (sign)
		mflags |= SILC_MESSAGE_FLAG_SIGNED;

	/* Find client entry */
	clients = silc_client_get_clients_local(client, conn, rcpt, FALSE);
	if (!clients) {
		/* Resolve unknown user */
		SilcPurpleIM im = silc_calloc(1, sizeof(*im));
		if (!im) {
			g_free(tmp);
			return 0;
		}
		im->nick = g_strdup(rcpt);
		im->message = g_strdup(message);
		im->message_len = strlen(im->message);
		im->flags = mflags;
		im->gflags = flags;
		silc_client_get_clients(client, conn, rcpt, NULL,
					silcpurple_send_im_resolved, im);
		g_free(tmp);
		return 0;
	}
	silc_dlist_start(clients);
	client_entry = silc_dlist_get(clients);

	/* Check for images */
	if (flags & PURPLE_MESSAGE_IMAGES) {
		list = silcpurple_image_message(message, &mflags);
		if (list) {
			/* Send one or more MIME message.  If more than one, they
			   are MIME fragments due to over large message */
			SilcBuffer buf;

			silc_dlist_start(list);
			while ((buf = silc_dlist_get(list)) != SILC_LIST_END)
				ret =
			 	silc_client_send_private_message(client, conn,
								 client_entry, mflags, sg->sha1hash,
								 buf->data,
								 silc_buffer_len(buf));
			silc_mime_partial_free(list);
			g_free(tmp);
			silc_client_list_free(client, conn, clients);
			return ret;
		}
	}

	/* Send private message directly */
	ret = silc_client_send_private_message(client, conn, client_entry,
					       mflags, sg->sha1hash,
					       (unsigned char *)msg,
					       strlen(msg));

	g_free(tmp);
	silc_client_list_free(client, conn, clients);
	return ret;
}


static GList *silcpurple_blist_node_menu(PurpleBlistNode *node) {
	/* split this single menu building function back into the two
	   original: one for buddies and one for chats */
	if(PURPLE_IS_CHAT(node)) {
		return silcpurple_chat_menu((PurpleChat *) node);
	} else if(PURPLE_IS_BUDDY(node)) {
		return silcpurple_buddy_menu((PurpleBuddy *) node);
	} else {
		g_return_val_if_reached(NULL);
	}
}

/********************************* Commands **********************************/

static PurpleCmdRet silcpurple_cmd_chat_part(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	PurpleChatConversation *chat = PURPLE_CHAT_CONVERSATION(conv);
	int id = 0;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	if(args && args[0])
		chat = purple_conversations_find_chat_with_account(args[0],
									purple_connection_get_account(gc));

	if (chat != NULL)
		id = purple_chat_conversation_get_id(chat);

	if (id == 0)
		return PURPLE_CMD_RET_FAILED;

	silcpurple_chat_leave(gc, id);

	return PURPLE_CMD_RET_OK;

}

static PurpleCmdRet silcpurple_cmd_chat_topic(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	int id = 0;
	char *buf, *tmp, *tmp2;
	const char *topic;

	gc = purple_conversation_get_connection(conv);
	id = purple_chat_conversation_get_id(PURPLE_CHAT_CONVERSATION(conv));

	if (gc == NULL || id == 0)
		return PURPLE_CMD_RET_FAILED;

	if (!args || !args[0]) {
		topic = purple_chat_conversation_get_topic(PURPLE_CHAT_CONVERSATION(conv));
		if (topic) {
			tmp = g_markup_escape_text(topic, -1);
			tmp2 = purple_markup_linkify(tmp);
			buf = g_strdup_printf(_("current topic is: %s"), tmp2);
			g_free(tmp);
			g_free(tmp2);
		} else
			buf = g_strdup(_("No topic is set"));
		purple_conversation_write_system_message(conv,
			buf, PURPLE_MESSAGE_NO_LOG);
		g_free(buf);

	}

	if (args && args[0] && (strlen(args[0]) > 255)) {
		*error = g_strdup(_("Topic too long"));
		return PURPLE_CMD_RET_FAILED;
	}

	silcpurple_chat_set_topic(gc, id, args ? args[0] : NULL);

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_chat_join(PurpleConversation *conv,
        const char *cmd, char **args, char **error, void *data)
{
	GHashTable *comp;

	if(!args || !args[0])
		return PURPLE_CMD_RET_FAILED;

	comp = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

	g_hash_table_replace(comp, "channel", args[0]);
	if(args[1])
		g_hash_table_replace(comp, "passphrase", args[1]);

	silcpurple_chat_join(purple_conversation_get_connection(conv), comp);

	g_hash_table_destroy(comp);
	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_chat_list(PurpleConversation *conv,
        const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	gc = purple_conversation_get_connection(conv);
	purple_roomlist_show_with_account(purple_connection_get_account(gc));
	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_whois(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	silcpurple_get_info(gc, args[0]);

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_msg(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	int ret;
	PurpleConnection *gc;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	ret = silcpurple_send_im(gc,
		purple_message_new_outgoing(args[0], args[1], 0));

	if (ret)
		return PURPLE_CMD_RET_OK;
	else
		return PURPLE_CMD_RET_FAILED;
}

static PurpleCmdRet silcpurple_cmd_query(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	int ret = 1;
	PurpleIMConversation *im;
	PurpleConnection *gc;
	PurpleAccount *account;

	if (!args || !args[0]) {
		*error = g_strdup(_("You must specify a nick"));
		return PURPLE_CMD_RET_FAILED;
	}

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	account = purple_connection_get_account(gc);

	im = purple_im_conversation_new(account, args[0]);

	if (args[1]) {
		PurpleMessage *msg = purple_message_new_outgoing(
			args[0], args[1], 0);

		ret = silcpurple_send_im(gc, msg);
		purple_conversation_write_message(PURPLE_CONVERSATION(im), msg);
	}

	if (ret)
		return PURPLE_CMD_RET_OK;
	else
		return PURPLE_CMD_RET_FAILED;
}

static PurpleCmdRet silcpurple_cmd_motd(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	SilcPurple sg;
	char *tmp;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	sg = purple_connection_get_protocol_data(gc);

	if (sg == NULL)
		return PURPLE_CMD_RET_FAILED;

	if (!sg->motd) {
		*error = g_strdup(_("There is no Message of the Day associated with this connection"));
		return PURPLE_CMD_RET_FAILED;
	}

	tmp = g_markup_escape_text(sg->motd, -1);
	purple_notify_formatted(gc, NULL, _("Message of the Day"), NULL,
			tmp, NULL, NULL);
	g_free(tmp);

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_detach(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	SilcPurple sg;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	sg = purple_connection_get_protocol_data(gc);

	if (sg == NULL)
		return PURPLE_CMD_RET_FAILED;

	silc_client_command_call(sg->client, sg->conn, "DETACH");
	sg->detaching = TRUE;

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_cmode(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	SilcPurple sg;
	SilcChannelEntry channel;
	char *silccmd, *silcargs, *msg, tmp[256];
	const char *chname;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL || !args || purple_connection_get_protocol_data(gc) == NULL)
		return PURPLE_CMD_RET_FAILED;

	sg = purple_connection_get_protocol_data(gc);

	if (args[0])
		chname = args[0];
	else
		chname = purple_conversation_get_name(conv);

	if (!args[1]) {
		channel = silc_client_get_channel(sg->client, sg->conn,
										  (char *)chname);
		if (!channel) {
			*error = g_strdup_printf(_("channel %s not found"), chname);
			return PURPLE_CMD_RET_FAILED;
		}
		if (channel->mode) {
			silcpurple_get_chmode_string(channel->mode, tmp, sizeof(tmp));
			msg = g_strdup_printf(_("channel modes for %s: %s"), chname, tmp);
		} else {
			msg = g_strdup_printf(_("no channel modes are set on %s"), chname);
		}
		purple_conversation_write_system_message(conv, msg, PURPLE_MESSAGE_NO_LOG);
		g_free(msg);
		return PURPLE_CMD_RET_OK;
	}

	silcargs = g_strjoinv(" ", args);
	silccmd = g_strconcat(cmd, " ", silcargs, NULL);
	g_free(silcargs);
	if (!silc_client_command_call(sg->client, sg->conn, silccmd)) {
		g_free(silccmd);
		*error = g_strdup_printf(_("Failed to set cmodes for %s"), args[0]);
		return PURPLE_CMD_RET_FAILED;
	}
	g_free(silccmd);

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_generic(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	SilcPurple sg;
	char *silccmd, *silcargs;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	sg = purple_connection_get_protocol_data(gc);

	if (sg == NULL)
		return PURPLE_CMD_RET_FAILED;

	silcargs = g_strjoinv(" ", args);
	silccmd = g_strconcat(cmd, " ", args ? silcargs : NULL, NULL);
	g_free(silcargs);
	if (!silc_client_command_call(sg->client, sg->conn, silccmd)) {
		g_free(silccmd);
		*error = g_strdup_printf(_("Unknown command: %s, (may be a client bug)"), cmd);
		return PURPLE_CMD_RET_FAILED;
	}
	g_free(silccmd);

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_quit(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	SilcPurple sg;
	GHashTable *ui_info;
	const char *ui_name = NULL, *ui_website = NULL;
	char *quit_msg;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	sg = purple_connection_get_protocol_data(gc);

	if (sg == NULL)
		return PURPLE_CMD_RET_FAILED;

	ui_info = purple_core_get_ui_info();

	if(ui_info) {
		ui_name = g_hash_table_lookup(ui_info, "name");
		ui_website = g_hash_table_lookup(ui_info, "website");
	}

	if(!ui_name || !ui_website) {
		ui_name = "Pidgin";
		ui_website = PURPLE_WEBSITE;
	}
	quit_msg = g_strdup_printf(_("Download %s: %s"),
							   ui_name, ui_website);

	silc_client_command_call(sg->client, sg->conn, NULL,
				 "QUIT", (args && args[0]) ? args[0] : quit_msg, NULL);
	g_free(quit_msg);

	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet silcpurple_cmd_call(PurpleConversation *conv,
		const char *cmd, char **args, char **error, void *data)
{
	PurpleConnection *gc;
	SilcPurple sg;

	gc = purple_conversation_get_connection(conv);

	if (gc == NULL)
		return PURPLE_CMD_RET_FAILED;

	sg = purple_connection_get_protocol_data(gc);

	if (sg == NULL)
		return PURPLE_CMD_RET_FAILED;

	if (!silc_client_command_call(sg->client, sg->conn, args[0])) {
		*error = g_strdup_printf(_("Unknown command: %s"), args[0]);
		return PURPLE_CMD_RET_FAILED;
	}

	return PURPLE_CMD_RET_OK;
}


/************************** Plugin Initialization ****************************/

static void
silcpurple_register_commands(void)
{
	PurpleCmdId id;

	id = purple_cmd_register("part", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
			PURPLE_CMD_FLAG_PROTOCOL_ONLY | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
			"prpl-silc", silcpurple_cmd_chat_part, _("part [channel]:  Leave the chat"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("leave", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
			PURPLE_CMD_FLAG_PROTOCOL_ONLY | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
			"prpl-silc", silcpurple_cmd_chat_part, _("leave [channel]:  Leave the chat"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("topic", "s", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc",
			silcpurple_cmd_chat_topic, _("topic [&lt;new topic&gt;]:  View or change the topic"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("join", "ws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
			PURPLE_CMD_FLAG_PROTOCOL_ONLY | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
			"prpl-silc", silcpurple_cmd_chat_join,
			_("join &lt;channel&gt; [&lt;password&gt;]:  Join a chat on this network"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("list", "", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc",
			silcpurple_cmd_chat_list, _("list:  List channels on this network"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("whois", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc",
			silcpurple_cmd_whois, _("whois &lt;nick&gt;:  View nick's information"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("msg", "ws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_msg,
			_("msg &lt;nick&gt; &lt;message&gt;:  Send a private message to a user"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("query", "ws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_query,
			_("query &lt;nick&gt; [&lt;message&gt;]:  Send a private message to a user"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("motd", "", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_motd,
			_("motd:  View the server's Message Of The Day"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("detach", "", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_detach,
			_("detach:  Detach this session"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("quit", "s", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_quit,
			_("quit [message]:  Disconnect from the server, with an optional message"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("call", "s", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_call,
			_("call &lt;command&gt;:  Call any silc client command"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	/* These below just get passed through for the silc client library to deal
	 * with */
	id = purple_cmd_register("kill", "ws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("kill &lt;nick&gt; [-pubkey|&lt;reason&gt;]:  Kill nick"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("nick", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_generic,
			_("nick &lt;newnick&gt;:  Change your nickname"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("whowas", "ww", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("whowas &lt;nick&gt;:  View nick's information"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("cmode", "wws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_cmode,
			_("cmode &lt;channel&gt; [+|-&lt;modes&gt;] [arguments]:  Change or display channel modes"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("cumode", "wws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("cumode &lt;channel&gt; +|-&lt;modes&gt; &lt;nick&gt;:  Change nick's modes on channel"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("umode", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_generic,
			_("umode &lt;usermodes&gt;:  Set your modes in the network"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("oper", "s", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_generic,
			_("oper &lt;nick&gt; [-pubkey]:  Get server operator privileges"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("invite", "ws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("invite &lt;channel&gt; [-|+]&lt;nick&gt;:  invite nick or add/remove from channel invite list"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("kick", "wws", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("kick &lt;channel&gt; &lt;nick&gt; [comment]:  Kick client from channel"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("info", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("info [server]:  View server administrative details"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("ban", "ww", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY |
			PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, "prpl-silc", silcpurple_cmd_generic,
			_("ban [&lt;channel&gt; +|-&lt;nick&gt;]:  Ban client from channel"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("getkey", "w", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_generic,
			_("getkey &lt;nick|server&gt;:  Retrieve client's or server's public key"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("stats", "", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_generic,
			_("stats:  View server and network statistics"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));

	id = purple_cmd_register("ping", "", PURPLE_CMD_P_PROTOCOL,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PROTOCOL_ONLY,
			"prpl-silc", silcpurple_cmd_generic,
			_("ping:  Send PING to the connected server"), NULL);
	cmds = g_slist_prepend(cmds, GUINT_TO_POINTER(id));
}

static void
silcpurple_unregister_commands(void)
{
	while (cmds) {
		PurpleCmdId id = GPOINTER_TO_UINT(cmds->data);
		purple_cmd_unregister(id);
		cmds = g_slist_delete_link(cmds, cmds);
	}
}

static PurpleWhiteboardOps silcpurple_wb_ops =
{
	silcpurple_wb_start,
	silcpurple_wb_end,
	silcpurple_wb_get_dimensions,
	silcpurple_wb_set_dimensions,
	silcpurple_wb_get_brush,
	silcpurple_wb_set_brush,
	silcpurple_wb_send,
	silcpurple_wb_clear,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
silcpurple_protocol_init(SilcProtocol *self)
{
	PurpleProtocol *protocol = PURPLE_PROTOCOL(self);
	PurpleAccountOption *option;
	PurpleAccountUserSplit *split;
	char tmp[256];
	int i;
	PurpleKeyValuePair *kvp;
	GList *list = NULL;

	protocol->id        = "prpl-silc";
	protocol->name      = "SILC";
	protocol->options   = OPT_PROTO_CHAT_TOPIC | OPT_PROTO_UNIQUE_CHATNAME |
	                      OPT_PROTO_PASSWORD_OPTIONAL |
	                      OPT_PROTO_SLASH_COMMANDS_NATIVE;
	protocol->icon_spec = purple_buddy_icon_spec_new("jpeg,gif,png,bmp",
	                                                 0, 0, 96, 96, 0,
	                                                 PURPLE_ICON_SCALE_DISPLAY);

	protocol->whiteboard_ops = &silcpurple_wb_ops;

	split = purple_account_user_split_new(_("Network"), "silcnet.org", '@');
	protocol->user_splits = g_list_append(protocol->user_splits, split);

	/* Account options */
	option = purple_account_option_string_new(_("Connect server"),
						  "server",
						  "silc.silcnet.org");
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_int_new(_("Port"), "port", 706);
	protocol->account_options = g_list_append(protocol->account_options, option);
	g_snprintf(tmp, sizeof(tmp), "%s" G_DIR_SEPARATOR_S "public_key.pub", silcpurple_silcdir());
	option = purple_account_option_string_new(_("Public Key file"),
						  "public-key", tmp);
	protocol->account_options = g_list_append(protocol->account_options, option);
	g_snprintf(tmp, sizeof(tmp), "%s" G_DIR_SEPARATOR_S "private_key.prv", silcpurple_silcdir());
	option = purple_account_option_string_new(_("Private Key file"),
						  "private-key", tmp);
	protocol->account_options = g_list_append(protocol->account_options, option);

	for (i = 0; silc_default_ciphers[i].name; i++) {
	        kvp = purple_key_value_pair_new_full(silc_default_ciphers[i].name, g_strdup(silc_default_ciphers[i].name), g_free);
		list = g_list_append(list, kvp);
	}
	option = purple_account_option_list_new(_("Cipher"), "cipher", list);
	protocol->account_options = g_list_append(protocol->account_options, option);

	list = NULL;
	for (i = 0; silc_default_hmacs[i].name; i++) {
	        kvp = purple_key_value_pair_new_full(silc_default_hmacs[i].name, g_strdup(silc_default_hmacs[i].name), g_free);
		list = g_list_append(list, kvp);
	}
	option = purple_account_option_list_new(_("HMAC"), "hmac", list);
	protocol->account_options = g_list_append(protocol->account_options, option);

	option = purple_account_option_bool_new(_("Use Perfect Forward Secrecy"),
						"pfs", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);

	option = purple_account_option_bool_new(_("Public key authentication"),
						"pubkey-auth", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_bool_new(_("Block IMs without Key Exchange"),
						"block-ims", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_bool_new(_("Block messages to whiteboard"),
						"block-wb", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_bool_new(_("Automatically open whiteboard"),
						"open-wb", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_bool_new(_("Digitally sign and verify all messages"),
						"sign-verify", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
}

static void
silcpurple_protocol_class_init(SilcProtocolClass *klass)
{
	PurpleProtocolClass *protocol_class = PURPLE_PROTOCOL_CLASS(klass);

	protocol_class->login = silcpurple_login;
	protocol_class->close = silcpurple_close;
	protocol_class->status_types = silcpurple_away_states;
	protocol_class->list_icon = silcpurple_list_icon;
}

static void
silcpurple_protocol_class_finalize(G_GNUC_UNUSED SilcProtocolClass *klass)
{
}

static void
silcpurple_protocol_client_iface_init(PurpleProtocolClientInterface *client_iface)
{
	client_iface->get_actions     = silcpurple_get_actions;
	client_iface->status_text     = silcpurple_status_text;
	client_iface->tooltip_text    = silcpurple_tooltip_text;
	client_iface->blist_node_menu = silcpurple_blist_node_menu;
}

static void
silcpurple_protocol_server_iface_init(PurpleProtocolServerInterface *server_iface)
{
	server_iface->set_info       = silcpurple_set_info;
	server_iface->get_info       = silcpurple_get_info;
	server_iface->set_status     = silcpurple_set_status;
	server_iface->set_idle       = silcpurple_idle_set;
	server_iface->change_passwd  = silcpurple_change_passwd;
	server_iface->add_buddy      = silcpurple_add_buddy;
	server_iface->remove_buddy   = silcpurple_remove_buddy;
	server_iface->keepalive      = silcpurple_keepalive;
	server_iface->set_buddy_icon = silcpurple_buddy_set_icon;
}

static void
silcpurple_protocol_im_iface_init(PurpleProtocolIMInterface *im_iface)
{
	im_iface->send = silcpurple_send_im;
}

static void
silcpurple_protocol_chat_iface_init(PurpleProtocolChatInterface *chat_iface)
{
	chat_iface->info          = silcpurple_chat_info;
	chat_iface->info_defaults = silcpurple_chat_info_defaults;
	chat_iface->join          = silcpurple_chat_join;
	chat_iface->get_name      = silcpurple_get_chat_name;
	chat_iface->invite        = silcpurple_chat_invite;
	chat_iface->leave         = silcpurple_chat_leave;
	chat_iface->send          = silcpurple_chat_send;
	chat_iface->set_topic     = silcpurple_chat_set_topic;
}

static void
silcpurple_protocol_roomlist_iface_init(PurpleProtocolRoomlistInterface *roomlist_iface)
{
	roomlist_iface->get_list = silcpurple_roomlist_get_list;
	roomlist_iface->cancel   = silcpurple_roomlist_cancel;
}

static void
silcpurple_protocol_xfer_iface_init(PurpleProtocolXferInterface *xfer_iface)
{
	xfer_iface->send_file = silcpurple_ftp_send_file;
	xfer_iface->new_xfer  = silcpurple_ftp_new_xfer;
}

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
        SilcProtocol, silcpurple_protocol, PURPLE_TYPE_PROTOCOL, 0,

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_CLIENT,
                                      silcpurple_protocol_client_iface_init)

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_SERVER,
                                      silcpurple_protocol_server_iface_init)

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_IM,
                                      silcpurple_protocol_im_iface_init)

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_CHAT,
                                      silcpurple_protocol_chat_iface_init)

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_ROOMLIST,
                                      silcpurple_protocol_roomlist_iface_init)

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_XFER,
                                      silcpurple_protocol_xfer_iface_init));

static PurplePluginInfo *
plugin_query(GError **error)
{
	const gchar * const authors[] = {
		"Pekka Riikonen",
		NULL
	};

	return purple_plugin_info_new(
		"id",           "prpl-silc",
		"name",         "SILC Protocol",
		"version",      "1.1",
		"category",     N_("Protocol"),
		"summary",      N_("SILC Protocol Plugin"),
		"description",  N_("Secure Internet Live Conferencing (SILC) Protocol"),
		"authors",      authors,
		"website",      "http://silcnet.org/",
		"abi-version",  PURPLE_ABI_VERSION,
		"flags",        PURPLE_PLUGIN_INFO_FLAGS_INTERNAL |
		                PURPLE_PLUGIN_INFO_FLAGS_AUTO_LOAD,
		NULL
	);
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
	silcpurple_protocol_register_type(G_TYPE_MODULE(plugin));

	my_protocol = purple_protocols_add(SILCPURPLE_TYPE_PROTOCOL, error);
	if (!my_protocol)
		return FALSE;

	purple_prefs_remove("/plugins/prpl/silc");

	silc_log_set_callback(SILC_LOG_ERROR, silcpurple_log_error, NULL);
	silcpurple_register_commands();

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
	silcpurple_unregister_commands();

	if (!purple_protocols_remove(my_protocol, error))
		return FALSE;

	return TRUE;
}

PURPLE_PLUGIN_INIT(silc, plugin_query, plugin_load, plugin_unload);
