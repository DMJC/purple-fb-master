/**
 * purple
 *
 * Copyright (C) 2005 Thomas Butter <butter@uni-mannheim.de>
 *
 * ***
 * Thanks to Google's Summer of Code Program and the helpful mentors
 * ***
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "internal.h"
#include <purple.h>

#include "http.h"
#include "ntlm.h"
#include "simple.h"
#include "sipmsg.h"

static PurpleProtocol *my_protocol = NULL;

static gchar *gentag(void) {
	return g_strdup_printf("%04d%04d", g_random_int(), g_random_int());
}

static char *genbranch(void) {
	return g_strdup_printf("z9hG4bK%04X%04X%04X%04X%04X",
		g_random_int(), g_random_int(), g_random_int(),
		g_random_int(), g_random_int());
}

static char *gencallid(void) {
	return g_strdup_printf("%04Xg%04Xa%04Xi%04Xm%04Xt%04Xb%04Xx%04Xx",
		g_random_int(), g_random_int(), g_random_int(),
		g_random_int(), g_random_int(), g_random_int(),
		g_random_int(), g_random_int());
}

static const char *simple_list_icon(PurpleAccount *a, PurpleBuddy *b) {
	return "simple";
}

static gint
simple_uri_handler_find_account(PurpleAccount *account,
                                G_GNUC_UNUSED gconstpointer data)
{
	const gchar *protocol_id;

	protocol_id = purple_account_get_protocol_id(account);

	if (purple_strequal(protocol_id, "prpl-simple") &&
			purple_account_is_connected(account)) {
		return 0;
	} else {
		return -1;
	}
}

static gboolean
simple_uri_handler(const gchar *scheme, const gchar *screenname,
		GHashTable *params)
{
	GList *accounts;
	GList *account_node;
	PurpleIMConversation *im;

	g_return_val_if_fail(screenname != NULL, FALSE);

	if (!purple_strequal(scheme, "sip")) {
		return FALSE;
	}

	if (screenname[0] == '\0') {
		purple_debug_warning("simple",
				"Invalid empty screenname in URI");
		return FALSE;
	}

	/* Find online SIMPLE account */
	accounts = purple_accounts_get_all();
	account_node = g_list_find_custom(
	        accounts, NULL, (GCompareFunc)simple_uri_handler_find_account);

	if (account_node == NULL) {
		return FALSE;
	}

	im = purple_im_conversation_new(account_node->data, screenname);
	purple_conversation_present(PURPLE_CONVERSATION(im));

	return TRUE;
}

static void simple_keep_alive(PurpleConnection *gc) {
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	if(sip->udp) { /* in case of UDP send a packet only with a 0 byte to
			 remain in the NAT table */
		gchar buf[2] = {0, 0};
		purple_debug_info("simple", "sending keep alive\n");
		if (sendto(sip->fd, buf, 1, 0,
			(struct sockaddr*)&sip->serveraddr,
			sizeof(struct sockaddr_in)) != 1)
		{
			purple_debug_error("simple", "failed sending keep alive\n");
		}
	}
}

static gboolean process_register_response(struct simple_account_data *sip, struct sipmsg *msg, struct transaction *tc);
static void send_notify(struct simple_account_data *sip, struct simple_watcher *);

static void send_open_publish(struct simple_account_data *sip);
static void send_closed_publish(struct simple_account_data *sip);

static void do_notifies(struct simple_account_data *sip) {
	GSList *tmp = sip->watcher;
	purple_debug_info("simple", "do_notifies()\n");
	if((sip->republish != -1) || sip->republish < time(NULL)) {
		if(purple_account_get_bool(sip->account, "dopublish", TRUE)) {
			send_open_publish(sip);
		}
	}

	while(tmp) {
		purple_debug_info("simple", "notifying %s\n", ((struct simple_watcher*)tmp->data)->name);
		send_notify(sip, tmp->data);
		tmp = tmp->next;
	}
}

static void simple_set_status(PurpleAccount *account, PurpleStatus *status) {
	PurpleConnection *gc = purple_account_get_connection(account);
	PurpleStatusPrimitive primitive = purple_status_type_get_primitive(purple_status_get_status_type(status));
	struct simple_account_data *sip = NULL;

	if (!purple_status_is_active(status))
		return;

	if (gc)
		sip = purple_connection_get_protocol_data(gc);

	if (sip)
	{
		g_free(sip->status);
		if (primitive == PURPLE_STATUS_AVAILABLE)
			sip->status = g_strdup("available");
		else
			sip->status = g_strdup("busy");

		do_notifies(sip);
	}
}

static struct simple_watcher *watcher_find(struct simple_account_data *sip,
		const gchar *name) {
	struct simple_watcher *watcher;
	GSList *entry = sip->watcher;
	while(entry) {
		watcher = entry->data;
		if(purple_strequal(name, watcher->name)) return watcher;
		entry = entry->next;
	}
	return NULL;
}

static struct simple_watcher *watcher_create(struct simple_account_data *sip,
		const gchar *name, const gchar *callid, const gchar *ourtag,
		const gchar *theirtag, gboolean needsxpidf) {
	struct simple_watcher *watcher = g_new0(struct simple_watcher, 1);
	watcher->name = g_strdup(name);
	watcher->dialog.callid = g_strdup(callid);
	watcher->dialog.ourtag = g_strdup(ourtag);
	watcher->dialog.theirtag = g_strdup(theirtag);
	watcher->needsxpidf = needsxpidf;
	sip->watcher = g_slist_append(sip->watcher, watcher);
	return watcher;
}

static void
watcher_destroy(struct simple_watcher *watcher)
{
	g_free(watcher->name);
	g_free(watcher->dialog.callid);
	g_free(watcher->dialog.ourtag);
	g_free(watcher->dialog.theirtag);
	g_free(watcher);
}

static struct sip_connection *
connection_create(struct simple_account_data *sip, GSocketConnection *sockconn)
{
	struct sip_connection *ret = g_new0(struct sip_connection, 1);
	ret->sockconn = sockconn;
	sip->openconns = g_slist_append(sip->openconns, ret);
	return ret;
}

static void
connection_destroy(struct sip_connection *conn)
{
	if (conn->inputhandler) {
		g_source_remove(conn->inputhandler);
	}
	g_clear_pointer(&conn->inbuf, g_free);
	g_clear_object(&conn->sockconn);
	g_free(conn);
}

static struct sip_connection *
connection_find(struct simple_account_data *sip, GInputStream *input)
{
	struct sip_connection *ret = NULL;
	GSList *entry = sip->openconns;
	while (entry) {
		ret = entry->data;
		if (g_io_stream_get_input_stream(G_IO_STREAM(ret->sockconn)) == input) {
			return ret;
		}
		entry = entry->next;
	}
	return NULL;
}

static void
connection_remove(struct simple_account_data *sip, GInputStream *input)
{
	struct sip_connection *conn = connection_find(sip, input);
	sip->openconns = g_slist_remove(sip->openconns, conn);
	connection_destroy(conn);
}

static void simple_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group, const char *message)
{
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	struct simple_buddy *b;
	const char *name = purple_buddy_get_name(buddy);
	if(strncmp(name, "sip:", 4)) {
		gchar *buf = g_strdup_printf("sip:%s", name);
		purple_buddy_set_name(buddy, buf);
		g_free(buf);
	}
	if(!g_hash_table_lookup(sip->buddies, name)) {
		b = g_new0(struct simple_buddy, 1);
		purple_debug_info("simple", "simple_add_buddy %s\n", name);
		b->name = g_strdup(name);
		g_hash_table_insert(sip->buddies, b->name, b);
	} else {
		purple_debug_info("simple", "buddy %s already in internal list\n", name);
	}
}

static void simple_get_buddies(PurpleConnection *gc) {
	GSList *buddies;
	PurpleAccount *account;

	purple_debug_info("simple", "simple_get_buddies\n");

	account = purple_connection_get_account(gc);
	buddies = purple_blist_find_buddies(account, NULL);
	while (buddies) {
		PurpleBuddy *buddy = buddies->data;
		simple_add_buddy(gc, buddy, purple_buddy_get_group(buddy), NULL);

		buddies = g_slist_delete_link(buddies, buddies);
	}
}

static void simple_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	const char *name = purple_buddy_get_name(buddy);
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	struct simple_buddy *b = g_hash_table_lookup(sip->buddies, name);
	g_hash_table_remove(sip->buddies, name);
	g_free(b->name);
	g_free(b);
}

static GList *simple_status_types(PurpleAccount *acc) {
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new_with_attrs(
		PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE, TRUE, FALSE,
		"message", _("Message"), purple_value_new(G_TYPE_STRING),
		NULL);
	types = g_list_append(types, type);

	type = purple_status_type_new_full(
		PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE, TRUE, FALSE);
	types = g_list_append(types, type);

	return types;
}

