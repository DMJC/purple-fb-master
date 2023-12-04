/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpleircv3connection.h"

#include "purpleircv3constants.h"
#include "purpleircv3core.h"
#include "purpleircv3formatting.h"
#include "purpleircv3parser.h"

enum {
	PROP_0,
	PROP_CAPABILITIES,
	PROP_REGISTERED,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

enum {
	SIG_REGISTRATION_COMPLETE,
	SIG_CTCP_REQUEST,
	SIG_CTCP_RESPONSE,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

typedef struct {
	GSocketConnection *connection;

	gchar *server_name;
	gboolean registered;

	GDataInputStream *input;
	PurpleQueuedOutputStream *output;

	PurpleIRCv3Parser *parser;

	PurpleIRCv3Capabilities *capabilities;

	PurpleConversation *status_conversation;
} PurpleIRCv3ConnectionPrivate;

G_DEFINE_DYNAMIC_TYPE_EXTENDED(PurpleIRCv3Connection,
                               purple_ircv3_connection,
                               PURPLE_TYPE_CONNECTION,
                               0,
                               G_ADD_PRIVATE_DYNAMIC(PurpleIRCv3Connection))

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_ircv3_connection_send_pass_command(PurpleIRCv3Connection *connection) {
	PurpleAccount *account = NULL;
	const char *password = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));

	password = purple_account_get_string(account, "server-password", "");
	if(password != NULL && *password != '\0') {
		purple_ircv3_connection_writef(connection, "PASS %s", password);
	}
}

static void
purple_ircv3_connection_send_user_command(PurpleIRCv3Connection *connection) {
	PurpleAccount *account = NULL;
	const char *identname = NULL;
	const char *nickname = NULL;
	const char *realname = NULL;

	nickname =
		purple_connection_get_display_name(PURPLE_CONNECTION(connection));

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));

	/* The stored value could be an empty string, so pass a default of empty
	 * string and then if it was empty, set our correct fallback.
	 */
	identname = purple_account_get_string(account, "ident", "");
	if(identname == NULL || *identname == '\0') {
		identname = nickname;
	}

	realname = purple_account_get_string(account, "real-name", "");
	if(realname == NULL || *realname == '\0') {
		realname = nickname;
	}

	purple_ircv3_connection_writef(connection, "USER %s 0 * :%s", identname,
	                               realname);
}

static void
purple_ircv3_connection_send_nick_command(PurpleIRCv3Connection *connection) {
	const char *nickname = NULL;

	nickname =
		purple_connection_get_display_name(PURPLE_CONNECTION(connection));

	purple_ircv3_connection_writef(connection, "NICK %s", nickname);
}

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
			const char *id = purple_conversation_get_id(conversation);

			purple_ircv3_connection_writef(connection, "%s %s",
			                               PURPLE_IRCV3_MSG_JOIN, id);
		}

		conversations = g_list_delete_link(conversations, conversations);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_ircv3_connection_read_cb(GObject *source, GAsyncResult *result,
                                gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	GCancellable *cancellable = NULL;
	GDataInputStream *istream = G_DATA_INPUT_STREAM(source);
	GError *error = NULL;
	gchar *line = NULL;
	gsize length;
	gboolean parsed = FALSE;

	line = g_data_input_stream_read_line_finish(istream, result, &length,
	                                            &error);
	if(line == NULL || error != NULL) {
		if(PURPLE_IS_CONNECTION(connection)) {
			if(error == NULL) {
				g_set_error_literal(&error, PURPLE_CONNECTION_ERROR,
				                    PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
				                    _("Server closed the connection"));
			} else {
				g_prefix_error(&error, "%s", _("Lost connection with server: "));
			}

			purple_connection_take_error(PURPLE_CONNECTION(connection), error);
		}

		/* In the off chance that line was returned, make sure we free it. */
		g_free(line);

		return;
	}

	priv = purple_ircv3_connection_get_instance_private(connection);

	parsed = purple_ircv3_parser_parse(priv->parser, line, &error,
	                                   connection);
	if(!parsed) {
		g_warning("failed to handle '%s': %s", line,
		          error != NULL ? error->message : "unknown error");
	}
	g_clear_error(&error);

	g_free(line);

	/* Call read_line_async again to continue reading lines. */
	cancellable = purple_connection_get_cancellable(PURPLE_CONNECTION(connection));
	g_data_input_stream_read_line_async(priv->input,
	                                    G_PRIORITY_DEFAULT,
	                                    cancellable,
	                                    purple_ircv3_connection_read_cb,
	                                    connection);
}

