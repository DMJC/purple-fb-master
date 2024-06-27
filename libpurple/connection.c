/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "connection.h"
#include "debug.h"
#include "notify.h"
#include "prefs.h"
#include "proxy.h"
#include "purpleaccount.h"
#include "purpleenums.h"
#include "purpleprivate.h"
#include "purpleprotocolserver.h"
#include "request.h"
#include "signals.h"
#include "util.h"

/**
 * PurpleConnection:
 *
 * Represents an active connection on an account.
 */
typedef struct {
	GObject gparent;

	gchar *id;

	GCancellable *cancellable;

	PurpleProtocol *protocol;     /* The protocol.                     */
	PurpleConnectionFlags flags;  /* Connection flags.                 */

	PurpleConnectionState state;  /* The connection state.             */

	PurpleAccount *account;       /* The account being connected to.   */
	char *password;               /* The password used.                */

	char *display_name;           /* How you appear to other people.   */

	/* Wants to Die state.  This is set when the user chooses to log out, or
	 * when the protocol is disconnected and should not be automatically
	 * reconnected (incorrect password, etc.).  Protocols should rely on
	 * purple_connection_error() to set this for them rather than
	 * setting it themselves.
	 * See purple_connection_error_is_fatal()
	 */
	gboolean wants_to_die;

	/* The connection error and its description if an error occurred. */
	PurpleConnectionErrorInfo *error_info;

	guint disconnect_timeout;  /* Timer used for nasty stack tricks. */
} PurpleConnectionPrivate;

enum {
	PROP_0,
	PROP_ID,
	PROP_CANCELLABLE,
	PROP_PROTOCOL,
	PROP_FLAGS,
	PROP_STATE,
	PROP_ACCOUNT,
	PROP_PASSWORD,
	PROP_DISPLAY_NAME,
	N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES] = {NULL, };

static GList *connections = NULL;
static GList *connections_connected = NULL;

static int connections_handle;

G_DEFINE_TYPE_WITH_PRIVATE(PurpleConnection, purple_connection, G_TYPE_OBJECT)

/**************************************************************************
 * Connection API
 **************************************************************************/

/*
 * d:)->-<
 *
 * d:O-\-<
 *
 * d:D-/-<
 *
 * d8D->-< DANCE!
 */

void
purple_connection_set_state(PurpleConnection *connection,
                            PurpleConnectionState state)
{
	PurpleConnectionPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONNECTION(connection));

	priv = purple_connection_get_instance_private(connection);

	if(priv->state == state) {
		return;
	}

	priv->state = state;

	if(priv->state == PURPLE_CONNECTION_STATE_CONNECTED) {
		PurplePresence *presence;
		GDateTime *timestamp = NULL;
		gboolean emit_online = FALSE;
		gpointer handle = NULL;

		presence = purple_account_get_presence(priv->account);

		/* Set the time the account came online */
		timestamp = g_date_time_new_now_utc();
		purple_presence_set_login_time(presence, timestamp);
		g_date_time_unref(timestamp);

		handle = purple_connections_get_handle();
		purple_signal_emit(handle, "signed-on", connection);
		purple_signal_emit_return_1(handle, "autojoin", connection);

		/* check if connections_connected is NULL, if so we need to emit the
		 * online signal.
		 */
		if(connections_connected == NULL) {
			emit_online = TRUE;
		}

		connections_connected = g_list_append(connections_connected,
		                                      connection);

		if(emit_online) {
			purple_signal_emit(handle, "online");
		}
	}

	g_object_notify_by_pspec(G_OBJECT(connection), properties[PROP_STATE]);
}

void
purple_connection_set_flags(PurpleConnection *connection,
                            PurpleConnectionFlags flags)
{
	PurpleConnectionPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONNECTION(connection));

	priv = purple_connection_get_instance_private(connection);

	priv->flags = flags;

	g_object_notify_by_pspec(G_OBJECT(connection), properties[PROP_FLAGS]);
}