static gchar *auth_header(struct simple_account_data *sip,
		struct sip_auth *auth, const gchar *method, const gchar *target) {
	gchar noncecount[9];
	gchar *response;
	gchar *ret;
	const char *authuser;

	authuser = purple_account_get_string(sip->account, "authuser", sip->username);

	if(!authuser || strlen(authuser) < 1) {
		authuser = sip->username;
	}

	if(auth->type == 1) { /* Digest */
		sprintf(noncecount, "%08d", auth->nc++);
		response = simple_http_digest_calculate_response(
		        method, target, auth->nonce, noncecount,
		        auth->digest_session_key);
		purple_debug(PURPLE_DEBUG_MISC, "simple", "response %s\n", response);

		ret = g_strdup_printf("Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", nc=\"%s\", response=\"%s\"", authuser, auth->realm, auth->nonce, target, noncecount, response);
		g_free(response);
		return ret;
	} else if(auth->type == 2) { /* NTLM */
#ifdef HAVE_NETTLE
		const gchar *authdomain;
		gchar *tmp;

		authdomain = purple_account_get_string(sip->account,
				"authdomain", "");

		if(auth->nc == 3 && auth->nonce) {
			/* TODO: Don't hardcode "purple" as the hostname */
			ret = purple_ntlm_gen_type3(authuser, sip->password, "purple", authdomain, (const guint8 *)auth->nonce, &auth->flags);
			tmp = g_strdup_printf("NTLM qop=\"auth\", opaque=\"%s\", realm=\"%s\", targetname=\"%s\", gssapi-data=\"%s\"", auth->opaque, auth->realm, auth->target, ret);
			g_free(ret);
			return tmp;
		}
		tmp = g_strdup_printf("NTLM qop=\"auth\", realm=\"%s\", targetname=\"%s\", gssapi-data=\"\"", auth->realm, auth->target);
		return tmp;
#else
		/* Used without support enabled */
		g_return_val_if_reached(NULL);
#endif /* HAVE_NETTLE */
	}

	sprintf(noncecount, "%08d", auth->nc++);
	response = simple_http_digest_calculate_response(
	        method, target, auth->nonce, noncecount,
	        auth->digest_session_key);
	purple_debug(PURPLE_DEBUG_MISC, "simple", "response %s\n", response);

	ret = g_strdup_printf("Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", nc=\"%s\", response=\"%s\"", authuser, auth->realm, auth->nonce, target, noncecount, response);
	g_free(response);
	return ret;
}

static char *parse_attribute(const char *attrname, const char *source) {
	const char *tmp, *tmp2;
	char *retval = NULL;
	int len = strlen(attrname);

	/* we know that source is NULL-terminated.
	 * Therefore this loop won't be infinite.
	 */
	while (source[0] == ' ')
 		source++;

	if(!strncmp(source, attrname, len)) {
		tmp = source + len;
		tmp2 = g_strstr_len(tmp, strlen(tmp), "\"");
		if(tmp2)
			retval = g_strndup(tmp, tmp2 - tmp);
		else
			retval = g_strdup(tmp);
	}

	return retval;
}

static void fill_auth(struct simple_account_data *sip, const gchar *hdr, struct sip_auth *auth) {
	int i = 0;
	const char *authuser;
	char *tmp;
	gchar **parts;

	authuser = purple_account_get_string(sip->account, "authuser", sip->username);

	if(!authuser || strlen(authuser) < 1) {
		authuser = sip->username;
	}

	if(!hdr) {
		purple_debug_error("simple", "fill_auth: hdr==NULL\n");
		return;
	}

	if(!g_ascii_strncasecmp(hdr, "NTLM", 4)) {
#ifdef HAVE_NETTLE
		purple_debug_info("simple", "found NTLM\n");
		auth->type = 2;
		parts = g_strsplit(hdr+5, "\",", 0);
		i = 0;
		while(parts[i]) {
			purple_debug_info("simple", "parts[i] %s\n", parts[i]);
			if((tmp = parse_attribute("gssapi-data=\"", parts[i]))) {
				auth->nonce = g_memdup(purple_ntlm_parse_type2(tmp, &auth->flags), 8);
				g_free(tmp);
			}
			if((tmp = parse_attribute("targetname=\"",
					parts[i]))) {
				auth->target = tmp;
			}
			else if((tmp = parse_attribute("realm=\"",
					parts[i]))) {
				auth->realm = tmp;
			}
			else if((tmp = parse_attribute("opaque=\"", parts[i]))) {
				auth->opaque = tmp;
			}
			i++;
		}
		g_strfreev(parts);
		auth->nc = 1;
		if(!strstr(hdr, "gssapi-data")) {
			auth->nc = 1;
		} else {
			auth->nc = 3;
		}

		return;
#else
		purple_debug_error("simple", "NTLM auth unsupported without "
				"libnettle support. Please rebuild with "
				"libnettle support for this feature.\n");
#endif /* HAVE_NETTLE */
	} else if(!g_ascii_strncasecmp(hdr, "DIGEST", 6)) {

		purple_debug_info("simple", "found DIGEST\n");

		auth->type = 1;
		parts = g_strsplit(hdr+7, ",", 0);
		while(parts[i]) {
			if((tmp = parse_attribute("nonce=\"", parts[i]))) {
				auth->nonce = tmp;
			}
			else if((tmp = parse_attribute("realm=\"", parts[i]))) {
				auth->realm = tmp;
			}
			i++;
		}
		g_strfreev(parts);
		purple_debug(PURPLE_DEBUG_MISC, "simple", "nonce: %s realm: %s\n",
					 auth->nonce ? auth->nonce : "(null)",
					 auth->realm ? auth->realm : "(null)");

		if(auth->realm) {
			auth->digest_session_key =
			        simple_http_digest_calculate_session_key(
			                authuser, auth->realm, sip->password,
			                auth->nonce);

			auth->nc = 1;
		}

	} else {
		purple_debug_error("simple", "Unsupported or bad WWW-Authenticate header (%s).\n", hdr);
	}

}

static void
simple_push_bytes_cb(GObject *sender, GAsyncResult *res, gpointer data)
{
	PurpleQueuedOutputStream *stream = PURPLE_QUEUED_OUTPUT_STREAM(sender);
	struct simple_account_data *sip = data;
	gboolean result;
	GError *error = NULL;

	result = purple_queued_output_stream_push_bytes_finish(stream, res, &error);

	if (!result) {
		purple_queued_output_stream_clear_queue(stream);

		if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free(error);
		} else {
			g_prefix_error(&error, "%s", _("Lost connection with server: "));
			purple_connection_take_error(sip->gc, error);
		}
	}
}

static gboolean simple_input_cb(GObject *stream, gpointer data);

static void
send_later_cb(GObject *sender, GAsyncResult *res, gpointer data)
{
	PurpleConnection *gc = data;
	struct simple_account_data *sip;
	struct sip_connection *conn;
	GSocketConnection *sockconn;
	gsize writelen;
	GSource *source;
	GError *error = NULL;

	sockconn = g_socket_client_connect_to_host_finish(G_SOCKET_CLIENT(sender),
	                                                  res, &error);
	if (sockconn == NULL) {
		if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_clear_error(&error);
		} else {
			purple_connection_take_error(gc, error);
		}
		return;
	}

	sip = purple_connection_get_protocol_data(gc);
	sip->input = g_io_stream_get_input_stream(G_IO_STREAM(sockconn));
	sip->output = purple_queued_output_stream_new(
	        g_io_stream_get_output_stream(G_IO_STREAM(sockconn)));
	sip->connecting = FALSE;

	writelen = purple_circular_buffer_get_max_read(sip->txbuf);
	if (writelen != 0) {
		const gchar *buf;
		GBytes *output;

		buf = purple_circular_buffer_get_output(sip->txbuf);

		output = g_bytes_new(buf, writelen);
		purple_queued_output_stream_push_bytes_async(
		        sip->output, output, G_PRIORITY_DEFAULT, sip->cancellable,
		        simple_push_bytes_cb, sip);
		g_bytes_unref(output);

		purple_circular_buffer_mark_read(sip->txbuf, writelen);
	}

	conn = connection_create(sip, sockconn);

	source = g_pollable_input_stream_create_source(
	        G_POLLABLE_INPUT_STREAM(sip->input), sip->cancellable);
	g_source_set_callback(source, (GSourceFunc)simple_input_cb, gc, NULL);
	conn->inputhandler = g_source_attach(source, NULL);
	g_source_unref(source);
}

static void sendlater(PurpleConnection *gc, const char *buf) {
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);

	if(!sip->connecting) {
		GSocketClient *client = NULL;
		GError *error = NULL;

		client = purple_gio_socket_client_new(sip->account, &error);
		if (client == NULL) {
			purple_connection_take_error(sip->gc, error);
			return;
		}

		purple_debug_info("simple", "connecting to %s port %d",
		                  sip->realhostname ? sip->realhostname : "{NULL}",
		                  sip->realport);
		g_socket_client_connect_to_host_async(client, sip->realhostname,
		                                      sip->realport, sip->cancellable,
		                                      send_later_cb, gc);
		g_object_unref(client);
		sip->connecting = TRUE;
	}

	if(purple_circular_buffer_get_max_read(sip->txbuf) > 0)
		purple_circular_buffer_append(sip->txbuf, "\r\n", 2);

	purple_circular_buffer_append(sip->txbuf, buf, strlen(buf));
}

static void sendout_pkt(PurpleConnection *gc, const char *buf) {
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	time_t currtime = time(NULL);
	int writelen = strlen(buf);

	purple_debug(PURPLE_DEBUG_MISC, "simple", "\n\nsending - %s\n######\n%s\n######\n\n", ctime(&currtime), buf);
	if(sip->udp) {
		if(sendto(sip->fd, buf, writelen, 0, (struct sockaddr*)&sip->serveraddr, sizeof(struct sockaddr_in)) < writelen) {
			purple_debug_info("simple", "could not send packet\n");
		}
	} else {
		GBytes *output;

		if (sip->output == NULL) {
			sendlater(gc, buf);
			return;
		}

		output = g_bytes_new(buf, writelen);
		purple_queued_output_stream_push_bytes_async(
		        sip->output, output, G_PRIORITY_DEFAULT, sip->cancellable,
		        simple_push_bytes_cb, sip);
		g_bytes_unref(output);
	}
}

static int simple_send_raw(PurpleConnection *gc, const char *buf, int len)
{
	sendout_pkt(gc, buf);
	return len;
}