static void
purple_ircv3_connection_write_cb(GObject *source, GAsyncResult *result,
                                 gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleQueuedOutputStream *stream = PURPLE_QUEUED_OUTPUT_STREAM(source);
	GError *error = NULL;
	gboolean success = FALSE;

	success = purple_queued_output_stream_push_bytes_finish(stream, result,
	                                                        &error);

	if(!success) {
		purple_queued_output_stream_clear_queue(stream);

		g_prefix_error(&error, "%s", _("Lost connection with server: "));

		purple_connection_take_error(PURPLE_CONNECTION(connection), error);

		return;
	}
}

static void
purple_ircv3_connection_connected_cb(GObject *source, GAsyncResult *result,
                                     gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	GCancellable *cancellable = NULL;
	GError *error = NULL;
	GInputStream *istream = NULL;
	GOutputStream *ostream = NULL;
	GSocketClient *client = G_SOCKET_CLIENT(source);
	GSocketConnection *conn = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

	/* Finish the async method. */
	conn = g_socket_client_connect_to_host_finish(client, result, &error);
	if(conn == NULL || error != NULL) {
		g_prefix_error(&error, "%s", _("Unable to connect: "));

		purple_connection_take_error(PURPLE_CONNECTION(connection), error);

		return;
	}

	g_message("Successfully connected to %s", priv->server_name);

	/* Save our connection and setup our input and outputs. */
	priv->connection = conn;

	/* Create our parser. */
	priv->parser = purple_ircv3_parser_new();
	purple_ircv3_parser_add_default_handlers(priv->parser);

	ostream = g_io_stream_get_output_stream(G_IO_STREAM(conn));
	priv->output = purple_queued_output_stream_new(ostream);

	istream = g_io_stream_get_input_stream(G_IO_STREAM(conn));
	priv->input = g_data_input_stream_new(istream);
	g_data_input_stream_set_newline_type(G_DATA_INPUT_STREAM(priv->input),
	                                     G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

	cancellable = purple_connection_get_cancellable(PURPLE_CONNECTION(connection));

	/* Add our read callback. */
	g_data_input_stream_read_line_async(priv->input,
	                                    G_PRIORITY_DEFAULT,
	                                    cancellable,
	                                    purple_ircv3_connection_read_cb,
	                                    connection);

	/* Send our registration commands. */
	purple_ircv3_capabilities_start(priv->capabilities);
	purple_ircv3_connection_send_pass_command(connection);
	purple_ircv3_connection_send_user_command(connection);
	purple_ircv3_connection_send_nick_command(connection);
}

static void
purple_ircv3_connection_caps_done_cb(G_GNUC_UNUSED PurpleIRCv3Capabilities *caps,
                                     gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

	priv->registered = TRUE;

	g_signal_emit(connection, signals[SIG_REGISTRATION_COMPLETE], 0);

	/* Now that registration is complete, rejoin any channels that the
	 * conversation manager has for us.
	 */
	purple_ircv3_connection_rejoin_channels(connection);
}

/******************************************************************************
 * PurpleConnection Implementation
 *****************************************************************************/
static gboolean
purple_ircv3_connection_connect(PurpleConnection *purple_connection,
                                GError **error)
{
	PurpleIRCv3Connection *connection = NULL;
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleAccount *account = NULL;
	GCancellable *cancellable = NULL;
	GSocketClient *client = NULL;
	gint default_port = PURPLE_IRCV3_DEFAULT_TLS_PORT;
	gint port = 0;
	gboolean use_tls = TRUE;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(purple_connection), FALSE);

	connection = PURPLE_IRCV3_CONNECTION(purple_connection);
	priv = purple_ircv3_connection_get_instance_private(connection);
	account = purple_connection_get_account(purple_connection);

	client = purple_gio_socket_client_new(account, error);
	if(!G_IS_SOCKET_CLIENT(client)) {
		if(error != NULL && *error != NULL) {
			purple_connection_take_error(purple_connection, *error);
		}

		return FALSE;
	}

	/* Turn on TLS if requested. */
	use_tls = purple_account_get_bool(account, "use-tls", TRUE);
	g_socket_client_set_tls(client, use_tls);

	/* If TLS is not being used, set the default port to the plain port. */
	if(!use_tls) {
		default_port = PURPLE_IRCV3_DEFAULT_PLAIN_PORT;
	}
	port = purple_account_get_int(account, "port", default_port);

	cancellable = purple_connection_get_cancellable(purple_connection);

	/* Finally start the async connection. */
	g_socket_client_connect_to_host_async(client, priv->server_name,
	                                      port, cancellable,
	                                      purple_ircv3_connection_connected_cb,
	                                      connection);

	g_clear_object(&client);

	return TRUE;
}

