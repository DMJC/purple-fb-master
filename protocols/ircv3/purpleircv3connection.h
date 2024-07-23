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

#if !defined(PURPLE_IRCV3_GLOBAL_HEADER_INSIDE) && \
    !defined(PURPLE_IRCV3_COMPILATION)
# error "only <libpurple/protocols/ircv3.h> may be included directly"
#endif

#ifndef PURPLE_IRCV3_CONNECTION_H
#define PURPLE_IRCV3_CONNECTION_H

#include <glib.h>
#include <glib-object.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

#include <ibis.h>

#include "purpleircv3version.h"

G_BEGIN_DECLS

#define PURPLE_IRCV3_TYPE_CONNECTION (purple_ircv3_connection_get_type())

PURPLE_IRCV3_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE(PurpleIRCv3Connection, purple_ircv3_connection,
                         PURPLE_IRCV3, CONNECTION, PurpleConnection)

struct _PurpleIRCv3ConnectionClass {
	/*< private >*/
	PurpleConnectionClass parent;

	/*< private >*/
	gpointer reserved[8];
};

/**
 * purple_ircv3_connection_register: (skip)
 * @plugin: The GTypeModule
 *
 * Registers the dynamic type using @plugin.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL void purple_ircv3_connection_register(GPluginNativePlugin *plugin);

/**
 * purple_ircv3_connection_get_client:
 * @connection: The instance.
 *
 * Gets the [class@Ibis.Client] from @connection.
 *
 * Returns: (transfer none): The client instance if connected, otherwise %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
IbisClient *purple_ircv3_connection_get_client(PurpleIRCv3Connection *connection);

/**
 * purple_ircv3_connection_writef:
 * @connection: The instance.
 * @format: The format string.
 * @...: The arguments for @format.
 *
 * Similar to C `printf()` but writes the format string out to @connection.
 *
 * This will add the proper line termination, so you do not need to worry about
 * that.
 */
void purple_ircv3_connection_writef(PurpleIRCv3Connection *connection, const char *format, ...) G_GNUC_PRINTF(2, 3);

/**
 * purple_ircv3_connection_add_status_message:
 * @connection: The instance.
 * @message: The message.
 *
 * Adds a message to the status conversation/window for @connection.
 *
 * Since: 3.0
 */
PURPLE_IRCV3_AVAILABLE_IN_ALL
void purple_ircv3_connection_add_status_message(PurpleIRCv3Connection *connection, IbisMessage *message);

/**
 * purple_ircv3_connection_is_channel:
 * @connection: The instance.
 * @id: The id to check.
 *
 * Checks if @id is a channel.
 *
 * Right now this just checks if @id starts with a `#` but in the future this
 * will be updated to check all channel prefixes that the connection supports.
 *
 * Returns: %TRUE if @id is a channel otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_IRCV3_AVAILABLE_IN_ALL
gboolean purple_ircv3_connection_is_channel(PurpleIRCv3Connection *connection, const char *id);

/**
 * purple_ircv3_connection_find_or_create_conversation:
 * @connection: The instance.
 * @id: The id of the conversation.
 *
 * Looks for an existing conversation belonging to @connection and returns it
 * if found. If not found a new conversation will be created.
 *
 * This will only ever return %NULL if @connection is invalid or @id is %NULL.
 *
 * Note that ownership of the conversation remains with the default
 * [class@Purple.ConversationManager].
 *
 * Returns: (transfer none) (nullable): The conversation.
 *
 * Since: 3.0
 */
PURPLE_IRCV3_AVAILABLE_IN_ALL
PurpleConversation *purple_ircv3_connection_find_or_create_conversation(PurpleIRCv3Connection *connection, const char *id);

/**
 * purple_ircv3_connection_find_or_create_contact:
 * @connection: The instance.
 * @message: The message to get the contact for.
 *
 * Looks for an existing contact from @message belonging to @connection and
 * returns it if found. If not a new contact will be created.
 *
 * This will only ever return %NULL if @connection is invalid or @message is
 * %NULL.
 *
 * Note that the ownership of the contact remains with the default
 * [class@Purple.ContactManager].
 *
 * Returns: (transfer none) (nullable): The contact.
 *
 * Since: 3.0
 */
PURPLE_IRCV3_AVAILABLE_IN_ALL
PurpleContact *purple_ircv3_connection_find_or_create_contact(PurpleIRCv3Connection *connection, IbisMessage *message);

G_END_DECLS

#endif /* PURPLE_IRCV3_CONNECTION_H */
