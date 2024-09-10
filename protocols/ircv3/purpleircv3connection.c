/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include <birb.h>

#include <hasl.h>

#include "purpleircv3connection.h"

#include "purpleircv3core.h"
#include "purpleircv3messagehandlers.h"

enum {
	PROP_0,
	PROP_CLIENT,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleIRCv3Connection {
	PurpleConnection parent;

	IbisClient *client;

	char *server_name;

	PurpleConversation *status_conversation;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(PurpleIRCv3Connection,
                               purple_ircv3_connection,
                               PURPLE_TYPE_CONNECTION,
                               G_TYPE_FLAG_FINAL,
                               {})

static gboolean
purple_ircv3_connection_unknown_message_cb(IbisClient *client,
                                           const char *command,
                                           IbisMessage *message,
                                           gpointer data);

static gboolean
purple_ircv3_connection_saslsuccess(IbisClient *client, const char *command,
                                    IbisMessage *message, gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_ircv3_connection_rejoin_channels(PurpleIRCv3Connection *connection) {
	PurpleAccount *account = NULL;
	PurpleConversationManager *manager = NULL;
	GList *conversations = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	manager = purple_conversation_manager_get_default();

	conversations = purple_conversation_manager_get_all(manager);
	while(conversations != NULL) {
		PurpleConversation *conversation = conversations->data;
		PurpleAccount *conv_account = NULL;

		conv_account = purple_conversation_get_account(conversation);
		if(conv_account == account) {
			IbisMessage *message = NULL;
			const char *id = purple_conversation_get_id(conversation);

			message = ibis_message_new(IBIS_MSG_JOIN);
			ibis_message_set_params(message, id, NULL);
			ibis_client_write(connection->client, message);
		}

		conversations = g_list_delete_link(conversations, conversations);
	}
}

static inline void
purple_ircv3_connection_setup_sasl(PurpleIRCv3Connection *connection,
                                   PurpleAccount *account)
{
	HaslContext *hasl_context = NULL;
	const char *value = NULL;
	gboolean clear_text = FALSE;

	hasl_context = hasl_context_new();

	value = purple_account_get_string(account, "sasl-login-name", NULL);
	if(!purple_strempty(value)) {
		hasl_context_set_username(hasl_context, value);
	} else {
		hasl_context_set_username(hasl_context,
		                          ibis_client_get_nick(connection->client));

		/* Since the user doesn't have a SASL login name set, we'll listen for
		 * IBIS_RPL_SASLSUCCESS and use that to set the login name to the
		 * username in HASL which worked if the signal handler gets called.
		 */
		g_signal_connect_object(connection->client,
		                        "message::" IBIS_RPL_SASLSUCCESS,
		                        G_CALLBACK(purple_ircv3_connection_saslsuccess),
		                        connection, G_CONNECT_DEFAULT);
	}

	value = purple_connection_get_password(PURPLE_CONNECTION(connection));
	hasl_context_set_password(hasl_context, value);

	value = purple_account_get_string(account, "sasl-mechanisms", NULL);
	if(!purple_strempty(value)) {
		hasl_context_set_allowed_mechanisms(hasl_context, value);
	}

	clear_text = purple_account_get_bool(account, "plain-sasl-in-clear",
	                                     FALSE);
	hasl_context_set_allow_clear_text(hasl_context, clear_text);

	ibis_client_set_hasl_context(connection->client, hasl_context);
	g_clear_object(&hasl_context);
}

/**
 * purple_ircv3_write_status_message: (skip)
 * @connection: The instance.
 * @ibis_message: The message to write.
 * @show_command: Whether or not to display the command.
 *
 * Writes @ibis_message to the status window of @connection.
 */
static void
purple_ircv3_write_status_message(PurpleIRCv3Connection *connection,
                                  IbisMessage *ibis_message,
                                  gboolean show_command)
{
	PurpleContact *contact = NULL;
	PurpleMessage *purple_message = NULL;
	GString *str = NULL;
	GStrv params = NULL;
	char *body = NULL;

	str = g_string_new("");

	if(show_command) {
		const char *command = NULL;

		command = ibis_message_get_command(ibis_message);

		if(!purple_strempty(command)) {
			g_string_append_printf(str, "%s ", command);
		}
	}

	params = ibis_message_get_params(ibis_message);
	body = g_strjoinv(" ", params);
	g_string_append(str, body);
	g_free(body);

	contact = purple_ircv3_connection_find_or_create_contact(connection,
	                                                         ibis_message);

	purple_message = purple_message_new(PURPLE_CONTACT_INFO(contact),
	                                    str->str);
	g_string_free(str, TRUE);

	purple_conversation_write_message(connection->status_conversation,
	                                  purple_message);
	g_clear_object(&purple_message);
}

/**
 * purple_ircv3_write_server_status_message: (skip)
 * @connection: The instance.
 * @message: The message to write to the status window.
 * @show_command: Whether or not to display the command.
 *
 * Like purple_ircv3_write_status_message() but removes the first parameter
 * which is the user's nick for server messages.
 */
static void
purple_ircv3_write_server_status_message(PurpleIRCv3Connection *connection,
                                         IbisMessage *message,
                                         gboolean show_command)
{
	GStrv original = NULL;
	GStrv extra_crispy = NULL;

	original = ibis_message_get_params(message);
	extra_crispy = g_strdupv(original + 1);
	ibis_message_set_paramsv(message, extra_crispy);
	g_strfreev(extra_crispy);

	purple_ircv3_write_status_message(connection, message, show_command);
}

/******************************************************************************
 * Message Handlers
 *****************************************************************************/
static void
purple_ircv3_wrote_message_echo_cb(G_GNUC_UNUSED IbisClient *client,
                                   IbisMessage *message, gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleAccount *account = NULL;
	PurpleContactInfo *contact = NULL;
	PurpleMessage *purple_message = NULL;
	GStrv params = NULL;
	char *body = NULL;
	const char *command = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	contact = purple_account_get_contact_info(account);

	command = ibis_message_get_command(message);
	params = ibis_message_get_params(message);
	if(params != NULL) {
		char *paramsv = g_strjoinv(" ", params);
		body = g_strdup_printf("%s %s", command, paramsv);
		g_free(paramsv);
	} else {
		body = g_strdup(command);
	}

	purple_message = purple_message_new(PURPLE_CONTACT_INFO(contact), body);
	g_clear_pointer(&body, g_free);

	purple_conversation_write_message(connection->status_conversation,
	                                  purple_message);
	g_clear_object(&purple_message);
}

static gboolean
purple_ircv3_message_handler_ignore(G_GNUC_UNUSED IbisClient *client,
                                    G_GNUC_UNUSED const char *command,
                                    G_GNUC_UNUSED IbisMessage *message,
                                    G_GNUC_UNUSED gpointer data)
{
	return TRUE;
}

static gboolean
purple_ircv3_connection_saslsuccess(IbisClient *client,
                                    G_GNUC_UNUSED const char *command,
                                    G_GNUC_UNUSED IbisMessage *message,
                                    gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleAccount *account = NULL;
	const char *value = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	value = purple_account_get_string(account, "sasl-login-name", NULL);

	/* If the sasl-login-name is empty, we set it to the current username in
	 * our hasl context that was used to login.
	 */
	if(purple_strempty(value)) {
		HaslContext *hasl_context = NULL;

		hasl_context = ibis_client_get_hasl_context(client);
		if(HASL_IS_CONTEXT(hasl_context)) {
			purple_account_set_string(account, "sasl-login-name",
			                          hasl_context_get_username(hasl_context));

		}
	}

	/* We don't actually handle SASLSUCCESS, but we just needed to know if it
	 * was sent.
	 */
	return FALSE;
}

static gboolean
purple_ircv3_server_message_echo(G_GNUC_UNUSED IbisClient *client,
                                 G_GNUC_UNUSED const char *command,
                                 IbisMessage *message,
                                 gpointer data)
{
	purple_ircv3_write_status_message(data, message, TRUE);

	return FALSE;
}

static gboolean
purple_ircv3_server_message_handler(G_GNUC_UNUSED IbisClient *client,
                                    G_GNUC_UNUSED const char *command,
                                    IbisMessage *message, gpointer data)
{
	GStrv original = NULL;
	GStrv extra_crispy = NULL;

	original = ibis_message_get_params(message);
	extra_crispy = g_strdupv(original + 1);
	ibis_message_set_paramsv(message, extra_crispy);
	g_strfreev(extra_crispy);

	purple_ircv3_write_status_message(data, message, FALSE);

	return TRUE;
}

static gboolean
purple_ircv3_rpl_welcome_handler(G_GNUC_UNUSED IbisClient *client,
                                 G_GNUC_UNUSED const char *command,
                                 IbisMessage *message, gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleAccount *account = NULL;
	PurpleContactInfo *info = NULL;
	GStrv params = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	info = purple_account_get_contact_info(account);

	params = ibis_message_get_params(message);
	if(params != NULL) {
		/* Per https://modern.ircdocs.horse/#connection-registration the first
		 * parameter of RPL_WELCOME is the nick that the server assigned us.
		 */
		purple_contact_info_set_id(info, params[0]);
	}

	purple_ircv3_write_server_status_message(connection, message, FALSE);

	return TRUE;
}

static gboolean
purple_ircv3_server_rpl_isupport(G_GNUC_UNUSED IbisClient *client,
                                 G_GNUC_UNUSED const char *command,
                                 IbisMessage *message, gpointer data)
{
	PurpleIRCv3Connection *connection = data;

	purple_ircv3_write_server_status_message(connection, message, FALSE);

	/* We want the default handler to run which will populate the features
	 * object on the client, so we return false here.
	 */

	return FALSE;
}

static gboolean
purple_ircv3_server_no_motd_handler(G_GNUC_UNUSED IbisClient *client,
                                    G_GNUC_UNUSED const char *command,
                                    IbisMessage *ibis_message,
                                    gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleContact *author = NULL;
	PurpleMessage *purple_message = NULL;

	author = purple_ircv3_connection_find_or_create_contact(connection,
	                                                        ibis_message);
	purple_message = purple_message_new(PURPLE_CONTACT_INFO(author),
	                                    _("no message of the day found"));
	purple_conversation_write_message(connection->status_conversation,
	                                  purple_message);
	g_clear_object(&purple_message);

	return TRUE;
}

static void
purple_ircv3_connection_add_message_handlers(PurpleIRCv3Connection *connection,
                                             IbisClient *client)
{
	g_signal_connect_object(client, "wrote-message::" IBIS_MSG_PING,
	                        G_CALLBACK(purple_ircv3_wrote_message_echo_cb),
	                        connection, 0);
	g_signal_connect_object(client, "wrote-message::" IBIS_MSG_PONG,
	                        G_CALLBACK(purple_ircv3_wrote_message_echo_cb),
	                        connection, 0);

	g_signal_connect_object(client, "message::" IBIS_MSG_PING,
	                        G_CALLBACK(purple_ircv3_server_message_echo),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_MSG_PONG,
	                        G_CALLBACK(purple_ircv3_server_message_echo),
	                        connection, G_CONNECT_DEFAULT);

	g_signal_connect_object(client, "message::" IBIS_RPL_WELCOME,
	                        G_CALLBACK(purple_ircv3_rpl_welcome_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_YOURHOST,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_CREATED,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_MYINFO,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_ISUPPORT,
	                        G_CALLBACK(purple_ircv3_server_rpl_isupport),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_LUSERCLIENT,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_LUSEROP,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_LUSERUNKNOWN,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_LUSERCHANNELS,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_LUSERME,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_LOCALUSERS,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_GLOBALUSERS,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_MOTD,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_MOTDSTART,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_UMODEIS,
	                        G_CALLBACK(purple_ircv3_server_message_handler),
	                        connection, G_CONNECT_DEFAULT);

	g_signal_connect_object(client, "message::" IBIS_ERR_NOMOTD,
	                        G_CALLBACK(purple_ircv3_server_no_motd_handler),
	                        connection, G_CONNECT_DEFAULT);

	g_signal_connect_object(client, "message::" IBIS_MSG_TOPIC,
	                        G_CALLBACK(purple_ircv3_message_handler_topic),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_NOTOPIC,
	                        G_CALLBACK(purple_ircv3_message_handler_topic),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_TOPIC,
	                        G_CALLBACK(purple_ircv3_message_handler_topic),
	                        connection, G_CONNECT_DEFAULT);

	g_signal_connect_object(client, "message::" IBIS_MSG_PRIVMSG,
	                        G_CALLBACK(purple_ircv3_message_handler_privmsg),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_MSG_NOTICE,
	                        G_CALLBACK(purple_ircv3_message_handler_privmsg),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_MSG_TAGMSG,
	                        G_CALLBACK(purple_ircv3_message_handler_tagmsg),
	                        connection, G_CONNECT_DEFAULT);

	g_signal_connect_object(client, "message::" IBIS_MSG_JOIN,
	                        G_CALLBACK(purple_ircv3_message_handler_join),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_MSG_PART,
	                        G_CALLBACK(purple_ircv3_message_handler_part),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_NAMREPLY,
	                        G_CALLBACK(purple_ircv3_message_handler_namreply),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_RPL_ENDOFNAMES,
	                        G_CALLBACK(purple_ircv3_message_handler_ignore),
	                        connection, G_CONNECT_DEFAULT);


	g_signal_connect_object(client, "message",
	                        G_CALLBACK(purple_ircv3_connection_unknown_message_cb),
	                        connection, G_CONNECT_AFTER);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
purple_ircv3_connection_unknown_message_cb(G_GNUC_UNUSED IbisClient *client,
                                           G_GNUC_UNUSED const char *command,
                                           IbisMessage *ibis_message,
                                           gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleContact *author = NULL;
	PurpleMessage *purple_message = NULL;
	char *contents = NULL;

	author = purple_ircv3_connection_find_or_create_contact(connection,
	                                                        ibis_message);
	contents = g_strdup_printf(_("unhandled message: '%s'"),
	                           ibis_message_get_raw_message(ibis_message));

	purple_message = purple_message_new(PURPLE_CONTACT_INFO(author), contents);
	purple_conversation_write_message(connection->status_conversation,
	                                  purple_message);
	g_clear_object(&purple_message);

	g_free(contents);

	return TRUE;
}

static void
purple_ircv3_connection_connect_cb(GObject *source,
                                   G_GNUC_UNUSED GParamSpec *pspec,
                                   gpointer data)
{
	PurpleConnection *connection = data;
	IbisClient *client = IBIS_CLIENT(source);

	if(ibis_client_get_connected(client)) {
		purple_connection_set_state(connection,
		                            PURPLE_CONNECTION_STATE_CONNECTED);

		/* Once reconnected, we need to rejoin any channels that the
		 * conversation manager has for us.
		 */
		purple_ircv3_connection_rejoin_channels(PURPLE_IRCV3_CONNECTION(connection));
	} else {
		purple_connection_set_state(connection,
		                            PURPLE_CONNECTION_STATE_DISCONNECTED);
	}
}

static void
purple_ircv3_connection_error_cb(GObject *source,
                                 G_GNUC_UNUSED GParamSpec *pspec,
                                 gpointer data)
{
	IbisClient *client = IBIS_CLIENT(source);
	PurpleConnection *connection = data;
	GError *error = NULL;

	error = ibis_client_get_error(client);
	if(error != NULL) {
		purple_connection_g_error(connection, error);
	}
}

static void
purple_ircv3_connection_capabilities_ready_cb(IbisCapabilities *capabilities,
                                              G_GNUC_UNUSED gpointer data)
{
	/* account-tag just adds an account tag to everything if it's available.
	 * The account-tag is the user's username for authentication for all users
	 * not just the one using libpurple.
	 */
	ibis_capabilities_lookup_and_request(capabilities,
	                                     IBIS_CAPABILITY_ACCOUNT_TAG);
}

/******************************************************************************
 * PurpleConnection Implementation
 *****************************************************************************/
static gboolean
purple_ircv3_connection_connect(PurpleConnection *purple_connection,
                                GError **error)
{
	PurpleIRCv3Connection *connection = NULL;
	PurpleAccount *account = NULL;
	GCancellable *cancellable = NULL;
	IbisCapabilities *capabilities = NULL;
	GError *local_error = NULL;
	GProxyResolver *resolver = NULL;
	const char *password = NULL;
	const char *value = NULL;
	int default_port = PURPLE_IRCV3_DEFAULT_TLS_PORT;
	int port = 0;
	gboolean tls = TRUE;
	gboolean require_password = FALSE;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(purple_connection), FALSE);

	connection = PURPLE_IRCV3_CONNECTION(purple_connection);
	account = purple_connection_get_account(purple_connection);

	connection->client = ibis_client_new();
	g_signal_connect_object(connection->client, "notify::connected",
	                        G_CALLBACK(purple_ircv3_connection_connect_cb),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(connection->client, "notify::error",
	                        G_CALLBACK(purple_ircv3_connection_error_cb),
	                        connection, G_CONNECT_DEFAULT);
	purple_ircv3_connection_add_message_handlers(connection,
	                                             connection->client);

	ibis_client_set_nick(connection->client,
	                     purple_connection_get_display_name(purple_connection));

	value = purple_account_get_string(account, "ident", NULL);
	ibis_client_set_username(connection->client, value);

	value = purple_account_get_string(account, "real-name", NULL);
	ibis_client_set_realname(connection->client, value);

	password = purple_account_get_string(account, "server-password", NULL);

	/* Turn on TLS if requested. */
	tls = purple_account_get_bool(account, "use-tls", TRUE);

	/* If TLS is not being used, set the default port to the plain port. */
	if(!tls) {
		default_port = PURPLE_IRCV3_DEFAULT_PLAIN_PORT;
	}
	port = purple_account_get_int(account, "port", default_port);

	require_password = purple_account_get_require_password(account);
	if(require_password) {
		purple_ircv3_connection_setup_sasl(connection, account);
	}

	cancellable = purple_connection_get_cancellable(purple_connection);

	/* Connect to the ready signal of capabilities. */
	capabilities = ibis_client_get_capabilities(connection->client);
	g_signal_connect_object(capabilities, "ready",
	                        G_CALLBACK(purple_ircv3_connection_capabilities_ready_cb),
	                        connection, G_CONNECT_DEFAULT);

	resolver = purple_proxy_get_proxy_resolver(account, &local_error);
	if(local_error != NULL) {
		g_propagate_error(error, local_error);

		g_clear_object(&resolver);
		g_clear_object(&connection->client);

		return FALSE;
	}

	ibis_client_connect(connection->client, connection->server_name, port,
	                    password, tls, cancellable, resolver);

	return TRUE;
}

static gboolean
purple_ircv3_connection_disconnect(PurpleConnection *purple_connection,
                                   G_GNUC_UNUSED GError **error)
{
	PurpleIRCv3Connection *connection = NULL;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(purple_connection), FALSE);

	connection = PURPLE_IRCV3_CONNECTION(purple_connection);

	/* TODO: send QUIT command. */

	g_clear_object(&connection->client);

	return TRUE;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_ircv3_connection_get_property(GObject *obj, guint param_id,
                                     GValue *value, GParamSpec *pspec)
{
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);

	switch(param_id) {
	case PROP_CLIENT:
		g_value_set_object(value,
		                   purple_ircv3_connection_get_client(connection));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_ircv3_connection_dispose(GObject *obj) {
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);

	g_clear_object(&connection->client);
	g_clear_object(&connection->status_conversation);

	G_OBJECT_CLASS(purple_ircv3_connection_parent_class)->dispose(obj);
}

static void
purple_ircv3_connection_finalize(GObject *obj) {
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);

	g_clear_pointer(&connection->server_name, g_free);

	G_OBJECT_CLASS(purple_ircv3_connection_parent_class)->finalize(obj);
}

static void
purple_ircv3_connection_constructed(GObject *obj) {
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);
	PurpleAccount *account = NULL;
	PurpleConversationManager *conversation_manager = NULL;
	char **userparts = NULL;
	const char *username = NULL;
	char *title = NULL;

	G_OBJECT_CLASS(purple_ircv3_connection_parent_class)->constructed(obj);

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));

	title = g_strdup_printf(_("status for %s"),
	                        purple_account_get_username(account));

	/* Split the username into nick and server and store the values. */
	username = purple_account_get_username(account);
	userparts = g_strsplit(username, "@", 2);
	purple_connection_set_display_name(PURPLE_CONNECTION(connection),
	                                   userparts[0]);
	connection->server_name = g_strdup(userparts[1]);

	/* Free the userparts vector. */
	g_strfreev(userparts);

	/* Check if we have an existing status conversation. */
	conversation_manager = purple_conversation_manager_get_default();
	connection->status_conversation = purple_conversation_manager_find_with_id(conversation_manager,
	                                                                           account,
	                                                                           connection->server_name);

	if(!PURPLE_IS_CONVERSATION(connection->status_conversation)) {
		/* Create our status conversation. */
		connection->status_conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"id", connection->server_name,
			"name", connection->server_name,
			"title", title,
			NULL);

			purple_conversation_manager_register(conversation_manager,
			                                     connection->status_conversation);
	} else {
		/* The conversation existed, so add a reference to it. */
		g_object_ref(connection->status_conversation);
	}