static void
send_sip_response(PurpleConnection *gc, struct sipmsg *msg, gint code,
                  const gchar *text, const gchar *body)
{
	gchar *outstr;

	/* When sending the acknowlegements and errors, the content length from the original
	   message is still here, but there is no body; we need to make sure we're sending the
	   correct content length */
	sipmsg_remove_header(msg, "Content-Length");
	g_clear_pointer(&msg->body, g_free);
	if(body) {
		gchar len[12];
		msg->bodylen = strlen(body);
		sprintf(len, "%d", msg->bodylen);
		sipmsg_add_header(msg, "Content-Length", len);
		msg->body = g_strdup(body);
	} else {
		sipmsg_add_header(msg, "Content-Length", "0");
		msg->bodylen = 0;
	}
	msg->response = code;

	outstr = sipmsg_to_string(msg, text);
	sendout_pkt(gc, outstr);

	g_free(outstr);
}

static void
transactions_destroy(struct transaction *trans)
{
	if (trans->msg) {
		sipmsg_free(trans->msg);
	}
	g_free(trans);
}

static void transactions_add_buf(struct simple_account_data *sip, const gchar *buf, void *callback) {
	struct transaction *trans = g_new0(struct transaction, 1);
	trans->time = time(NULL);
	trans->msg = sipmsg_parse_msg(buf);
	trans->cseq = sipmsg_find_header(trans->msg, "CSeq");
	trans->callback = callback;
	sip->transactions = g_slist_append(sip->transactions, trans);
}

static struct transaction *transactions_find(struct simple_account_data *sip, struct sipmsg *msg) {
	struct transaction *trans;
	GSList *transactions = sip->transactions;
	const gchar *cseq = sipmsg_find_header(msg, "CSeq");

	if (cseq) {
		while(transactions) {
			trans = transactions->data;
			if(purple_strequal(trans->cseq, cseq)) {
				return trans;
			}
			transactions = transactions->next;
		}
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "simple", "Received message contains no CSeq header.\n");
	}

	return NULL;
}

static void send_sip_request(PurpleConnection *gc, const gchar *method,
		const gchar *url, const gchar *to, const gchar *addheaders,
		const gchar *body, struct sip_dialog *dialog, TransCallback tc) {
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	char *callid = dialog ? g_strdup(dialog->callid) : gencallid();
	char *auth = NULL;
	const char *addh = "";
	gchar *branch = genbranch();
	gchar *tag = NULL;
	char *buf;

	if(purple_strequal(method, "REGISTER")) {
		if(sip->regcallid) {
			g_free(callid);
			callid = g_strdup(sip->regcallid);
		}
		else sip->regcallid = g_strdup(callid);
	}

	if(addheaders) addh = addheaders;
	if(sip->registrar.type && purple_strequal(method, "REGISTER")) {
		buf = auth_header(sip, &sip->registrar, method, url);
		auth = g_strdup_printf("Authorization: %s\r\n", buf);
		g_free(buf);
		purple_debug(PURPLE_DEBUG_MISC, "simple", "header %s", auth);
	} else if(sip->proxy.type && !purple_strequal(method, "REGISTER")) {
		buf = auth_header(sip, &sip->proxy, method, url);
		auth = g_strdup_printf("Proxy-Authorization: %s\r\n", buf);
		g_free(buf);
		purple_debug(PURPLE_DEBUG_MISC, "simple", "header %s", auth);
	}

	if (!dialog)
		tag = gentag();

	buf = g_strdup_printf("%s %s SIP/2.0\r\n"
			"Via: SIP/2.0/%s %s:%d;branch=%s\r\n"
			/* Don't know what epid is, but LCS wants it */
			"From: <sip:%s@%s>;tag=%s;epid=1234567890\r\n"
			"To: <%s>%s%s\r\n"
			"Max-Forwards: 10\r\n"
			"CSeq: %d %s\r\n"
			"User-Agent: Purple/" VERSION "\r\n"
			"Call-ID: %s\r\n"
			"%s%s"
			"Content-Length: %" G_GSIZE_FORMAT "\r\n\r\n%s",
			method,
			url,
			sip->udp ? "UDP" : "TCP",
			purple_network_get_my_ip(-1),
			sip->listenport,
			branch,
			sip->username,
			sip->servername,
			dialog ? dialog->ourtag : tag,
			to,
			dialog ? ";tag=" : "",
			dialog ? dialog->theirtag : "",
			++sip->cseq,
			method,
			callid,
			auth ? auth : "",
			addh,
			strlen(body),
			body);

	g_free(tag);
	g_free(auth);
	g_free(branch);
	g_free(callid);

	/* add to ongoing transactions */

	transactions_add_buf(sip, buf, tc);

	sendout_pkt(gc, buf);

	g_free(buf);
}

static char *get_contact(struct simple_account_data  *sip) {
	return g_strdup_printf("<sip:%s@%s:%d;transport=%s>;methods=\"MESSAGE, SUBSCRIBE, NOTIFY\"",
			       sip->username, purple_network_get_my_ip(-1),
			       sip->listenport,
			       sip->udp ? "udp" : "tcp");
}

static void do_register_exp(struct simple_account_data *sip, int expire) {
	char *uri, *to, *contact, *hdr;

	sip->reregister = time(NULL) + expire - 50;

	uri = g_strdup_printf("sip:%s", sip->servername);
	to = g_strdup_printf("sip:%s@%s", sip->username, sip->servername);
	contact = get_contact(sip);
	hdr = g_strdup_printf("Contact: %s\r\nExpires: %d\r\n", contact, expire);
	g_free(contact);

	sip->registerstatus = SIMPLE_REGISTER_SENT;

	send_sip_request(sip->gc, "REGISTER", uri, to, hdr, "", NULL,
		process_register_response);

	g_free(hdr);
	g_free(uri);
	g_free(to);
}

static void do_register(struct simple_account_data *sip) {
	do_register_exp(sip, sip->registerexpire);
}

static gchar *parse_from(const gchar *hdr) {
	gchar *from;
	const gchar *tmp, *tmp2 = hdr;

	if(!hdr) return NULL;
	purple_debug_info("simple", "parsing address out of %s\n", hdr);
	tmp = strchr(hdr, '<');

	/* i hate the different SIP UA behaviours... */
	if(tmp) { /* sip address in <...> */
		tmp2 = tmp + 1;
		tmp = strchr(tmp2, '>');
		if(tmp) {
			from = g_strndup(tmp2, tmp - tmp2);
		} else {
			purple_debug_info("simple", "found < without > in From\n");
			return NULL;
		}
	} else {
		tmp = strchr(tmp2, ';');
		if(tmp) {
			from = g_strndup(tmp2, tmp - tmp2);
		} else {
			from = g_strdup(tmp2);
		}
	}
	purple_debug_info("simple", "got %s\n", from);
	return from;
}
static gchar *find_tag(const gchar *);

static gboolean process_subscribe_response(struct simple_account_data *sip, struct sipmsg *msg, struct transaction *tc) {
	gchar *to = NULL;
	struct simple_buddy *b = NULL;
	gchar *theirtag = NULL, *ourtag = NULL;
	const gchar *callid = NULL;

	purple_debug_info("simple", "process subscribe response\n");

	if(msg->response == 200 || msg->response == 202) {
		if ( (to = parse_from(sipmsg_find_header(msg, "To"))) &&
		      (b = g_hash_table_lookup(sip->buddies, to)) &&
		       !(b->dialog))
		{
			purple_debug_info("simple", "creating dialog"
				" information for a subscription.\n");

			theirtag = find_tag(sipmsg_find_header(msg, "To"));
			ourtag = find_tag(sipmsg_find_header(msg, "From"));
			callid = sipmsg_find_header(msg, "Call-ID");

			if (theirtag && ourtag && callid)
			{
				b->dialog = g_new0(struct sip_dialog, 1);
				b->dialog->ourtag = g_strdup(ourtag);
				b->dialog->theirtag = g_strdup(theirtag);
				b->dialog->callid = g_strdup(callid);

				purple_debug_info("simple", "ourtag: %s\n",
					ourtag);
				purple_debug_info("simple", "theirtag: %s\n",
					theirtag);
				purple_debug_info("simple", "callid: %s\n",
					callid);
				g_free(theirtag);
				g_free(ourtag);
			}
		}
		else
		{
			purple_debug_info("simple", "cannot create dialog!\n");
		}
		return TRUE;
	}

	to = parse_from(sipmsg_find_header(tc->msg, "To")); /* cant be NULL since it is our own msg */

	/* we can not subscribe -> user is offline (TODO unknown status?) */

	purple_protocol_got_user_status(sip->account, to, "offline", NULL);
	g_free(to);
	return TRUE;
}

static void simple_subscribe_exp(struct simple_account_data *sip, struct simple_buddy *buddy, int expiration) {
	gchar *contact, *to, *tmp, *tmp2;

	tmp2 = g_strdup_printf(
		"Expires: %d\r\n"
		"Accept: application/pidf+xml, application/xpidf+xml\r\n"
		"Event: presence\r\n",
		expiration);

	if(strncmp(buddy->name, "sip:", 4))
		to = g_strdup_printf("sip:%s", buddy->name);
	else
		to = g_strdup(buddy->name);

	tmp = get_contact(sip);
	contact = g_strdup_printf("%sContact: %s\r\n", tmp2, tmp);
	g_free(tmp);
	g_free(tmp2);

	send_sip_request(sip->gc, "SUBSCRIBE", to, to, contact,"",buddy->dialog,
			 (expiration > 0) ? process_subscribe_response : NULL);

	g_free(to);
	g_free(contact);

	/* resubscribe before subscription expires */
	/* add some jitter */
	if (expiration > 60)
		buddy->resubscribe = time(NULL) + (expiration - 60) + (g_random_int_range(0, 50));
	else if (expiration > 0)
		buddy->resubscribe = time(NULL) + ((int) (expiration / 2));
}

