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

#ifndef PURPLE_PROTOCOL_CHAT_H
#define PURPLE_PROTOCOL_CHAT_H

#include <glib.h>
#include <glib-object.h>

#include "connection.h"
#include "purpleconversation.h"
#include "purplemessage.h"
#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_CHAT (purple_protocol_chat_get_type())

/**
 * PurpleProtocolChat:
 *
 * #PurpleProtocolChat describes the API that protocols need to implement for
 * handling multiple user conversations.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolChat, purple_protocol_chat, PURPLE,
                    PROTOCOL_CHAT, PurpleProtocol)

typedef struct _PurpleProtocolChatEntry PurpleProtocolChatEntry;

/**
 * PurpleProtocolChatEntry:
 * @label:      User-friendly name of the entry
 * @identifier: Used by the protocol to identify the option
 * @required:   True if it's required
 * @is_int:     True if the entry expects an integer
 * @min:        Minimum value in case of integer
 * @max:        Maximum value in case of integer
 * @secret:     True if the entry is secret (password)
 *
 * Represents an entry containing information that must be supplied by the
 * user when joining a chat.
 */
struct _PurpleProtocolChatEntry {
	const char *label;
	const char *identifier;
	gboolean required;
	gboolean is_int;
	int min;
	int max;
	gboolean secret;
};

G_BEGIN_DECLS

struct _PurpleProtocolChatInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	GList *(*info)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection);

	GHashTable *(*info_defaults)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, const gchar *chat_name);

	void (*join)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, GHashTable *components);

	void (*reject)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, GHashTable *components);

	gchar *(*get_name)(PurpleProtocolChat *protocol_chat, GHashTable *components);

	void (*invite)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, const gchar *message, const gchar *who);

	void (*leave)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id);

	gint (*send)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, PurpleConversation *conversation, PurpleMessage *message);

	gchar *(*get_user_real_name)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, const gchar *who);

	void (*set_topic)(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, const gchar *topic);

	/*< private >*/
	gpointer reserved[8];
};

/**
 * purple_protocol_chat_info:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 *
 * Gets the list of #PurpleProtocolChatEntry's that are required to join a
 * multi user chat.
 *
 * Returns: (transfer full) (element-type PurpleProtocolChatEntry): The list
 *          of #PurpleProtocolChatEntry's that are used to join a chat.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GList *purple_protocol_chat_info(PurpleProtocolChat *protocol_chat, PurpleConnection *connection);

/**
 * purple_protocol_chat_info_defaults:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @chat_name: The name of the chat.
 *
 * Returns a #GHashTable of the default protocol dependent components that will
 * be passed to purple_protocol_chat_join().
 *
 * Returns: (transfer full) (element-type utf8 utf8): The values that will be
 *          used to join the chat.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GHashTable *purple_protocol_chat_info_defaults(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, const gchar *chat_name);

/**
 * purple_protocol_chat_join:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @components: (element-type utf8 utf8) (transfer none): The protocol
 *              dependent join components.
 *
 * Joins the chat described in @components.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_chat_join(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, GHashTable *components);

/**
 * purple_protocol_chat_reject:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @components: (element-type utf8 utf8) (transfer none): The protocol
 *              dependent join components.
 *
 * Used to reject a chat invite.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_chat_reject(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, GHashTable *components);

/**
 * purple_protocol_chat_get_name:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @components: (element-type utf8 utf8) (transfer none): The protocol
 *              dependent join components.
 *
 * Gets the name from @components.
 *
 * Returns: (transfer full): The chat name from @components.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gchar *purple_protocol_chat_get_name(PurpleProtocolChat *protocol_chat, GHashTable *components);

/**
 * purple_protocol_chat_invite:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @id: The id of the chat.
 * @message: The invite message.
 * @who: The target of the invite.
 *
 * Sends an invite to @who with @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_chat_invite(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, const gchar *message, const gchar *who);

/**
 * purple_protocol_chat_leave:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @id: The id of the chat.
 *
 * Leaves the chat identified by @id.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_chat_leave(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id);

/**
 * purple_protocol_chat_send:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @id: The id of the chat.
 * @conversation: The [class@Conversation] of the chat.
 * @message: The message to send.
 *
 * Sends @message to the chat identified by @id.
 *
 * Returns: 0 on success, non-zero on failure.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gint purple_protocol_chat_send(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, PurpleConversation *conversation, PurpleMessage *message);

/**
 * purple_protocol_chat_get_user_real_name:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @id: The id of the chat.
 * @who: The username.
 *
 * Gets the real name of @who.
 *
 * Returns: (transfer full): The realname of @who.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gchar *purple_protocol_chat_get_user_real_name(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, const gchar *who);

/**
 * purple_protocol_chat_set_topic:
 * @protocol_chat: The #PurpleProtocolChat instance.
 * @connection: The #PurpleConnection instance.
 * @id: The id of the chat.
 * @topic: The new topic.
 *
 * Sets the topic for the chat with id @id to @topic.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_chat_set_topic(PurpleProtocolChat *protocol_chat, PurpleConnection *connection, gint id, const gchar *topic);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_CHAT_H */

