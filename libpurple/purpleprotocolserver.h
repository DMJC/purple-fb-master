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

#ifndef PURPLE_PROTOCOL_SERVER_H
#define PURPLE_PROTOCOL_SERVER_H

#include <glib.h>
#include <glib-object.h>

#include "connection.h"
#include "purpleaccount.h"
#include "purplemessage.h"
#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_SERVER (purple_protocol_server_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolServer, purple_protocol_server, PURPLE,
                    PROTOCOL_SERVER, PurpleProtocol)

G_BEGIN_DECLS

/**
 * PurpleProtocolServer:
 *
 * #PurpleProtocolServer describes the API for protocols that have a central
 * server.
 *
 * Since: 3.0
 */

struct _PurpleProtocolServerInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	void (*set_info)(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *info);
	void (*get_info)(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *who);

	void (*set_idle)(PurpleProtocolServer *protocol_server, PurpleConnection *connection, gint idletime);

	void (*change_passwd)(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *old_pass, const gchar *new_pass);

	gint (*send_raw)(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *buf, gint len);

	/*< private >*/
	gpointer reserved[8];
};

/**
 * purple_protocol_server_set_info:
 * @protocol_server: The #PurpleProtocolServer instance.
 * @connection: The #PurpleConnection instance.
 * @info: The user info to set.
 *
 * Sets the user info, sometimes referred to as a user profile to @info.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_server_set_info(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *info);

/**
 * purple_protocol_server_get_info:
 * @protocol_server: The #PurpleProtocolServer instance.
 * @connection: The #PurpleConnection instance.
 * @who: The name of the user whose information you're asking for.
 *
 * Gets the user info or profile for @who and displays it in a protocol
 * specific way.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_server_get_info(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *who);

/**
 * purple_protocol_server_set_idle:
 * @protocol_server: The #PurpleProtocolServer instance.
 * @connection: The #PurpleConnection instance.
 * @idletime: The number of seconds that the user has been idle.
 *
 * Tells @protocol_server to set the user's idle time to @idletime.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_server_set_idle(PurpleProtocolServer *protocol_server, PurpleConnection *connection, gint idletime);

/**
 * purple_protocol_server_change_passwd:
 * @protocol_server: The #PurpleProtocolServer instance.
 * @connection: The #PurpleConnection instance.
 * @old_pass: The user's old password.
 * @new_pass: The new password for the user.
 *
 * Changes the user's password from @old_pass to @new_pass.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_server_change_passwd(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *old_pass, const gchar *new_pass);

/**
 * purple_protocol_server_send_raw:
 * @protocol_server: The #PurpleProtocolServer instance.
 * @connection: The #PurpleConnection instance.
 * @buf: The raw protocol data to send.
 * @len: The length of @buf in bytes.
 *
 * Sends raw data over the protocol. This should only be called when you know
 * the exact underlying protocol.
 *
 * Returns: The number of bytes that was sent.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gint purple_protocol_server_send_raw(PurpleProtocolServer *protocol_server, PurpleConnection *connection, const gchar *buf, gint len);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_SERVER_H */

