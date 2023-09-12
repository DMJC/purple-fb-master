/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_PROTOCOL_CONVERSATION_H
#define PURPLE_PROTOCOL_CONVERSATION_H

#include <glib.h>
#include <glib-object.h>

#include "purpleconversation.h"
#include "purplemessage.h"
#include "purpleprotocol.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_PROTOCOL_CONVERSATION (purple_protocol_conversation_get_type())
G_DECLARE_INTERFACE(PurpleProtocolConversation, purple_protocol_conversation,
                    PURPLE, PROTOCOL_CONVERSATION, PurpleProtocol)

/**
 * PurpleProtocolConversationInterface:
 *
 * This interface defines the behavior for interacting with conversations at
 * the protocol layer. These methods will primarily be called by the user
 * interface.
 *
 * Since: 3.0.0
 */
struct _PurpleProtocolConversationInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	void (*send_message_async)(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleMessage *message, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	gboolean (*send_message_finish)(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

	/*< private >*/
	gpointer reserved[8];
};

/**
 * purple_protocol_conversation_send_message_async:
 * @protocol: The instance.
 * @conversation: The conversation to send a message to.
 * @message: The message to send.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (nullable) (scope async): The callback to call after the message
 *            has been sent.
 * @data: (nullable): Optional user data to pass to @callback.
 *
 * Starts the process of sending @message to @conversation.
 *
 * Since: 3.0.0
 */
void purple_protocol_conversation_send_message_async(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleMessage *message, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_conversation_send_message_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolConversation.send_message_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to
 * [method@ProtocolConversation.send_message_async]. This should be called from
 * the callback of that function to get the result of whether or not the
 * message was sent successfully.
 *
 * Returns: %TRUE if the message was sent successfully, otherwise %FALSE with
 *          @error possibly set.
 *
 * Since: 3.0.0
 */
gboolean purple_protocol_conversation_send_message_finish(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_CONVERSATION_H */