void
purple_connection_set_display_name(PurpleConnection *connection,
                                   const gchar *name)
{
	PurpleConnectionPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONNECTION(connection));

	priv = purple_connection_get_instance_private(connection);

	if(g_set_str(&priv->display_name, name)) {
		g_object_notify_by_pspec(G_OBJECT(connection),
		                         properties[PROP_DISPLAY_NAME]);
	}
}

PurpleConnectionState
purple_connection_get_state(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection),
	                     PURPLE_CONNECTION_STATE_DISCONNECTED);

	priv = purple_connection_get_instance_private(connection);

	return priv->state;
}

PurpleConnectionFlags
purple_connection_get_flags(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), 0);

	priv = purple_connection_get_instance_private(connection);

	return priv->flags;
}

PurpleAccount *
purple_connection_get_account(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->account;
}

const gchar *
purple_connection_get_id(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->id;
}

PurpleProtocol *
purple_connection_get_protocol(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->protocol;
}

const char *
purple_connection_get_password(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->password;
}

void
purple_connection_set_password(PurpleConnection *connection,
                               const char *password)
{
	PurpleConnectionPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONNECTION(connection));

	priv = purple_connection_get_instance_private(connection);

	purple_str_wipe(priv->password);
	priv->password = g_strdup(password);

	g_object_notify_by_pspec(G_OBJECT(connection), properties[PROP_PASSWORD]);
}

const char *
purple_connection_get_display_name(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->display_name;
}

gboolean
_purple_connection_wants_to_die(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), FALSE);

	priv = purple_connection_get_instance_private(connection);

	return priv->wants_to_die;
}

static gboolean
purple_connection_disconnect_cb(gpointer data) {
	PurpleAccount *account = data;
	PurpleConnection *connection;

	connection = purple_account_get_connection(account);

	if(PURPLE_IS_CONNECTION(connection)) {
		PurpleConnectionPrivate *priv = NULL;

		priv = purple_connection_get_instance_private(connection);

		priv->disconnect_timeout = 0;

		if(priv->state != PURPLE_CONNECTION_STATE_DISCONNECTED) {
			/* If the connection is not disconnected, disconnect it. */
			purple_account_disconnect(account);
		} else {
			/* Otherwise assume the connection was already disconnected or in
			 * the process of being disconnected and we just need to finish our
			 * cleanup.
			 */
			GError *error = NULL;

			if(!purple_connection_disconnect(connection, &error)) {
				const char *message = "unknown error";

				if(error != NULL) {
					message = error->message;
				}

				purple_debug_warning("connections",
				                     "failed to disconnect %p : %s",
				                     connection, message);
			}

			g_clear_error(&error);

			purple_account_set_connection(account, NULL);
		}
	}

	return G_SOURCE_REMOVE;
}

void
purple_connection_error(PurpleConnection *connection,
                        PurpleConnectionError reason,
                        const char *description)
{
	PurpleConnectionPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_CONNECTION(connection));

	priv = purple_connection_get_instance_private(connection);

	/* This sanity check relies on PURPLE_CONNECTION_ERROR_OTHER_ERROR
	 * being the last member of the PurpleConnectionError enum in
	 * connection.h; if other reasons are added after it, this check should
	 * be updated.
	 */
	if(reason > PURPLE_CONNECTION_ERROR_OTHER_ERROR) {
		purple_debug_error("connection",
			"purple_connection_error: reason %u isn't a "
			"valid reason\n", reason);
		reason = PURPLE_CONNECTION_ERROR_OTHER_ERROR;
	}

	if(description == NULL) {
		purple_debug_error("connection", "purple_connection_error called with NULL description\n");
		description = _("Unknown error");
	}

	/* If we've already got one error, we don't need any more */
	if(priv->error_info != NULL) {
		return;
	}

	priv->wants_to_die = purple_connection_error_is_fatal(reason);

	purple_debug_info("connection",
	                  "Connection error on %p (reason: %u description: %s)\n",
	                  connection, reason, description);

	priv->error_info = purple_connection_error_info_new(reason, description);

	purple_signal_emit(purple_connections_get_handle(), "connection-error",
	                   connection, reason, description);

	priv->disconnect_timeout = g_timeout_add(0, purple_connection_disconnect_cb,
	                                         priv->account);
}