static gboolean
purple_ircv3_connection_disconnect(PurpleConnection *purple_connection,
                                   GError **error)
{
	PurpleIRCv3Connection *connection = NULL;
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleConnectionClass *parent_class = NULL;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(purple_connection), FALSE);

	/* Chain up to our parent's disconnect to do initial clean up. */
	parent_class = PURPLE_CONNECTION_CLASS(purple_ircv3_connection_parent_class);
	parent_class->disconnect(purple_connection, error);

	connection = PURPLE_IRCV3_CONNECTION(purple_connection);
	priv = purple_ircv3_connection_get_instance_private(connection);

	/* TODO: send QUIT command. */

	if(G_IS_SOCKET_CONNECTION(priv->connection)) {
		GInputStream *istream = G_INPUT_STREAM(priv->input);
		GOutputStream *ostream = G_OUTPUT_STREAM(priv->output);

		purple_gio_graceful_close(G_IO_STREAM(priv->connection),
		                          istream, ostream);
	}

	g_clear_object(&priv->input);
	g_clear_object(&priv->output);
	g_clear_object(&priv->connection);

	return TRUE;
}

static void
purple_ircv3_connection_registration_complete_cb(PurpleIRCv3Connection *connection) {
	/* Don't set our connection state to connected until we've completed
	 * registration as connected implies that we can start chatting or join
	 * rooms and other "online" activities.
	 */
	purple_connection_set_state(PURPLE_CONNECTION(connection),
	                            PURPLE_CONNECTION_STATE_CONNECTED);
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
		case PROP_CAPABILITIES:
			g_value_set_object(value,
			                   purple_ircv3_connection_get_capabilities(connection));
			break;
		case PROP_REGISTERED:
			g_value_set_boolean(value,
			                    purple_ircv3_connection_get_registered(connection));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_ircv3_connection_dispose(GObject *obj) {
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

	g_clear_object(&priv->input);
	g_clear_object(&priv->output);
	g_clear_object(&priv->connection);

	g_clear_object(&priv->capabilities);
	g_clear_object(&priv->parser);
	g_clear_object(&priv->status_conversation);

	G_OBJECT_CLASS(purple_ircv3_connection_parent_class)->dispose(obj);
}

static void
purple_ircv3_connection_finalize(GObject *obj) {
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

	g_clear_pointer(&priv->server_name, g_free);

	G_OBJECT_CLASS(purple_ircv3_connection_parent_class)->finalize(obj);
}

static void
purple_ircv3_connection_constructed(GObject *obj) {
	PurpleIRCv3Connection *connection = PURPLE_IRCV3_CONNECTION(obj);
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleAccount *account = NULL;
	PurpleContactInfo *info = NULL;
	PurpleConversationManager *conversation_manager = NULL;
	char **userparts = NULL;
	const char *username = NULL;
	char *title = NULL;

	G_OBJECT_CLASS(purple_ircv3_connection_parent_class)->constructed(obj);

	priv = purple_ircv3_connection_get_instance_private(connection);
	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	info = PURPLE_CONTACT_INFO(account);

	title = g_strdup_printf(_("status for %s"),
	                        purple_contact_info_get_username(info));

	/* Split the username into nick and server and store the values. */
	username = purple_contact_info_get_username(PURPLE_CONTACT_INFO(account));
	userparts = g_strsplit(username, "@", 2);
	purple_connection_set_display_name(PURPLE_CONNECTION(connection),
	                                   userparts[0]);
	priv->server_name = g_strdup(userparts[1]);

	/* Free the userparts vector. */
	g_strfreev(userparts);

	/* Check if we have an existing status conversation. */
	conversation_manager = purple_conversation_manager_get_default();
	priv->status_conversation = purple_conversation_manager_find_with_id(conversation_manager,
	                                                                     account,
	                                                                     priv->server_name);

	if(!PURPLE_IS_CONVERSATION(priv->status_conversation)) {
		/* Create our status conversation. */
		priv->status_conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"id", priv->server_name,
			"name", priv->server_name,
			"title", title,
			NULL);

			purple_conversation_manager_register(conversation_manager,
			                                     priv->status_conversation);
	} else {
		/* The conversation existed, so add a reference to it. */
		g_object_ref(priv->status_conversation);
	}

	g_clear_pointer(&title, g_free);

	/* Finally create our objects. */
	priv->capabilities = purple_ircv3_capabilities_new(connection);
	g_signal_connect_object(priv->capabilities, "done",
	                        G_CALLBACK(purple_ircv3_connection_caps_done_cb),
	                        connection, 0);
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
	 * PurpleIRCv3Connection:capabilities:
	 *
	 * The capabilities that the server supports.
	 *
	 * This is created during registration of the connection and is useful for
	 * troubleshooting or just reporting them to end users.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_CAPABILITIES] = g_param_spec_object(
		"capabilities", "capabilities",
		"The capabilities that the server supports",
		PURPLE_IRCV3_TYPE_CAPABILITIES,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleIRCv3Connection:registered:
	 *
	 * Whether or not the connection has finished the registration portion of
	 * the connection.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_REGISTERED] = g_param_spec_boolean(
		"registered", "registered",
		"Whether or not the connection has finished registration.",
		FALSE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/* Signals */

	/**
	 * PurpleIRCv3Connection::registration-complete:
	 * @connection: The instance.
	 *
	 * This signal is emitted after the registration process has been
	 * completed. Plugins can use this to perform additional actions before
	 * any channels are auto joined or similar.
	 *
	 * Since: 3.0.0
	 */
	signals[SIG_REGISTRATION_COMPLETE] = g_signal_new_class_handler(
		"registration-complete",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		G_CALLBACK(purple_ircv3_connection_registration_complete_cb),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0);

	/**
	 * PurpleIRCv3Connection::ctcp-request:
	 * @connection: The instance.
	 * @command: The CTCP command.
	 * @params: (nullable): The CTCP parameters.
	 *
	 * This signal is emitted after a CTCP request has been received.
	 *
	 * Since: 3.0.0
	 */
	signals[SIG_CTCP_REQUEST] = g_signal_new_class_handler(
		"ctcp-request",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		G_TYPE_STRING,
		G_TYPE_STRING);

	/**
	 * PurpleIRCv3Connection::ctcp-response:
	 * @connection: The instance.
	 * @command: The CTCP command.
	 * @params: (nullable): The CTCP parameters.
	 *
	 * This signal is emitted after a CTCP response has been received.
	 *
	 * Since: 3.0.0
	 */
	signals[SIG_CTCP_RESPONSE] = g_signal_new_class_handler(
		"ctcp-response",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		G_TYPE_STRING,
		G_TYPE_STRING);
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_ircv3_connection_register(GPluginNativePlugin *plugin) {
	GObjectClass *hack = NULL;

	purple_ircv3_connection_register_type(G_TYPE_MODULE(plugin));

	/* Without this hack we get some warnings about no reference on this type
	 * when generating the gir file. However, there are no changes to the
	 * generated gir file with or without this hack, so this is purely to get
	 * rid of the warnings.
	 *
	 * - GK 2023-08-21
	 */
	hack = g_type_class_ref(purple_ircv3_connection_get_type());
	g_type_class_unref(hack);
}

void
purple_ircv3_connection_emit_ctcp_request(PurpleIRCv3Connection *connection,
                                          const char *command,
                                          const char *parameters)
{
	g_return_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection));
	g_return_if_fail(!purple_strempty(command));

	g_signal_emit(connection, signals[SIG_CTCP_REQUEST], 0, command,
	              parameters);
}