static void simple_subscribe(struct simple_account_data *sip, struct simple_buddy *buddy) {
	simple_subscribe_exp(sip, buddy, SUBSCRIBE_EXPIRATION);
}

static void simple_unsubscribe(char *name, struct simple_buddy *buddy, struct simple_account_data *sip) {
	if (buddy->dialog)
	{
		purple_debug_info("simple", "Unsubscribing from %s\n", name);
		simple_subscribe_exp(sip, buddy, 0);
	}
}

static gboolean simple_add_lcs_contacts(struct simple_account_data *sip, struct sipmsg *msg, struct transaction *tc) {
	const gchar *tmp;
	PurpleXmlNode *item, *group, *isc;
	const char *name_group;
	PurpleBuddy *b;
	PurpleGroup *g = NULL;
	struct simple_buddy *bs;
	int len = msg->bodylen;


	tmp = sipmsg_find_header(msg, "Event");
	if(tmp && !strncmp(tmp, "vnd-microsoft-roaming-contacts", 30)){

		purple_debug_info("simple", "simple_add_lcs_contacts->%s-%d\n", msg->body, len);
		/*Convert the contact from XML to Purple Buddies*/
		isc = purple_xmlnode_from_str(msg->body, len);

		/* ToDo. Find for all groups */
		if ((group = purple_xmlnode_get_child(isc, "group"))) {
			name_group = purple_xmlnode_get_attrib(group, "name");
			purple_debug_info("simple", "name_group->%s\n", name_group);
			g = purple_blist_find_group(name_group);
			if(!g)
				g = purple_group_new(name_group);
		}

		if (!g)
			g = purple_blist_get_default_group();

		for(item = purple_xmlnode_get_child(isc, "contact"); item; item = purple_xmlnode_get_next_twin(item))
		{
			const char *uri;
			char *buddy_name;
			uri = purple_xmlnode_get_attrib(item, "uri");
			/*name = purple_xmlnode_get_attrib(item, "name");
			groups = purple_xmlnode_get_attrib(item, "groups");*/
			purple_debug_info("simple", "URI->%s\n", uri);

			buddy_name = g_strdup_printf("sip:%s", uri);

			b = purple_blist_find_buddy(sip->account, buddy_name);
			if(!b){
				b = purple_buddy_new(sip->account, buddy_name, uri);
			}
			g_free(buddy_name);

			purple_blist_add_buddy(b, NULL, g, NULL);
			purple_buddy_set_local_alias(b, uri);
			bs = g_new0(struct simple_buddy, 1);
			bs->name = g_strdup(purple_buddy_get_name(b));
			g_hash_table_insert(sip->buddies, bs->name, bs);
		}
		purple_xmlnode_free(isc);
	}
	return 0;
}

static void simple_subscribe_buddylist(struct simple_account_data *sip) {
	gchar *contact = "Event: vnd-microsoft-roaming-contacts\r\nAccept: application/vnd-microsoft-roaming-contacts+xml\r\nSupported: com.microsoft.autoextend\r\nSupported: ms-benotify\r\nProxy-Require: ms-benotify\r\nSupported: ms-piggyback-first-notify\r\n";
	gchar *to;
	gchar *tmp;
	to = g_strdup_printf("sip:%s@%s", sip->username, sip->servername);

	tmp = get_contact(sip);

	contact = g_strdup_printf("%sContact: %s\r\n", contact, tmp);
	g_free(tmp);

	send_sip_request(sip->gc, "SUBSCRIBE", to, to, contact, "", NULL, simple_add_lcs_contacts);

	g_free(to);
	g_free(contact);
}


static void simple_buddy_resub(char *name, struct simple_buddy *buddy, struct simple_account_data *sip) {
	time_t curtime = time(NULL);
	purple_debug_info("simple", "buddy resub\n");
	if(buddy->resubscribe < curtime) {
		purple_debug(PURPLE_DEBUG_MISC, "simple", "simple_buddy_resub %s\n", name);
		simple_subscribe(sip, buddy);
	}
}

static gboolean resend_timeout(struct simple_account_data *sip) {
	GSList *tmp = sip->transactions;
	time_t currtime = time(NULL);
	while(tmp) {
		struct transaction *trans = tmp->data;
		tmp = tmp->next;
		purple_debug_info("simple", "have open transaction age: %lu\n", currtime- trans->time);
		if((currtime - trans->time > 5) && trans->retries >= 1) {
			/* TODO 408 */
		} else {
			if((currtime - trans->time > 2) && trans->retries == 0) {
				gchar *outstr;
				trans->retries++;
				outstr = sipmsg_to_string(trans->msg, NULL);
				sendout_pkt(sip->gc, outstr);
				g_free(outstr);
			}
		}
	}
	return TRUE;
}

static gboolean subscribe_timeout(struct simple_account_data *sip) {
	GSList *tmp;
	time_t curtime = time(NULL);
	/* register again if first registration expires */
	if(sip->reregister < curtime) {
		do_register(sip);
	}

	/* publish status again if our last update is about to expire. */
	if (sip->republish != -1 &&
		sip->republish < curtime &&
		purple_account_get_bool(sip->account, "dopublish", TRUE))
	{
		purple_debug_info("simple", "subscribe_timeout: republishing status.\n");
		send_open_publish(sip);
	}

	/* check for every subscription if we need to resubscribe */
	g_hash_table_foreach(sip->buddies, (GHFunc)simple_buddy_resub, (gpointer)sip);

	/* remove a timed out suscriber */
	tmp = sip->watcher;
	while(tmp) {
		GSList *next = tmp->next;
		struct simple_watcher *watcher = tmp->data;
		if(watcher->expire < curtime) {
			sip->watcher = g_slist_delete_link(sip->watcher, tmp);
			watcher_destroy(watcher);
		}
		tmp = next;
	}

	return TRUE;
}

static void simple_send_message(struct simple_account_data *sip, const char *to, const char *msg, const char *type) {
	gchar *hdr;
	gchar *fullto;
	if(strncmp(to, "sip:", 4))
		fullto = g_strdup_printf("sip:%s", to);
	else
		fullto = g_strdup(to);

	if(type) {
		hdr = g_strdup_printf("Content-Type: %s\r\n", type);
	} else {
		hdr = g_strdup("Content-Type: text/plain\r\n");
	}
	send_sip_request(sip->gc, "MESSAGE", fullto, fullto, hdr, msg, NULL, NULL);
	g_free(hdr);
	g_free(fullto);
}

static int simple_im_send(PurpleConnection *gc, PurpleMessage *msg) {
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	char *to = g_strdup(purple_message_get_recipient(msg));
	char *text = purple_unescape_html(purple_message_get_contents(msg));
	simple_send_message(sip, to, text, NULL);
	g_free(to);
	g_free(text);
	return 1;
}

static void process_incoming_message(struct simple_account_data *sip, struct sipmsg *msg) {
	gchar *from;
	const gchar *contenttype;
	gboolean found = FALSE;

	from = parse_from(sipmsg_find_header(msg, "From"));

	if(!from) return;

	purple_debug(PURPLE_DEBUG_MISC, "simple", "got message from %s: %s\n", from, msg->body);

	contenttype = sipmsg_find_header(msg, "Content-Type");
	if(!contenttype || !strncmp(contenttype, "text/plain", 10) || !strncmp(contenttype, "text/html", 9)) {
		purple_serv_got_im(sip->gc, from, msg->body, 0, time(NULL));
		send_sip_response(sip->gc, msg, 200, "OK", NULL);
		found = TRUE;
	}
	else if(!strncmp(contenttype, "application/im-iscomposing+xml", 30)) {
		PurpleXmlNode *isc = purple_xmlnode_from_str(msg->body, msg->bodylen);
		PurpleXmlNode *state;
		gchar *statedata;

		if(!isc) {
			purple_debug_info("simple", "process_incoming_message: can not parse iscomposing\n");
			g_free(from);
			return;
		}

		state = purple_xmlnode_get_child(isc, "state");

		if(!state) {
			purple_debug_info("simple", "process_incoming_message: no state found\n");
			purple_xmlnode_free(isc);
			g_free(from);
			return;
		}

		statedata = purple_xmlnode_get_data(state);
		if(statedata) {
			if(strstr(statedata, "active"))
				purple_serv_got_typing(sip->gc, from, 0, PURPLE_IM_TYPING);
			else
				purple_serv_got_typing_stopped(sip->gc, from);

			g_free(statedata);
		}
		purple_xmlnode_free(isc);
		send_sip_response(sip->gc, msg, 200, "OK", NULL);
		found = TRUE;
	}
	if(!found) {
		purple_debug_info("simple", "got unknown mime-type\n");
		send_sip_response(sip->gc, msg, 415, "Unsupported media type", NULL);
	}
	g_free(from);
}