	g_clear_pointer(&title, g_free);
}

static void
purple_ircv3_connection_init(G_GNUC_UNUSED PurpleIRCv3Connection *connection) {
}

static void
purple_ircv3_connection_class_finalize(G_GNUC_UNUSED PurpleIRCv3ConnectionClass *klass) {
}

static void
purple_ircv3_connection_class_init(PurpleIRCv3ConnectionClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	PurpleConnectionClass *connection_class = PURPLE_CONNECTION_CLASS(klass);

	obj_class->get_property = purple_ircv3_connection_get_property;
	obj_class->constructed = purple_ircv3_connection_constructed;
	obj_class->dispose = purple_ircv3_connection_dispose;
	obj_class->finalize = purple_ircv3_connection_finalize;

	connection_class->connect = purple_ircv3_connection_connect;
	connection_class->disconnect = purple_ircv3_connection_disconnect;

	/**
	 * PurpleIRCv3Connection:client:
	 *
	 * The [class@Ibis.Client] that this connection is using.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CLIENT] = g_param_spec_object(
		"client", NULL, NULL,
		IBIS_TYPE_CLIENT,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_ircv3_connection_register(GPluginNativePlugin *plugin) {
	purple_ircv3_connection_register_type(G_TYPE_MODULE(plugin));
}

/******************************************************************************
 * Public API
 *****************************************************************************/
IbisClient *
purple_ircv3_connection_get_client(PurpleIRCv3Connection *connection) {
	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), NULL);

	return connection->client;
}