PurpleConnectionErrorInfo *
purple_connection_get_error_info(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->error_info;
}

void
purple_connection_g_error(PurpleConnection *connection, const GError *error) {
	PurpleConnectionError reason;

	if(g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		/* Not a connection error. Ignore. */
		return;
	}

	if(error->domain == G_TLS_ERROR) {
		switch (error->code) {
			case G_TLS_ERROR_UNAVAILABLE:
				reason = PURPLE_CONNECTION_ERROR_NO_SSL_SUPPORT;
				break;
			case G_TLS_ERROR_NOT_TLS:
			case G_TLS_ERROR_HANDSHAKE:
				reason = PURPLE_CONNECTION_ERROR_ENCRYPTION_ERROR;
				break;
			case G_TLS_ERROR_BAD_CERTIFICATE:
			case G_TLS_ERROR_CERTIFICATE_REQUIRED:
				reason = PURPLE_CONNECTION_ERROR_CERT_OTHER_ERROR;
				break;
			case G_TLS_ERROR_EOF:
			case G_TLS_ERROR_MISC:
			default:
				reason = PURPLE_CONNECTION_ERROR_NETWORK_ERROR;
		}
	} else if (error->domain == G_IO_ERROR) {
		reason = PURPLE_CONNECTION_ERROR_NETWORK_ERROR;
	} else if (error->domain == PURPLE_CONNECTION_ERROR) {
		reason = error->code;
	} else {
		reason = PURPLE_CONNECTION_ERROR_OTHER_ERROR;
	}

	purple_connection_error(connection, reason, error->message);
}

void
purple_connection_take_error(PurpleConnection *connection, GError *error) {
	purple_connection_g_error(connection, error);
	g_error_free(error);
}

gboolean
purple_connection_error_is_fatal(PurpleConnectionError reason) {
	switch (reason) {
		case PURPLE_CONNECTION_ERROR_NETWORK_ERROR:
		case PURPLE_CONNECTION_ERROR_ENCRYPTION_ERROR:
		case PURPLE_CONNECTION_ERROR_CUSTOM_TEMPORARY:
			return FALSE;
		case PURPLE_CONNECTION_ERROR_INVALID_USERNAME:
		case PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED:
		case PURPLE_CONNECTION_ERROR_AUTHENTICATION_IMPOSSIBLE:
		case PURPLE_CONNECTION_ERROR_NO_SSL_SUPPORT:
		case PURPLE_CONNECTION_ERROR_NAME_IN_USE:
		case PURPLE_CONNECTION_ERROR_INVALID_SETTINGS:
		case PURPLE_CONNECTION_ERROR_CERT_NOT_PROVIDED:
		case PURPLE_CONNECTION_ERROR_CERT_UNTRUSTED:
		case PURPLE_CONNECTION_ERROR_CERT_EXPIRED:
		case PURPLE_CONNECTION_ERROR_CERT_NOT_ACTIVATED:
		case PURPLE_CONNECTION_ERROR_CERT_HOSTNAME_MISMATCH:
		case PURPLE_CONNECTION_ERROR_CERT_FINGERPRINT_MISMATCH:
		case PURPLE_CONNECTION_ERROR_CERT_SELF_SIGNED:
		case PURPLE_CONNECTION_ERROR_CERT_OTHER_ERROR:
		case PURPLE_CONNECTION_ERROR_CUSTOM_FATAL:
		case PURPLE_CONNECTION_ERROR_OTHER_ERROR:
			return TRUE;
		default:
			g_return_val_if_reached(TRUE);
	}
}

/**************************************************************************
 * Helpers
 **************************************************************************/
static void
purple_connection_set_id(PurpleConnection *connection, const gchar *id) {
	PurpleConnectionPrivate *priv = NULL;

	priv = purple_connection_get_instance_private(connection);

	if(g_set_str(&priv->id, id)) {
		g_object_notify_by_pspec(G_OBJECT(connection), properties[PROP_ID]);
	}
}