gboolean process_register_response(struct simple_account_data *sip, struct sipmsg *msg, struct transaction *tc) {
	const gchar *tmp;
	purple_debug(PURPLE_DEBUG_MISC, "simple", "in process register response response: %d\n", msg->response);
	switch (msg->response) {
		case 200:
			if(sip->registerstatus < SIMPLE_REGISTER_COMPLETE) { /* registered */
				if(purple_account_get_bool(sip->account, "dopublish", TRUE)) {
					send_open_publish(sip);
				}
			}
			sip->registerstatus = SIMPLE_REGISTER_COMPLETE;
			purple_connection_set_state(sip->gc, PURPLE_CONNECTION_CONNECTED);

			/* get buddies from blist */
			simple_get_buddies(sip->gc);

			subscribe_timeout(sip);
			tmp = sipmsg_find_header(msg, "Allow-Events");
		        if(tmp && strstr(tmp, "vnd-microsoft-provisioning")){
				simple_subscribe_buddylist(sip);
			}

			break;
		case 401:
			if(sip->registerstatus != SIMPLE_REGISTER_RETRY) {
				purple_debug_info("simple", "REGISTER retries %d\n", sip->registrar.retries);
				if(sip->registrar.retries > SIMPLE_REGISTER_RETRY_MAX) {
					if (!purple_account_get_remember_password(purple_connection_get_account(sip->gc)))
						purple_account_set_password(purple_connection_get_account(sip->gc), NULL, NULL, NULL);
					purple_connection_error(sip->gc,
						PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
						_("Incorrect password"));
					return TRUE;
				}
				tmp = sipmsg_find_header(msg, "WWW-Authenticate");
				fill_auth(sip, tmp, &sip->registrar);
				sip->registerstatus = SIMPLE_REGISTER_RETRY;
				do_register(sip);
			}
			break;
		default:
			if (sip->registerstatus != SIMPLE_REGISTER_RETRY) {
				purple_debug_info("simple", "Unrecognized return code for REGISTER.\n");
				if (sip->registrar.retries > SIMPLE_REGISTER_RETRY_MAX) {
					purple_connection_error(sip->gc,
						PURPLE_CONNECTION_ERROR_OTHER_ERROR,
						_("Unknown server response"));
					return TRUE;
				}
				sip->registerstatus = SIMPLE_REGISTER_RETRY;
				do_register(sip);
			}
			break;
		}
	return TRUE;
}

static gboolean dialog_match(struct sip_dialog *dialog, struct sipmsg *msg)
{
	const gchar *fromhdr;
	const gchar *tohdr;
	const gchar *callid;
	gchar *ourtag, *theirtag;
	gboolean match = FALSE;

	fromhdr = sipmsg_find_header(msg, "From");
	tohdr = sipmsg_find_header(msg, "To");
	callid = sipmsg_find_header(msg, "Call-ID");

	if (!fromhdr || !tohdr || !callid)
		return FALSE;

	ourtag = find_tag(tohdr);
	theirtag = find_tag(fromhdr);

	if (ourtag && theirtag &&
			purple_strequal(dialog->callid, callid) &&
			purple_strequal(dialog->ourtag, ourtag) &&
			purple_strequal(dialog->theirtag, theirtag))
		match = TRUE;

	g_free(ourtag);
	g_free(theirtag);

	return match;
}

static void process_incoming_notify(struct simple_account_data *sip, struct sipmsg *msg) {
	gchar *from;
	const gchar *fromhdr;
	gchar *basicstatus_data;
	PurpleXmlNode *pidf;
	PurpleXmlNode *basicstatus = NULL, *tuple, *status;
	gboolean isonline = FALSE;
	struct simple_buddy *b = NULL;
	const gchar *sshdr = NULL;

	fromhdr = sipmsg_find_header(msg, "From");
	from = parse_from(fromhdr);
	if(!from) return;

	b = g_hash_table_lookup(sip->buddies, from);
	if (!b)
	{
		g_free(from);
		purple_debug_info("simple", "Could not find the buddy.\n");
		return;
	}

	if (b->dialog && !dialog_match(b->dialog, msg))
	{
		/* We only accept notifies from people that
		 * we already have a dialog with.
		 */
		purple_debug_info("simple","No corresponding dialog for notify--discard\n");
		g_free(from);
		return;
	}

	pidf = purple_xmlnode_from_str(msg->body, msg->bodylen);

	if(!pidf) {
		purple_debug_info("simple", "process_incoming_notify: no parseable pidf\n");
		sshdr = sipmsg_find_header(msg, "Subscription-State");
		if (sshdr)
		{
			int i = 0;
			gchar **ssparts = g_strsplit(sshdr, ":", 0);
			while (ssparts[i])
			{
				g_strchug(ssparts[i]);
				if (g_str_has_prefix(ssparts[i], "terminated")) {
					purple_debug_info("simple", "Subscription expired!");
					if (b->dialog)
					{
						g_free(b->dialog->ourtag);
						g_free(b->dialog->theirtag);
						g_free(b->dialog->callid);
						g_free(b->dialog);
						b->dialog = NULL;
					}

					purple_protocol_got_user_status(sip->account, from, "offline", NULL);
					break;
				}
				i++;
			}
			g_strfreev(ssparts);
		}
		send_sip_response(sip->gc, msg, 200, "OK", NULL);
		g_free(from);
		return;
	}

	if ((tuple = purple_xmlnode_get_child(pidf, "tuple")))
		if ((status = purple_xmlnode_get_child(tuple, "status")))
			basicstatus = purple_xmlnode_get_child(status, "basic");

	if(!basicstatus) {
		purple_debug_info("simple", "process_incoming_notify: no basic found\n");
		purple_xmlnode_free(pidf);
		g_free(from);
		return;
	}

	basicstatus_data = purple_xmlnode_get_data(basicstatus);

	if(!basicstatus_data) {
		purple_debug_info("simple", "process_incoming_notify: no basic data found\n");
		purple_xmlnode_free(pidf);
		g_free(from);
		return;
	}

	if(strstr(basicstatus_data, "open"))
		isonline = TRUE;


	if(isonline)
		purple_protocol_got_user_status(sip->account, from, "available", NULL);
	else
		purple_protocol_got_user_status(sip->account, from, "offline", NULL);

	purple_xmlnode_free(pidf);
	g_free(from);
	g_free(basicstatus_data);

	send_sip_response(sip->gc, msg, 200, "OK", NULL);
}

static unsigned int simple_typing(PurpleConnection *gc, const char *name, PurpleIMTypingState state) {
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);

	gchar *xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\"\n"
			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing iscomposing.xsd\">\n"
			"<state>%s</state>\n"
			"<contenttype>text/plain</contenttype>\n"
			"<refresh>60</refresh>\n"
			"</isComposing>";
	gchar *recv = g_strdup(name);
	if(state == PURPLE_IM_TYPING) {
		gchar *msg = g_strdup_printf(xml, "active");
		simple_send_message(sip, recv, msg, "application/im-iscomposing+xml");
		g_free(msg);
	} else /* TODO: Only if (state == PURPLE_IM_TYPED) ? */ {
		gchar *msg = g_strdup_printf(xml, "idle");
		simple_send_message(sip, recv, msg, "application/im-iscomposing+xml");
		g_free(msg);
	}
	g_free(recv);
	/*
	 * TODO: Is this right?  It will cause the core to call
	 *       purple_serv_send_typing(gc, who, PURPLE_IM_TYPING) once every second
	 *       until the user stops typing.  If that's not desired,
	 *       then return 0 instead.
	 */
	return 1;
}

static gchar *find_tag(const gchar *hdr) {
	const gchar *tmp = strstr(hdr, ";tag="), *tmp2;

	if(!tmp) return NULL;
	tmp += 5;
	if((tmp2 = strchr(tmp, ';'))) {
		return g_strndup(tmp, tmp2 - tmp);
	}
	return g_strdup(tmp);
}

static gchar* gen_xpidf(struct simple_account_data *sip) {
	gchar *doc = g_strdup_printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<presence>\n"
			"<presentity uri=\"sip:%s@%s;method=SUBSCRIBE\"/>\n"
			"<display name=\"sip:%s@%s\"/>\n"
			"<atom id=\"1234\">\n"
			"<address uri=\"sip:%s@%s\">\n"
			"<status status=\"%s\"/>\n"
			"</address>\n"
			"</atom>\n"
			"</presence>\n",
			sip->username,
			sip->servername,
			sip->username,
			sip->servername,
			sip->username,
			sip->servername,
			sip->status);
	return doc;
}

static gchar* gen_pidf(struct simple_account_data *sip, gboolean open) {
	gchar *doc = g_strdup_printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n"
			"xmlns:im=\"urn:ietf:params:xml:ns:pidf:im\"\n"
			"entity=\"sip:%s@%s\">\n"
			"<tuple id=\"bs35r9f\">\n"
			"<status>\n"
			"<basic>%s</basic>\n"
			"</status>\n"
			"<note>%s</note>\n"
			"</tuple>\n"
			"</presence>",
			sip->username,
			sip->servername,
			(open == TRUE) ? "open" : "closed",
			(open == TRUE) ? sip->status : "");
	return doc;
}

static void send_notify(struct simple_account_data *sip, struct simple_watcher *watcher) {
	gchar *doc = watcher->needsxpidf ? gen_xpidf(sip) : gen_pidf(sip, TRUE);
	gchar *hdr = watcher->needsxpidf ? "Event: presence\r\nContent-Type: application/xpidf+xml\r\n" : "Event: presence\r\nContent-Type: application/pidf+xml\r\n";
	send_sip_request(sip->gc, "NOTIFY", watcher->name, watcher->name, hdr, doc, &watcher->dialog, NULL);
	g_free(doc);
}

static gboolean process_publish_response(struct simple_account_data *sip, struct sipmsg *msg, struct transaction *tc) {

	const gchar *etag = NULL;

	if(msg->response != 200 && msg->response != 408) {
		/* never send again */
		sip->republish = -1;
	}

	etag = sipmsg_find_header(msg, "SIP-Etag");
	if (etag) {
		/* we must store the etag somewhere. */
		g_free(sip->publish_etag);
		sip->publish_etag = g_strdup(etag);
	}

	return TRUE;
}

