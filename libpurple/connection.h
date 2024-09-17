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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_CONNECTION_H
#define PURPLE_CONNECTION_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_CONNECTION  purple_connection_get_type()

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_DERIVABLE_TYPE(PurpleConnection, purple_connection, PURPLE,
                         CONNECTION, GObject)

/* This is meant to track use-after-free errors.
 * TODO: it should be disabled in released code. */
#define PURPLE_ASSERT_CONNECTION_IS_VALID(gc) \
	_purple_assert_connection_is_valid(gc, __FILE__, __LINE__)

/**
 * PurpleConnectionState:
 * @PURPLE_CONNECTION_STATE_DISCONNECTED: Disconnected
 * @PURPLE_CONNECTION_STATE_DISCONNECTING: Disconnecting
 * @PURPLE_CONNECTION_STATE_CONNECTED: Connected
 * @PURPLE_CONNECTION_STATE_CONNECTING: Connecting
 *
 * A representation of the state of a [class@Purple.Connection].
 *
 * Since: 2.0
 */
typedef enum {
	PURPLE_CONNECTION_STATE_DISCONNECTED PURPLE_AVAILABLE_ENUMERATOR_IN_3_0 = 0,
	PURPLE_CONNECTION_STATE_DISCONNECTING PURPLE_AVAILABLE_ENUMERATOR_IN_3_0,
	PURPLE_CONNECTION_STATE_CONNECTED PURPLE_AVAILABLE_ENUMERATOR_IN_3_0,
	PURPLE_CONNECTION_STATE_CONNECTING PURPLE_AVAILABLE_ENUMERATOR_IN_3_0
} PurpleConnectionState;

/**
 * PURPLE_CONNECTION_ERROR:
 *
 * Error domain for Purple connection errors. Errors in this domain will be
 * from the #PurpleConnectionError enum.
 *
 * Since: 3.0
 */
#define PURPLE_CONNECTION_ERROR \
	g_quark_from_static_string("purple-connection-error") \
	PURPLE_AVAILABLE_MACRO_IN_3_0

#include "purpleaccount.h"
#include "purpleconnectionerrorinfo.h"
#include "purpleprotocol.h"

struct _PurpleConnectionClass {
	/*< private >*/
	GObjectClass parent;

	gboolean (*connect)(PurpleConnection *connection, GError **error);
	gboolean (*disconnect)(PurpleConnection *connection, GError **error);

	/*< private >*/
	gpointer reserved[8];
};

/**************************************************************************/
/* Connection API                                                         */
/**************************************************************************/

/**
 * purple_connection_connect:
 * @connection: The instance.
 * @error: Return address for a #GError, or %NULL.
 *
 * Tells the connection to connect. This is done by calling the
 * [vfunc@Purple.Connection.connect] function. State is managed by this
 * function.
 *
 * Due to the asynchronous nature of network connections, the return value and
 * @error are to be used to do some initial validation before a connection is
 * actually attempted.
 *
 * Returns: %TRUE if the initial connection for @account was successful,
 *          otherwise %FALSE with @error possibly set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_connection_connect(PurpleConnection *connection, GError **error);

/**
 * purple_connection_disconnect:
 * @connection: The instance.
 * @error: Return address for a #GError, or %NULL.
 *
 * Tells the connection to disconnect. This is done by calling the
 * [vfunc@Purple.Connection.disconnect] function. State is managed by this
 * function.
 *
 * Returns: %TRUE if the account was disconnected gracefully, otherwise %FALSE
 *          with @error possibly set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_connection_disconnect(PurpleConnection *connection, GError **error);

/**
 * purple_connection_set_state:
 * @gc:    The connection.
 * @state: The connection state.
 *
 * Sets the connection state.  Protocols should call this and pass in
 * the state #PURPLE_CONNECTION_CONNECTED when the account is completely
 * signed on.  What does it mean to be completely signed on?  If
 * the core can call protocol's set_status, and it successfully changes
 * your status, then the account is online.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_connection_set_state(PurpleConnection *gc, PurpleConnectionState state);

/**
 * purple_connection_get_state:
 * @gc: The connection.
 *
 * Returns the connection state.
 *
 * Returns: The connection state.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleConnectionState purple_connection_get_state(PurpleConnection *gc);

/**
 * PURPLE_CONNECTION_IS_CONNECTED:
 *
 * Returns TRUE if the account is connected, otherwise returns FALSE.
 *
 * Returns: TRUE if the account is connected, otherwise returns FALSE.
 *
 * Since: 2.0
 */
#define PURPLE_CONNECTION_IS_CONNECTED(gc) \
	(purple_connection_get_state(gc) == PURPLE_CONNECTION_STATE_CONNECTED)

