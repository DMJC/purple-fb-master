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

typedef struct {
	IbisClient *client;

	gchar *server_name;

	PurpleConversation *status_conversation;
} PurpleIRCv3ConnectionPrivate;

G_DEFINE_DYNAMIC_TYPE_EXTENDED(PurpleIRCv3Connection,
                               purple_ircv3_connection,
                               PURPLE_TYPE_CONNECTION,
                               0,
                               G_ADD_PRIVATE_DYNAMIC(PurpleIRCv3Connection))

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
purple_ircv3_connection_add_message_handlers(PurpleIRCv3Connection *connection,
                                             IbisClient *client)
{
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

	g_signal_connect_object(client, "message::" IBIS_MSG_JOIN,
	                        G_CALLBACK(purple_ircv3_message_handler_join),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(client, "message::" IBIS_MSG_PART,
	                        G_CALLBACK(purple_ircv3_message_handler_part),
	                        connection, G_CONNECT_DEFAULT);

	g_signal_connect_object(client, "message",
	                        G_CALLBACK(purple_ircv3_connection_unknown_message_cb),
	                        connection, G_CONNECT_AFTER);
}

static void
purple_ircv3_connection_rejoin_channels(PurpleIRCv3Connection *connection) {
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleAccount *account = NULL;
	PurpleConversationManager *manager = NULL;
	GList *conversations = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

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
			ibis_client_write(priv->client, message);
		}

		conversations = g_list_delete_link(conversations, conversations);
	}
}

static inline void
purple_ircv3_connection_setup_sasl(PurpleIRCv3Connection *connection,
                                   PurpleAccount *account)
{
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	HaslContext *hasl_context = NULL;
	const char *value = NULL;
	gboolean clear_text = FALSE;

	priv = purple_ircv3_connection_get_instance_private(connection);

	hasl_context = hasl_context_new();

	value = purple_account_get_string(account, "sasl-login-name", NULL);
	if(!purple_strempty(value)) {
		hasl_context_set_username(hasl_context, value);
	} else {
		hasl_context_set_username(hasl_context,
		                          ibis_client_get_nick(priv->client));

		/* Since the user doesn't have a SASL login name set, we'll listen for
		 * IBIS_RPL_SASLSUCCESS and use that to set the login name to the
		 * username in HASL which worked if the signal handler gets called.
		 */
		g_signal_connect_object(priv->client, "message::" IBIS_RPL_SASLSUCCESS,
		                        G_CALLBACK(purple_ircv3_connection_saslsuccess),
		                        connection, G_CONNECT_DEFAULT);
	}

	value = purple_connection_get_password(PURPLE_CONNECTION(connection));
	hasl_context_set_password(hasl_context, value);

	value = purple_account_get_string(account, "sasl-mechanisms", NULL);
	if(!purple_strempty(value)) {
		hasl_context_set_allowed_mechanisms(hasl_context, value);
	}

	clear_text = purple_account_get_bool(account, "plain-sasl-in-clear", FALSE);
	hasl_context_set_allow_clear_text(hasl_context, clear_text);

	ibis_client_set_hasl_context(priv->client, hasl_context);
	g_clear_object(&hasl_context);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
purple_ircv3_connection_unknown_message_cb(G_GNUC_UNUSED IbisClient *client,
                                           G_GNUC_UNUSED const char *command,
                                           IbisMessage *message,
                                           gpointer data)
{
	PurpleIRCv3Connection *connection = data;
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleMessage *purple_message = NULL;
	char *contents = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

	contents = g_strdup_printf(_("unhandled message: '%s'"),
	                           ibis_message_get_raw_message(message));

	purple_message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", ibis_message_get_source(message),
		"contents", contents,
		NULL);

	purple_conversation_write_message(priv->status_conversation,
	                                  purple_message);
	g_clear_object(&purple_message);

	g_free(contents);

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
	GError *local_error = NULL;
	GProxyResolver *resolver = NULL;
	const char *password = NULL;
	const char *value = NULL;
	gint default_port = PURPLE_IRCV3_DEFAULT_TLS_PORT;
	gint port = 0;
	gboolean tls = TRUE;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(purple_connection), FALSE);

	connection = PURPLE_IRCV3_CONNECTION(purple_connection);
	priv = purple_ircv3_connection_get_instance_private(connection);
	account = purple_connection_get_account(purple_connection);

	priv->client = ibis_client_new();
	g_signal_connect_object(priv->client, "notify::connected",
	                        G_CALLBACK(purple_ircv3_connection_connect_cb),
	                        connection, G_CONNECT_DEFAULT);
	g_signal_connect_object(priv->client, "notify::error",
	                        G_CALLBACK(purple_ircv3_connection_error_cb),
	                        connection, G_CONNECT_DEFAULT);
	purple_ircv3_connection_add_message_handlers(connection, priv->client);

	ibis_client_set_nick(priv->client,
	                     purple_connection_get_display_name(purple_connection));

	value = purple_account_get_string(account, "ident", NULL);
	ibis_client_set_username(priv->client, value);

	value = purple_account_get_string(account, "real-name", NULL);
	ibis_client_set_realname(priv->client, value);

	password = purple_account_get_string(account, "server-password", NULL);

	/* Turn on TLS if requested. */
	tls = purple_account_get_bool(account, "use-tls", TRUE);

	/* If TLS is not being used, set the default port to the plain port. */
	if(!tls) {
		default_port = PURPLE_IRCV3_DEFAULT_PLAIN_PORT;
	}
	port = purple_account_get_int(account, "port", default_port);

	purple_ircv3_connection_setup_sasl(connection, account);

	cancellable = purple_connection_get_cancellable(purple_connection);

	resolver = purple_proxy_get_proxy_resolver(account, &local_error);
	if(local_error != NULL) {
		g_propagate_error(error, local_error);

		g_clear_object(&resolver);
		g_clear_object(&priv->client);

		return FALSE;
	}

	ibis_client_connect(priv->client, priv->server_name, port, password, tls,
	                    cancellable, resolver);

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

	g_clear_object(&priv->client);

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
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	priv = purple_ircv3_connection_get_instance_private(connection);

	g_clear_object(&priv->client);
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

/******************************************************************************
 * Public API
 *****************************************************************************/
IbisClient *
purple_ircv3_connection_get_client(PurpleIRCv3Connection *connection) {
	PurpleIRCv3ConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), NULL);

	priv = purple_ircv3_connection_get_instance_private(connection);

	return priv->client;
}

void
purple_ircv3_connection_add_status_message(PurpleIRCv3Connection *connection,
                                           IbisMessage *ibis_message)
{
	PurpleIRCv3ConnectionPrivate *priv = NULL;
	PurpleMessage *message = NULL;
	GString *str = NULL;
	GStrv params = NULL;
	char *stripped = NULL;
	const char *command = NULL;

	g_return_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection));
	g_return_if_fail(IBIS_IS_MESSAGE(ibis_message));

	priv = purple_ircv3_connection_get_instance_private(connection);

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

	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", ibis_message_get_source(ibis_message),
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
		PurpleConversationType type = PURPLE_CONVERSATION_TYPE_DM;

		if(purple_ircv3_connection_is_channel(connection, id)) {
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