static void send_open_publish(struct simple_account_data *sip) {
	gchar *add_headers = NULL;
	gchar *uri = g_strdup_printf("sip:%s@%s", sip->username, sip->servername);
	gchar *doc = gen_pidf(sip, TRUE);

	add_headers = g_strdup_printf("%s%s%s%s%d\r\n%s",
		sip->publish_etag ? "SIP-If-Match: " : "",
		sip->publish_etag ? sip->publish_etag : "",
		sip->publish_etag ? "\r\n" : "",
		"Expires: ", PUBLISH_EXPIRATION,
		"Event: presence\r\n"
		"Content-Type: application/pidf+xml\r\n");

	send_sip_request(sip->gc, "PUBLISH", uri, uri,
		add_headers, doc, NULL, process_publish_response);
	sip->republish = time(NULL) + PUBLISH_EXPIRATION - 50;
	g_free(uri);
	g_free(doc);
	g_free(add_headers);
}

static void send_closed_publish(struct simple_account_data *sip) {
	gchar *uri = g_strdup_printf("sip:%s@%s", sip->username, sip->servername);
	gchar *add_headers, *doc;

	add_headers = g_strdup_printf("%s%s%s%s",
		sip->publish_etag ? "SIP-If-Match: " : "",
		sip->publish_etag ? sip->publish_etag : "",
		sip->publish_etag ? "\r\n" : "",
		"Expires: 600\r\n"
		"Event: presence\r\n"
		"Content-Type: application/pidf+xml\r\n");

	doc = gen_pidf(sip, FALSE);
	send_sip_request(sip->gc, "PUBLISH", uri, uri, add_headers,
		doc, NULL, process_publish_response);
	/*sip->republish = time(NULL) + 500;*/
	g_free(uri);
	g_free(doc);
	g_free(add_headers);
}

static void process_incoming_subscribe(struct simple_account_data *sip, struct sipmsg *msg) {
	const char *from_hdr = sipmsg_find_header(msg, "From");
	gchar *from = parse_from(from_hdr);
	gchar *theirtag = find_tag(from_hdr);
	gchar *ourtag = find_tag(sipmsg_find_header(msg, "To"));
	gboolean tagadded = FALSE;
	const gchar *callid = sipmsg_find_header(msg, "Call-ID");
	const gchar *expire = sipmsg_find_header(msg, "Expire");
	gchar *tmp;
	struct simple_watcher *watcher = watcher_find(sip, from);
	if(!ourtag) {
		tagadded = TRUE;
		ourtag = gentag();
	}
	if(!watcher) { /* new subscription */
		const gchar *acceptheader = sipmsg_find_header(msg, "Accept");
		gboolean needsxpidf = FALSE;
		if(!purple_account_privacy_check(sip->account, from)) {
			send_sip_response(sip->gc, msg, 202, "Ok", NULL);
			goto privend;
		}
		if(acceptheader) {
			const gchar *tmp = acceptheader;
			gboolean foundpidf = FALSE;
			gboolean foundxpidf = FALSE;
			while(tmp && tmp < acceptheader + strlen(acceptheader)) {
				gchar *tmp2 = strchr(tmp, ',');
				if(tmp2) *tmp2 = '\0';
				if(!g_ascii_strcasecmp("application/pidf+xml", tmp))
					foundpidf = TRUE;
				if(!g_ascii_strcasecmp("application/xpidf+xml", tmp))
					foundxpidf = TRUE;
				if(tmp2) {
					*tmp2 = ',';
					tmp = tmp2 + 1;
					while(*tmp == ' ') tmp++;
				} else
					tmp = NULL;
			}
			if(!foundpidf && foundxpidf) needsxpidf = TRUE;
		}
		watcher = watcher_create(sip, from, callid, ourtag, theirtag, needsxpidf);
	}
	if(tagadded) {
		gchar *to = g_strdup_printf("%s;tag=%s", sipmsg_find_header(msg, "To"), ourtag);
		sipmsg_remove_header(msg, "To");
		sipmsg_add_header(msg, "To", to);
		g_free(to);
	}
	if(expire)
		watcher->expire = time(NULL) + strtol(expire, NULL, 10);
	else
		watcher->expire = time(NULL) + 600;
	sipmsg_remove_header(msg, "Contact");
	tmp = get_contact(sip);
	sipmsg_add_header(msg, "Contact", tmp);
	g_free(tmp);
	purple_debug_info("simple", "got subscribe: name %s ourtag %s theirtag %s callid %s\n", watcher->name, watcher->dialog.ourtag, watcher->dialog.theirtag, watcher->dialog.callid);
	send_sip_response(sip->gc, msg, 200, "Ok", NULL);
	send_notify(sip, watcher);
privend:
	g_free(from);
	g_free(theirtag);
	g_free(ourtag);
}

static void process_input_message(struct simple_account_data *sip, struct sipmsg *msg) {
	gboolean found = FALSE;
	if(msg->response == 0) { /* request */
		if(purple_strequal(msg->method, "MESSAGE")) {
			process_incoming_message(sip, msg);
			found = TRUE;
		} else if(purple_strequal(msg->method, "NOTIFY")) {
			process_incoming_notify(sip, msg);
			found = TRUE;
		} else if(purple_strequal(msg->method, "SUBSCRIBE")) {
			process_incoming_subscribe(sip, msg);
			found = TRUE;
		} else {
			send_sip_response(sip->gc, msg, 501, "Not implemented", NULL);
		}
	} else { /* response */
		struct transaction *trans = transactions_find(sip, msg);
		if(trans) {
			if(msg->response == 407) {
				gchar *resend, *auth;
				const gchar *ptmp;

				if(sip->proxy.retries > 3) return;
				sip->proxy.retries++;
				/* do proxy authentication */

				ptmp = sipmsg_find_header(msg, "Proxy-Authenticate");

				fill_auth(sip, ptmp, &sip->proxy);
				auth = auth_header(sip, &sip->proxy, trans->msg->method, trans->msg->target);
				sipmsg_remove_header(trans->msg, "Proxy-Authorization");
				sipmsg_add_header(trans->msg, "Proxy-Authorization", auth);
				g_free(auth);
				resend = sipmsg_to_string(trans->msg, NULL);
				/* resend request */
				sendout_pkt(sip->gc, resend);
				g_free(resend);
			} else {
				if(msg->response == 100) {
					/* ignore provisional response */
					purple_debug_info("simple", "got trying response\n");
				} else {
					sip->proxy.retries = 0;
					if(purple_strequal(trans->msg->method, "REGISTER")) {

						/* This is encountered when a REGISTER request was ...
						 */
						if(msg->response == 401) {
							/* denied until further authentication was provided. */
							sip->registrar.retries++;
						}
						else if (msg->response != 200) {
							/* denied for some other reason! */
							sip->registrar.retries++;
						}
						else {
							/* accepted! */
							sip->registrar.retries = 0;
						}
					} else {
						if(msg->response == 401) {
							/* This is encountered when a generic (MESSAGE, NOTIFY, etc)
							 * was denied until further authorization is provided.
							 */
							gchar *resend, *auth;
							const gchar *ptmp;

							if(sip->registrar.retries > SIMPLE_REGISTER_RETRY_MAX) return;
							sip->registrar.retries++;

							ptmp = sipmsg_find_header(msg, "WWW-Authenticate");

							fill_auth(sip, ptmp, &sip->registrar);
							auth = auth_header(sip, &sip->registrar, trans->msg->method, trans->msg->target);
							sipmsg_remove_header(trans->msg, "Authorization");
							sipmsg_add_header(trans->msg, "Authorization", auth);
							g_free(auth);
							resend = sipmsg_to_string(trans->msg, NULL);
							/* resend request */
							sendout_pkt(sip->gc, resend);
							g_free(resend);
						} else {
							/* Reset any count of retries that may have
							 * accumulated in the above branch.
							 */
							sip->registrar.retries = 0;
						}
					}
					if(trans->callback) {
						/* call the callback to process response*/
						(trans->callback)(sip, msg, trans);
					}
					sip->transactions =
					        g_slist_remove(sip->transactions, trans);
					transactions_destroy(trans);
				}
			}
			found = TRUE;
		} else {
			purple_debug(PURPLE_DEBUG_MISC, "simple", "received response to unknown transaction");
		}
	}
	if(!found) {
		purple_debug(PURPLE_DEBUG_MISC, "simple", "received a unknown sip message with method %s and response %d\n", msg->method, msg->response);
	}
}

static void process_input(struct simple_account_data *sip, struct sip_connection *conn)
{
	char *cur;
	char *dummy;
	struct sipmsg *msg;
	int restlen;
	cur = conn->inbuf;

	/* according to the RFC remove CRLF at the beginning */
	while(*cur == '\r' || *cur == '\n') {
		cur++;
	}
	if(cur != conn->inbuf) {
		memmove(conn->inbuf, cur, conn->inbufused - (cur - conn->inbuf));
		conn->inbufused = strlen(conn->inbuf);
	}

	/* Received a full Header? */
	if((cur = strstr(conn->inbuf, "\r\n\r\n")) != NULL) {
		time_t currtime = time(NULL);
		cur += 2;
		cur[0] = '\0';
		purple_debug_info("simple", "\n\nreceived - %s\n######\n%s\n#######\n\n", ctime(&currtime), conn->inbuf);
		msg = sipmsg_parse_header(conn->inbuf);

		if(!msg) {
			/* Should we re-use this error message (from lower in the function)? */
			purple_debug_misc("simple", "received a incomplete sip msg: %s\n", conn->inbuf);
			return;
		}

		cur[0] = '\r';
		cur += 2;
		restlen = conn->inbufused - (cur - conn->inbuf);
		if(restlen >= msg->bodylen) {
			dummy = g_new(char, msg->bodylen + 1);
			memcpy(dummy, cur, msg->bodylen);
			dummy[msg->bodylen] = '\0';
			msg->body = dummy;
			cur += msg->bodylen;
			memmove(conn->inbuf, cur, conn->inbuflen - (cur - conn->inbuf));
			conn->inbufused = strlen(conn->inbuf);
		} else {
			sipmsg_free(msg);
			return;
		}
		purple_debug(PURPLE_DEBUG_MISC, "simple", "in process response response: %d\n", msg->response);
		process_input_message(sip, msg);
		sipmsg_free(msg);
	} else {
		purple_debug(PURPLE_DEBUG_MISC, "simple", "received a incomplete sip msg: %s\n", conn->inbuf);
	}
}