void
purple_ircv3_connection_emit_ctcp_response(PurpleIRCv3Connection *connection,
                                           const char *command,
                                           const char *parameters)
{
	g_return_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection));
	g_return_if_fail(!purple_strempty(command));

	g_signal_emit(connection, signals[SIG_CTCP_RESPONSE], 0, command,
	              parameters);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
void
purple_ircv3_connection_writef(PurpleIRCv3Connection *connection,
                               const char *format, ...)
{
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	GBytes *bytes = NULL;
	GCancellable *cancellable = NULL;
	GString *msg = NULL;
	va_list vargs;

	g_return_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection));
	g_return_if_fail(format != NULL);

	priv = purple_ircv3_connection_get_instance_private(connection);

	/* Create our string and append our format to it. */
	msg = g_string_new("");

	va_start(vargs, format);
	g_string_vprintf(msg, format, vargs);
	va_end(vargs);

	/* Next add the trailing carriage return line feed. */
	g_string_append(msg, "\r\n");

	/* Finally turn the string into bytes and send it! */
	bytes = g_string_free_to_bytes(msg);

	cancellable = purple_connection_get_cancellable(PURPLE_CONNECTION(connection));
	purple_queued_output_stream_push_bytes_async(priv->output, bytes,
	                                             G_PRIORITY_DEFAULT,
	                                             cancellable,
	                                             purple_ircv3_connection_write_cb,
	                                             connection);

	g_bytes_unref(bytes);
}