/**
 * purple_connection_get_id:
 * @connection: The connection.
 *
 * Gets the identifier of the connection.
 *
 * Returns: The identifier of the connection.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *purple_connection_get_id(PurpleConnection *connection);

/**
 * purple_connection_get_account:
 * @gc: The connection.
 *
 * Returns the connection's account.
 *
 * Returns: (transfer none): The connection's account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleAccount *purple_connection_get_account(PurpleConnection *gc);

/**
 * purple_connection_get_protocol:
 * @gc: The connection.
 *
 * Returns the protocol managing a connection.
 *
 * Returns: (transfer none): The protocol.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleProtocol *purple_connection_get_protocol(PurpleConnection *gc);

/**
 * purple_connection_get_password:
 * @gc: The connection.
 *
 * Returns the connection's password.
 *
 * Returns: The connection's password.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_connection_get_password(PurpleConnection *gc);

/**
 * purple_connection_set_password:
 * @connection: The instance.
 * @password: (nullable): The new password.
 *
 * Sets the password for @connection to @password.
 *
 * This will not change your password on the remote service. It just updates
 * the password that the protocol should use when connecting.
 *
 * This is generally used by protocol plugins that support multiple
 * authentication methods and need to prompt the user for a password.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_connection_set_password(PurpleConnection *connection, const char *password);

/**
 * purple_connection_get_cancellable:
 * @connection: The instance.
 *
 * Gets the cancellable that should be used with @connection.
 *
 * Returns: (transfer none): The cancellable.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GCancellable *purple_connection_get_cancellable(PurpleConnection *connection);

/**
 * purple_connection_error:
 * @gc:          the connection which is closing.
 * @reason:      why the connection is closing.
 * @description: a localized description of the error (not %NULL ).
 *
 * Closes a connection with an error and a human-readable description of the
 * error.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_connection_error(PurpleConnection *gc, PurpleConnectionError reason, const char *description);

/**
 * purple_connection_get_error_info:
 * @gc: The connection.
 *
 * Returns the #PurpleConnectionErrorInfo instance of a connection if an
 * error exists.
 *
 * Returns: The #PurpleConnectionErrorInfo instance of the connection if an
 *          error exists, %NULL otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConnectionErrorInfo *purple_connection_get_error_info(PurpleConnection *gc);

/**
 * purple_connection_g_error:
 * @pc: Connection the error is associated with
 * @error: Error information
 *
 * Closes a connection similar to [method@Purple.Connection.error], but takes a
 * [type@GLib.Error] which is then converted to purple error codes.
 *
 * This function ignores G_IO_ERROR_CANCELLED, returning without
 * closing the connection. This can be used as a shortcut when
 * cancelling connections, as this is commonly done when shutting
 * down a connection. If G_IO_ERROR_CANCELLED needs to be caught,
 * do so with [method@GLib.Error.matches] prior to calling this function.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_connection_g_error(PurpleConnection *pc, const GError *error);

/**
 * purple_connection_take_error:
 * @pc: Connection the error is associated with
 * @error: Return address for a #GError, or %NULL.
 *
 * Closes a connection similar to [method@Purple.Connection.error], but takes a
 * [type@GLib.Error] which is then converted to purple error codes.
 *
 * This function is equivalent to [method@Purple.Connection.g_error], expect
 * that it takes ownership of the GError.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_connection_take_error(PurpleConnection *pc, GError *error);

/**
 * purple_connection_error_is_fatal:
 * @reason: The connection error to check.
 *
 * Reports whether a disconnection reason is fatal (in which case the account
 * should probably not be automatically reconnected) or transient (so
 * auto-reconnection is a good idea).
 *
 * For instance, #PURPLE_CONNECTION_ERROR_NETWORK_ERROR is a temporary error,
 * which might be caused by losing the network connection, so <code>
 * purple_connection_error_is_fatal(PURPLE_CONNECTION_ERROR_NETWORK_ERROR)
 * </code> is %FALSE.
 *
 * On the other hand, #PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED probably
 * indicates a misconfiguration of the account which needs the user to go fix
 * it up, so <code>
 * purple_connection_error_is_fatal(PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED)
 * </code> is %TRUE.
 *
 * Returns: %TRUE if the account should not be automatically reconnected, and
 *         %FALSE otherwise.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_connection_error_is_fatal(PurpleConnectionError reason);

/**************************************************************************/
/* Connections API                                                        */
/**************************************************************************/

/**
 * purple_connections_disconnect_all:
 *
 * Disconnects from all connections.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_connections_disconnect_all(void);

/**
 * purple_connections_get_all:
 *
 * Returns a list of all active connections.  This does not
 * include connections that are in the process of connecting.
 *
 * Returns: (element-type PurpleConnection) (transfer none): A list of all
 *          active connections.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
GList *purple_connections_get_all(void);

/**
 * purple_connections_is_online:
 *
 * Checks if at least one account is online.
 *
 * Returns: %TRUE if at least one account is online.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_connections_is_online(void);

/**************************************************************************/
/* Connections Subsystem                                                  */
/**************************************************************************/

/**
 * purple_connections_init:
 *
 * Initializes the connections subsystem.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_connections_init(void);

/**
 * purple_connections_uninit:
 *
 * Uninitializes the connections subsystem.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_connections_uninit(void);

/**
 * purple_connections_get_handle:
 *
 * Returns the handle to the connections subsystem.
 *
 * Returns: The connections subsystem handle.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void *purple_connections_get_handle(void);

G_END_DECLS

#endif /* PURPLE_CONNECTION_H */