void
purple_ircv3_connection_add_status_message(PurpleIRCv3Connection *connection,
                                           IbisMessage *ibis_message)
{
	PurpleContact *author = NULL;
	PurpleMessage *purple_message = NULL;
	GString *str = NULL;
	GStrv params = NULL;
	char *stripped = NULL;
	const char *command = NULL;

	g_return_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection));
	g_return_if_fail(IBIS_IS_MESSAGE(ibis_message));

	command = ibis_message_get_command(ibis_message);

	str = g_string_new(command);

	params = ibis_message_get_params(ibis_message);
	if(params != NULL && params[0] != NULL) {
		char *joined = g_strjoinv(" ", params);

		g_string_append_printf(str, " %s", joined);

		g_free(joined);
	}

	stripped = ibis_formatting_strip(str->str);
	g_string_free(str, TRUE);

	author = purple_ircv3_connection_find_or_create_contact(connection,
	                                                        ibis_message);
	purple_message = purple_message_new(PURPLE_CONTACT_INFO(author), stripped);
	g_free(stripped);

	purple_conversation_write_message(connection->status_conversation,
	                                  purple_message);

	g_clear_object(&purple_message);
}

PurpleConversation *
purple_ircv3_connection_find_or_create_conversation(PurpleIRCv3Connection *connection,
                                                    const char *id)
{
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), NULL);
	g_return_val_if_fail(id != NULL, NULL);

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	manager = purple_conversation_manager_get_default();
	conversation = purple_conversation_manager_find_with_id(manager, account,
	                                                        id);

	if(!PURPLE_IS_CONVERSATION(conversation)) {
		PurpleConversationType type = PURPLE_CONVERSATION_TYPE_DM;

		if(ibis_client_is_channel(connection->client, id)) {
			type = PURPLE_CONVERSATION_TYPE_CHANNEL;
		}

		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"id", id,
			"name", id,
			"title", id,
			"type", type,
			NULL);

		purple_conversation_manager_register(manager, conversation);

		/* The manager creates its own reference on our new conversation, so we
		 * borrow it like we do above if it already exists.
		 */
		g_object_unref(conversation);
	}

	return conversation;
}