static void simple_udp_process(gpointer data, gint source, PurpleInputCondition con) {
	PurpleConnection *gc = data;
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	struct sipmsg *msg;
	int len;
	time_t currtime = time(NULL);

	static char buffer[65536];
	if((len = recv(source, buffer, sizeof(buffer) - 1, 0)) > 0) {
		buffer[len] = '\0';
		purple_debug_info("simple", "\n\nreceived - %s\n######\n%s\n#######\n\n", ctime(&currtime), buffer);
		msg = sipmsg_parse_msg(buffer);
		if (msg) {
			process_input_message(sip, msg);
			sipmsg_free(msg);
		}
	}
}

static gboolean
simple_input_cb(GObject *stream, gpointer data)
{
	GInputStream *input = G_INPUT_STREAM(stream);
	PurpleConnection *gc = data;
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	gssize len;
	struct sip_connection *conn = connection_find(sip, input);
	GError *error = NULL;

	if (!conn) {
		purple_debug_error("simple", "Connection not found!\n");
		return G_SOURCE_REMOVE;
	}

	if(conn->inbuflen < conn->inbufused + SIMPLE_BUF_INC) {
		conn->inbuflen += SIMPLE_BUF_INC;
		conn->inbuf = g_realloc(conn->inbuf, conn->inbuflen);
	}

	len = g_pollable_input_stream_read_nonblocking(
	        G_POLLABLE_INPUT_STREAM(stream), conn->inbuf + conn->inbufused,
	        SIMPLE_BUF_INC - 1, sip->cancellable, &error);
	if (len < 0) {
		if (error->code == G_IO_ERROR_WOULD_BLOCK) {
			g_error_free(error);
			return G_SOURCE_CONTINUE;
		} else if (error->code != G_IO_ERROR_CANCELLED) {
			/* There has been an error reading from the socket */
			purple_debug_info("simple", "simple_input_cb: read error");
			if (sip->input == input) {
				g_clear_object(&sip->input);
				g_clear_object(&sip->output);
			}
			connection_remove(sip, input);
		}
		g_clear_error(&error);
		return G_SOURCE_REMOVE;
	} else if (len == 0) { /* The other end has closed the socket */
		purple_debug_warning("simple", "simple_input_cb: connection closed");
		if (sip->input == input) {
			g_clear_object(&sip->input);
			g_clear_object(&sip->output);
		}
		connection_remove(sip, input);
		return G_SOURCE_REMOVE;
	}

	purple_connection_update_last_received(gc);
	conn->inbufused += len;
	conn->inbuf[conn->inbufused] = '\0';

	process_input(sip, conn);

	return G_SOURCE_CONTINUE;
}

/* Callback for new connections on incoming TCP port */
static void
simple_newconn_cb(G_GNUC_UNUSED GSocketService *service,
                  GSocketConnection *connection, GObject *source_object,
                  G_GNUC_UNUSED gpointer data)
{
	PurpleConnection *gc = PURPLE_CONNECTION(source_object);
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);
	struct sip_connection *conn;
	GSource *source;

	conn = connection_create(sip, g_object_ref(connection));

	source = g_pollable_input_stream_create_source(
	        G_POLLABLE_INPUT_STREAM(sip->input), sip->cancellable);
	g_source_set_callback(source, (GSourceFunc)simple_input_cb, gc, NULL);
	conn->inputhandler = g_source_attach(source, NULL);
	g_source_unref(source);
}

static void
login_cb(GObject *sender, GAsyncResult *res, gpointer data)
{
	PurpleConnection *gc = data;
	struct simple_account_data *sip;
	struct sip_connection *conn;
	GSocketConnection *sockconn;
	GSource *source;
	GError *error = NULL;

	sockconn = g_socket_client_connect_to_host_finish(G_SOCKET_CLIENT(sender),
	                                                  res, &error);
	if (sockconn == NULL) {
		if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_clear_error(&error);
		} else {
			purple_connection_take_error(gc, error);
		}
		return;
	}

	sip = purple_connection_get_protocol_data(gc);
	sip->input = g_io_stream_get_input_stream(G_IO_STREAM(sockconn));
	sip->output = purple_queued_output_stream_new(
	        g_io_stream_get_output_stream(G_IO_STREAM(sockconn)));

	conn = connection_create(sip, sockconn);

	sip->registertimeout = g_timeout_add_seconds(
	        g_random_int_range(10, 100), (GSourceFunc)subscribe_timeout, sip);

	do_register(sip);

	source = g_pollable_input_stream_create_source(
	        G_POLLABLE_INPUT_STREAM(sip->input), sip->cancellable);
	g_source_set_callback(source, (GSourceFunc)simple_input_cb, gc, NULL);
	conn->inputhandler = g_source_attach(source, NULL);
	g_source_unref(source);
}

static guint simple_ht_hash_nick(const char *nick) {
	char *lc = g_utf8_strdown(nick, -1);
	guint bucket = g_str_hash(lc);
	g_free(lc);

	return bucket;
}

static gboolean simple_ht_equals_nick(const char *nick1, const char *nick2) {
	return (purple_utf8_strcasecmp(nick1, nick2) == 0);
}

static void simple_udp_host_resolved_listen_cb(int listenfd, gpointer data) {
	struct simple_account_data *sip = (struct simple_account_data*) data;

	sip->listen_data = NULL;

	if(listenfd == -1) {
		purple_connection_error(sip->gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			_("Unable to create listen socket"));
		return;
	}

	/*
	 * TODO: Is it correct to set sip->fd to the listenfd?  For the TCP
	 *       listener we set sip->listenfd, but maybe UDP is different?
	 *       Maybe we use the same fd for outgoing data or something?
	 */
	sip->fd = listenfd;

	sip->listenport = purple_network_get_port_from_fd(sip->fd);

	sip->listenpa = purple_input_add(sip->fd, PURPLE_INPUT_READ, simple_udp_process, sip->gc);

	sip->resendtimeout = g_timeout_add(2500, (GSourceFunc) resend_timeout, sip);
	sip->registertimeout = g_timeout_add_seconds(
	        g_random_int_range(10, 100), (GSourceFunc)subscribe_timeout, sip);
	do_register(sip);
}

static void
simple_udp_host_resolved(GObject *sender, GAsyncResult *result, gpointer data) {
	GError *error = NULL;
	GList *addresses = NULL;
	GInetAddress *inet_address = NULL;
	GSocketAddress *socket_address = NULL;
	struct simple_account_data *sip = (struct simple_account_data*) data;

	addresses = g_resolver_lookup_by_name_finish(G_RESOLVER(sender),
			result, &error);
	if(error) {
		gchar *msg = g_strdup_printf(_("Unable to resolve hostname : %s"),
			error->message);

		purple_connection_error(sip->gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			msg
			);

		g_error_free(error);

		return;
	}

	inet_address = G_INET_ADDRESS(addresses->data);
	socket_address = g_inet_socket_address_new(inet_address, sip->realport);
	g_object_unref(G_OBJECT(inet_address));

	g_socket_address_to_native(socket_address,
	                           &(sip->serveraddr),
	                           g_socket_address_get_native_size(socket_address),
	                           NULL);

	g_object_unref(G_OBJECT(socket_address));

	g_resolver_free_addresses(addresses);

	/* create socket for incoming connections */
	sip->listen_data = purple_network_listen_range(5060, 5160, AF_UNSPEC, SOCK_DGRAM, TRUE,
				simple_udp_host_resolved_listen_cb, sip);
	if (sip->listen_data == NULL) {
		purple_connection_error(sip->gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			_("Unable to create listen socket"));
		return;
	}
}

static void
srvresolved(GObject *sender, GAsyncResult *result, gpointer data) {
	GError *error = NULL;
	GList *targets = NULL;
	struct simple_account_data *sip;
	gchar *hostname;
	int port;

	sip = data;

	targets = g_resolver_lookup_service_finish(G_RESOLVER(sender),
			result, &error);
	if(error) {
		purple_debug_info("simple",
		                  "srv lookup failed, continuing with configured settings : %s",
		                  error->message);

		g_error_free(error);

		if(!purple_account_get_bool(sip->account, "useproxy", FALSE)) {
			hostname = g_strdup(sip->servername);
		} else {
			hostname = g_strdup(purple_account_get_string(sip->account, "proxy", sip->servername));
		}		
		port = purple_account_get_int(sip->account, "port", 0);
	} else {
		GSrvTarget *target = (GSrvTarget *)targets->data;

		hostname = g_strdup(g_srv_target_get_hostname(target));
		port = g_srv_target_get_port(target);

		g_resolver_free_targets(targets);
	}

	sip->realhostname = hostname;
	sip->realport = port;

	if(!sip->realport)
		sip->realport = 5060;

	/* TCP case */
	if(!sip->udp) {
		GSocketClient *client;
		/* create service for incoming connections */
		sip->service = g_socket_service_new();
		sip->listenport = purple_socket_listener_add_any_inet_port(
		        G_SOCKET_LISTENER(sip->service), G_OBJECT(sip->gc), &error);
		if (sip->listenport == 0) {
			purple_connection_take_error(sip->gc, error);
			return;
		}
		g_signal_connect(sip->service, "incoming",
		                 G_CALLBACK(simple_newconn_cb), NULL);

		purple_debug_info("simple", "connecting to %s port %d",
		                  sip->realhostname, sip->realport);

		client = purple_gio_socket_client_new(sip->account, &error);
		if (client == NULL) {
			purple_connection_take_error(sip->gc, error);
			return;
		}

		/* open tcp connection to the server */
		g_socket_client_connect_to_host_async(client, sip->realhostname,
		                                      sip->realport, sip->cancellable,
		                                      login_cb, sip->gc);
		g_object_unref(client);
	} else { /* UDP */
		GResolver *resolver = g_resolver_get_default();

		purple_debug_info("simple", "using udp with server %s and port %d\n", hostname, port);

		g_resolver_lookup_by_name_async(resolver,
		                                sip->realhostname,
		                                sip->cancellable,
		                                simple_udp_host_resolved,
		                                sip);
		g_object_unref(resolver);
	}
}