static void
purple_connection_set_account(PurpleConnection *connection,
                              PurpleAccount *account)
{
	PurpleConnectionPrivate *priv = NULL;

	priv = purple_connection_get_instance_private(connection);

	if(g_set_object(&priv->account, account)) {
		g_object_notify_by_pspec(G_OBJECT(connection),
		                         properties[PROP_ACCOUNT]);
	}
}

/**************************************************************************
 * GObject Implementation
 **************************************************************************/
static void
purple_connection_set_property(GObject *obj, guint param_id,
                               const GValue *value, GParamSpec *pspec)
{
	PurpleConnection *connection = PURPLE_CONNECTION(obj);
	PurpleConnectionPrivate *priv = NULL;

	priv = purple_connection_get_instance_private(connection);

	switch (param_id) {
		case PROP_ID:
			purple_connection_set_id(connection, g_value_get_string(value));
			break;
		case PROP_PROTOCOL:
			priv->protocol = g_value_get_object(value);
			break;
		case PROP_FLAGS:
			purple_connection_set_flags(connection, g_value_get_flags(value));
			break;
		case PROP_STATE:
			purple_connection_set_state(connection, g_value_get_enum(value));
			break;
		case PROP_ACCOUNT:
			purple_connection_set_account(connection,
			                              g_value_get_object(value));
			break;
		case PROP_PASSWORD:
			purple_connection_set_password(connection,
			                               g_value_get_string(value));
			break;
		case PROP_DISPLAY_NAME:
			purple_connection_set_display_name(connection,
			                                   g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_connection_get_property(GObject *obj, guint param_id, GValue *value,
                               GParamSpec *pspec)
{
	PurpleConnection *connection = PURPLE_CONNECTION(obj);

	switch (param_id) {
		case PROP_ID:
			g_value_set_string(value, purple_connection_get_id(connection));
			break;
		case PROP_CANCELLABLE:
			g_value_set_object(value,
			                   purple_connection_get_cancellable(connection));
			break;
		case PROP_PROTOCOL:
			g_value_set_object(value,
			                   purple_connection_get_protocol(connection));
			break;
		case PROP_FLAGS:
			g_value_set_flags(value, purple_connection_get_flags(connection));
			break;
		case PROP_STATE:
			g_value_set_enum(value, purple_connection_get_state(connection));
			break;
		case PROP_ACCOUNT:
			g_value_set_object(value,
			                   purple_connection_get_account(connection));
			break;
		case PROP_PASSWORD:
			g_value_set_string(value,
			                   purple_connection_get_password(connection));
			break;
		case PROP_DISPLAY_NAME:
			g_value_set_string(value,
			                   purple_connection_get_display_name(connection));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_connection_init(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	priv = purple_connection_get_instance_private(connection);

	priv->cancellable = g_cancellable_new();

	purple_connection_set_state(connection, PURPLE_CONNECTION_STATE_CONNECTING);
	connections = g_list_append(connections, connection);
}

static void
purple_connection_constructed(GObject *object) {
	PurpleConnection *connection = PURPLE_CONNECTION(object);
	PurpleConnectionPrivate *priv = NULL;

	G_OBJECT_CLASS(purple_connection_parent_class)->constructed(object);

	priv = purple_connection_get_instance_private(connection);

	if(priv->id == NULL) {
		gchar *uuid = g_uuid_string_random();

		purple_connection_set_id(connection, uuid);

		g_free(uuid);
	}

	purple_signal_emit(purple_connections_get_handle(), "signing-on",
	                   connection);
}

static void
purple_connection_dispose(GObject *obj) {
	PurpleConnection *connection = PURPLE_CONNECTION(obj);
	PurpleConnectionPrivate *priv = NULL;

	priv = purple_connection_get_instance_private(connection);

	g_clear_object(&priv->account);

	g_clear_handle_id(&priv->disconnect_timeout, g_source_remove);

	G_OBJECT_CLASS(purple_connection_parent_class)->dispose(obj);
}

static void
purple_connection_finalize(GObject *object) {
	PurpleConnection *connection = PURPLE_CONNECTION(object);
	PurpleConnectionPrivate *priv = NULL;

	priv = purple_connection_get_instance_private(connection);

	g_clear_pointer(&priv->error_info, purple_connection_error_info_free);

	purple_str_wipe(priv->password);
	g_free(priv->display_name);
	g_free(priv->id);

	g_clear_object(&priv->cancellable);

	G_OBJECT_CLASS(purple_connection_parent_class)->finalize(object);
}

static void
purple_connection_class_init(PurpleConnectionClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_connection_get_property;
	obj_class->set_property = purple_connection_set_property;
	obj_class->dispose = purple_connection_dispose;
	obj_class->finalize = purple_connection_finalize;
	obj_class->constructed = purple_connection_constructed;

	/**
	 * PurpleConnection:id:
	 *
	 * The unique identifier for the connection.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", "id",
		"The identifier of the connection",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:cancellable:
	 *
	 * A [class@Gio.Cancellable] to be used with the connection.
	 *
	 * This can be passed function that require a cancellable for the
	 * connection.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CANCELLABLE] = g_param_spec_object(
		"cancellable", "cancellable",
		"A cancellable for the connection.",
		G_TYPE_CANCELLABLE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:protocol:
	 *
	 * The protocol that this connection is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PROTOCOL] = g_param_spec_object(
		"protocol", "Protocol",
		"The protocol that the connection is using.",
		PURPLE_TYPE_PROTOCOL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:flags:
	 *
	 * The flags for this connection.
	 *
	 * Since: 3.0
	 */
	properties[PROP_FLAGS] = g_param_spec_flags(
		"flags", "Connection flags",
		"The flags of the connection.",
		PURPLE_TYPE_CONNECTION_FLAGS, 0,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:state:
	 *
	 * The state of the connection.
	 *
	 * Since: 3.0
	 */
	properties[PROP_STATE] = g_param_spec_enum(
		"state", "Connection state",
		"The current state of the connection.",
		PURPLE_TYPE_CONNECTION_STATE, PURPLE_CONNECTION_STATE_DISCONNECTED,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:account:
	 *
	 * The account this connection belongs to.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "Account",
		"The account using the connection.", PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:password:
	 *
	 * The password for this connection.
	 *
	 * This is only stored for reconnections and may go away in the future.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PASSWORD] = g_param_spec_string(
		"password", "Password",
		"The password used for connection.", NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleConnection:display-name:
	 *
	 * The display name for the account.
	 *
	 * Since: 3.0
	 */
	properties[PROP_DISPLAY_NAME] = g_param_spec_string(
		"display-name", "Display name",
		"Your name that appears to other people.", NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

gboolean
purple_connection_connect(PurpleConnection *connection, GError **error) {
	PurpleConnectionClass *klass = NULL;
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), FALSE);

	priv = purple_connection_get_instance_private(connection);

	if(!purple_account_is_disconnected(priv->account)) {
		g_set_error(error, PURPLE_CONNECTION_ERROR, 0,
		            "account %s is not disconnected",
		            purple_account_get_username(priv->account));

		return TRUE;
	}

	if(((priv->password == NULL) || (*priv->password == '\0')) &&
	   !(purple_protocol_get_options(priv->protocol) & OPT_PROTO_NO_PASSWORD) &&
	   !(purple_protocol_get_options(priv->protocol) & OPT_PROTO_PASSWORD_OPTIONAL))
	{
		g_set_error(error, PURPLE_CONNECTION_ERROR, 0,
		            "Cannot connect to account %s without a password.",
		            purple_account_get_username(priv->account));

		return FALSE;
	}

	purple_debug_info("connection", "Connecting. connection = %p",
	                  connection);

	klass = PURPLE_CONNECTION_GET_CLASS(connection);
	if(klass != NULL && klass->connect != NULL) {
		return klass->connect(connection, error);
	}

	g_set_error(error, PURPLE_CONNECTION_ERROR, 0,
	            "The connection for %s did not implement the connect method",
	            purple_account_get_username(priv->account));

	return FALSE;
}

gboolean
purple_connection_disconnect(PurpleConnection *connection, GError **error) {
	PurpleConnectionClass *klass = NULL;
	PurpleConnectionPrivate *priv = NULL;
	gboolean ret = TRUE;
	gpointer handle = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), FALSE);

	/* We don't check if the connection's state is connected as everything
	 * should be idempotent when doing cleanup.
	 */

	priv = purple_connection_get_instance_private(connection);

	handle = purple_connections_get_handle();

	purple_debug_info("connection", "Disconnecting connection %p", connection);
	purple_connection_set_state(connection,
	                            PURPLE_CONNECTION_STATE_DISCONNECTING);
	purple_signal_emit(handle, "signing-off", connection);

	/* Dispatch to the connection's disconnect method. */
	klass = PURPLE_CONNECTION_GET_CLASS(connection);
	if(klass != NULL && klass->disconnect != NULL) {
		ret = klass->disconnect(connection, error);
	}

	/* Do the rest of our cleanup. */
	connections = g_list_remove(connections, connection);

	purple_connection_set_state(connection,
	                            PURPLE_CONNECTION_STATE_DISCONNECTED);

	purple_signal_emit(handle, "signed-off", connection);

	purple_account_request_close_with_account(priv->account);
	purple_request_close_with_handle(connection);
	purple_notify_close_with_handle(connection);

	connections_connected = g_list_remove(connections_connected, connection);
	if(connections_connected == NULL) {
		purple_signal_emit(handle, "offline");
	}

	purple_debug_info("connection", "Destroying connection %p", connection);

	return ret;
}

GCancellable *
purple_connection_get_cancellable(PurpleConnection *connection) {
	PurpleConnectionPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_CONNECTION(connection), NULL);

	priv = purple_connection_get_instance_private(connection);

	return priv->cancellable;
}

/**************************************************************************
 * Connections API
 **************************************************************************/

void
purple_connections_disconnect_all(void) {
	GList *l;

	while((l = purple_connections_get_all()) != NULL) {
		PurpleConnection *connection = l->data;
		PurpleConnectionPrivate *priv = NULL;

		priv = purple_connection_get_instance_private(connection);

		priv->wants_to_die = TRUE;

		purple_account_disconnect(priv->account);
	}
}

GList *
purple_connections_get_all(void) {
	return connections;
}

gboolean
purple_connections_is_online(void) {
	return (connections_connected != NULL);
}

void
purple_connections_init(void) {
	void *handle = purple_connections_get_handle();

	purple_signal_register(handle, "online", purple_marshal_VOID, G_TYPE_NONE,
	                       0);

	purple_signal_register(handle, "offline", purple_marshal_VOID, G_TYPE_NONE,
	                       0);

	purple_signal_register(handle, "signing-on",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_CONNECTION);

	purple_signal_register(handle, "signed-on",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_CONNECTION);

	purple_signal_register(handle, "signing-off",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_CONNECTION);

	purple_signal_register(handle, "signed-off",
						 purple_marshal_VOID__POINTER, G_TYPE_NONE, 1,
						 PURPLE_TYPE_CONNECTION);

	purple_signal_register(handle, "connection-error",
	                       purple_marshal_VOID__POINTER_INT_POINTER,
	                       G_TYPE_NONE, 3, PURPLE_TYPE_CONNECTION,
	                       PURPLE_TYPE_CONNECTION_ERROR, G_TYPE_STRING);

	purple_signal_register(handle, "autojoin",
	                       purple_marshal_BOOLEAN__POINTER, G_TYPE_NONE, 1,
	                       PURPLE_TYPE_CONNECTION);
}

void
purple_connections_uninit(void) {
	purple_signals_unregister_by_instance(purple_connections_get_handle());
}

void *
purple_connections_get_handle(void) {
	return &connections_handle;
}