PurpleContact *
purple_ircv3_connection_find_or_create_contact(PurpleIRCv3Connection *connection,
                                               IbisMessage *message)
{
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleContactManager *manager = NULL;
	IbisTags *tags = NULL;
	const char *source = NULL;
	char *nick = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	manager = purple_contact_manager_get_default();

	tags = ibis_message_get_tags(message);
	if(IBIS_IS_TAGS(tags)) {
		const char *account_tag = NULL;

		account_tag = ibis_tags_lookup(tags, IBIS_TAG_ACCOUNT);
		if(!purple_strempty(account_tag)) {
			contact = purple_contact_manager_find_with_id(manager, account,
			                                              account_tag);
		}
	}

	source = ibis_message_get_source(message);
	ibis_source_parse(source, &nick, NULL, NULL);

	/* If we don't have a contact yet, use the source (Luke) to search next. */
	if(!PURPLE_IS_CONTACT(contact)) {
		contact = purple_contact_manager_find_with_id(manager, account, nick);
	}

	/* If we _still_ don't have a contact, create it. */
	if(!PURPLE_IS_CONTACT(contact)) {
		contact = purple_contact_new(account, nick);
		purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact), nick);

		purple_contact_manager_add(manager, contact);
	}

	purple_contact_info_set_sid(PURPLE_CONTACT_INFO(contact), source);

	g_free(nick);

	return contact;
}

PurpleConversation *
purple_ircv3_connection_get_status_conversation(PurpleIRCv3Connection *connection)
{
	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), NULL);

	return connection->status_conversation;
}