static void simple_login(PurpleAccount *account)
{
	PurpleConnection *gc;
	struct simple_account_data *sip;
	gchar **userserver;
	const gchar *hosttoconnect;
	const char *username = purple_account_get_username(account);
	GResolver *resolver;

	gc = purple_account_get_connection(account);

	purple_connection_set_flags(gc, PURPLE_CONNECTION_FLAG_NO_IMAGES);

	if (strpbrk(username, " \t\v\r\n") != NULL) {
		purple_connection_error(gc,
			PURPLE_CONNECTION_ERROR_INVALID_SETTINGS,
			_("SIP usernames may not contain whitespaces or @ symbols"));
		return;
	}

	sip = g_new0(struct simple_account_data, 1);
	purple_connection_set_protocol_data(gc, sip);
	sip->gc = gc;
	sip->fd = -1;
	sip->account = account;
	sip->registerexpire = 900;
	sip->udp = purple_account_get_bool(account, "udp", FALSE);
	/* TODO: is there a good default grow size? */
	if(!sip->udp)
		sip->txbuf = purple_circular_buffer_new(0);

	userserver = g_strsplit(username, "@", 2);
	if (userserver[1] == NULL || userserver[1][0] == '\0') {
		purple_connection_error(gc,
			PURPLE_CONNECTION_ERROR_INVALID_SETTINGS,
			_("SIP connect server not specified"));
		g_strfreev(userserver);
		return;
	}

	purple_connection_set_display_name(gc, userserver[0]);
	sip->username = g_strdup(userserver[0]);
	sip->servername = g_strdup(userserver[1]);
	sip->password = g_strdup(purple_connection_get_password(gc));
	g_strfreev(userserver);

	sip->buddies = g_hash_table_new((GHashFunc)simple_ht_hash_nick, (GEqualFunc)simple_ht_equals_nick);

	purple_connection_update_progress(gc, _("Connecting"), 1, 2);

	/* TODO: Set the status correctly. */
	sip->status = g_strdup("available");

	if(!purple_account_get_bool(account, "useproxy", FALSE)) {
		hosttoconnect = sip->servername;
	} else {
		hosttoconnect = purple_account_get_string(account, "proxy", sip->servername);
	}

	resolver = g_resolver_get_default();
	g_resolver_lookup_service_async(resolver,
	                                "sip",
	                                sip->udp ? "udp" : "tcp",
	                                hosttoconnect,
	                                sip->cancellable,
	                                srvresolved,
	                                sip);
	g_object_unref(resolver);
}

static void simple_close(PurpleConnection *gc)
{
	struct simple_account_data *sip = purple_connection_get_protocol_data(gc);

	if (!sip)
		return;

	/* unregister */
	if (sip->registerstatus == SIMPLE_REGISTER_COMPLETE)
	{
		g_hash_table_foreach(sip->buddies,
			(GHFunc)simple_unsubscribe,
			(gpointer)sip);

		if (purple_account_get_bool(sip->account, "dopublish", TRUE))
			send_closed_publish(sip);

		do_register_exp(sip, 0);
	}
	g_slist_free_full(sip->openconns, (GDestroyNotify)connection_destroy);

	if (sip->listenpa)
		purple_input_remove(sip->listenpa);
	if (sip->resendtimeout)
		g_source_remove(sip->resendtimeout);
	if (sip->registertimeout)
		g_source_remove(sip->registertimeout);

	g_cancellable_cancel(sip->cancellable);
	g_object_unref(G_OBJECT(sip->cancellable));

	if (sip->service) {
		g_socket_service_stop(sip->service);
	}
	g_clear_object(&sip->service);

	if (sip->listen_data != NULL)
		purple_network_listen_cancel(sip->listen_data);

	g_clear_object(&sip->input);
	g_clear_object(&sip->output);
	if (sip->fd >= 0)
		close(sip->fd);

	g_free(sip->servername);
	g_free(sip->username);
	g_free(sip->password);
	g_free(sip->registrar.nonce);
	g_free(sip->registrar.opaque);
	g_free(sip->registrar.target);
	g_free(sip->registrar.realm);
	g_free(sip->registrar.digest_session_key);
	g_free(sip->proxy.nonce);
	g_free(sip->proxy.opaque);
	g_free(sip->proxy.target);
	g_free(sip->proxy.realm);
	g_free(sip->proxy.digest_session_key);
	g_free(sip->status);
	g_hash_table_destroy(sip->buddies);
	g_free(sip->regcallid);
	g_slist_free_full(sip->transactions, (GDestroyNotify)transactions_destroy);
	g_slist_free_full(sip->watcher, (GDestroyNotify)watcher_destroy);
	g_free(sip->publish_etag);
	if (sip->txbuf)
		g_object_unref(G_OBJECT(sip->txbuf));
	g_free(sip->realhostname);

	g_free(sip);
	purple_connection_set_protocol_data(gc, NULL);
}

static void
simple_protocol_init(SIMPLEProtocol *self)
{
	PurpleProtocol *protocol = PURPLE_PROTOCOL(self);
	PurpleAccountUserSplit *split;
	PurpleAccountOption *option;

	protocol->id   = "prpl-simple";
	protocol->name = "SIMPLE";

	split = purple_account_user_split_new(_("Server"), "", '@');
	protocol->user_splits = g_list_append(protocol->user_splits, split);

	option = purple_account_option_bool_new(_("Publish status (note: everyone may watch you)"), "dopublish", TRUE);
	protocol->account_options = g_list_append(protocol->account_options, option);

	option = purple_account_option_int_new(_("Connect port"), "port", 0);
	protocol->account_options = g_list_append(protocol->account_options, option);

	option = purple_account_option_bool_new(_("Use UDP"), "udp", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_bool_new(_("Use proxy"), "useproxy", FALSE);
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_string_new(_("Proxy"), "proxy", "");
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_string_new(_("Auth User"), "authuser", "");
	protocol->account_options = g_list_append(protocol->account_options, option);
	option = purple_account_option_string_new(_("Auth Domain"), "authdomain", "");
	protocol->account_options = g_list_append(protocol->account_options, option);
}

static void
simple_protocol_class_init(SIMPLEProtocolClass *klass)
{
	PurpleProtocolClass *protocol_class = PURPLE_PROTOCOL_CLASS(klass);

	protocol_class->login = simple_login;
	protocol_class->close = simple_close;
	protocol_class->status_types = simple_status_types;
	protocol_class->list_icon = simple_list_icon;
}

static void
simple_protocol_class_finalize(G_GNUC_UNUSED SIMPLEProtocolClass *klass)
{
}

static void
simple_protocol_server_iface_init(PurpleProtocolServerInterface *server_iface)
{
	server_iface->set_status   = simple_set_status;
	server_iface->add_buddy    = simple_add_buddy;
	server_iface->remove_buddy = simple_remove_buddy;
	server_iface->keepalive    = simple_keep_alive;
	server_iface->send_raw     = simple_send_raw;
}

static void
simple_protocol_im_iface_init(PurpleProtocolIMInterface *im_iface)
{
	im_iface->send        = simple_im_send;
	im_iface->send_typing = simple_typing;
}

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
        SIMPLEProtocol, simple_protocol, PURPLE_TYPE_PROTOCOL, 0,

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_SERVER,
                                      simple_protocol_server_iface_init)

        G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_IM,
                                      simple_protocol_im_iface_init));

static PurplePluginInfo *
plugin_query(GError **error)
{
	const gchar * const authors[] = {
		"Thomas Butter <butter@uni-mannheim.de>",
		NULL
	};

	return purple_plugin_info_new(
		"id",           "prpl-simple",
		"name",         "SIMPLE Protocol",
		"version",		DISPLAY_VERSION,
		"category",		N_("Protocol"),
		"summary",		N_("SIP/SIMPLE Protocol Plugin"),
		"description",	N_("The SIP/SIMPLE Protocol Plugin"),
		"authors",		authors,
		"website",		PURPLE_WEBSITE,
		"abi-version",	PURPLE_ABI_VERSION,
		"flags",        PURPLE_PLUGIN_INFO_FLAGS_INTERNAL |
		                PURPLE_PLUGIN_INFO_FLAGS_AUTO_LOAD,
		NULL
	);
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
	simple_protocol_register_type(G_TYPE_MODULE(plugin));

	my_protocol = purple_protocols_add(SIMPLE_TYPE_PROTOCOL, error);
	if (!my_protocol)
		return FALSE;

	purple_signal_connect(purple_get_core(), "uri-handler", plugin,
			PURPLE_CALLBACK(simple_uri_handler), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
	purple_signal_disconnect(purple_get_core(), "uri-handler", plugin,
			PURPLE_CALLBACK(simple_uri_handler));

	if (!purple_protocols_remove(my_protocol, error))
		return FALSE;

	return TRUE;
}

PURPLE_PLUGIN_INIT(simple, plugin_query, plugin_load, plugin_unload);