PurpleIRCv3Capabilities *
purple_ircv3_connection_get_capabilities(PurpleIRCv3Connection *connection) {
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), NULL);

	priv = purple_ircv3_connection_get_instance_private(connection);

	return priv->capabilities;
}

gboolean
purple_ircv3_connection_get_registered(PurpleIRCv3Connection *connection) {
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), FALSE);

	priv = purple_ircv3_connection_get_instance_private(connection);

	return priv->registered;
}

void
purple_ircv3_connection_add_status_message(PurpleIRCv3Connection *connection,
                                           PurpleIRCv3Message *v3_message)
{
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleMessage *message = NULL;
	GString *str = NULL;
	GStrv params = NULL;
	char *stripped = NULL;
	const char *command = NULL;

	g_return_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection));
	g_return_if_fail(PURPLE_IRCV3_IS_MESSAGE(v3_message));

	priv = purple_ircv3_connection_get_instance_private(connection);

	command = purple_ircv3_message_get_command(v3_message);

	str = g_string_new(command);

	params = purple_ircv3_message_get_params(v3_message);
	if(params != NULL && params[0] != NULL) {
		char *joined = g_strjoinv(" ", params);

		g_string_append_printf(str, " %s", joined);

		g_free(joined);
	}

	stripped = purple_ircv3_formatting_strip(str->str);
	g_string_free(str, TRUE);

	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", purple_ircv3_message_get_source(v3_message),
		"contents", stripped,
		NULL);
	g_free(stripped);

	purple_conversation_write_message(priv->status_conversation, message);

	g_clear_object(&message);
}

gboolean
purple_ircv3_connection_is_channel(PurpleIRCv3Connection *connection,
                                   const char *id)
{
	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);

	return (id[0] == '#');
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
		PurpleConversationType type = PurpleConversationTypeDM;

		if(purple_ircv3_connection_is_channel(connection, id)) {
			type = PurpleConversationTypeChannel;
		}

		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"id", id,
			"name", id,
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
                                               const char *nick)
{
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleContactManager *manager = NULL;

	account = purple_connection_get_account(PURPLE_CONNECTION(connection));
	manager = purple_contact_manager_get_default();
	contact = purple_contact_manager_find_with_id(manager, account, nick);

	if(!PURPLE_IS_CONTACT(contact)) {
		contact = purple_contact_new(account, nick);
		purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact), nick);

		purple_contact_manager_add(manager, contact);
	}

	return contact;
}
