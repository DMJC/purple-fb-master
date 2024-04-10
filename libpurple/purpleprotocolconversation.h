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

#ifndef PURPLE_PROTOCOL_CONVERSATION_H
#define PURPLE_PROTOCOL_CONVERSATION_H

#include <glib.h>
#include <glib-object.h>

#include "purpleaccount.h"
#include "purpleavatar.h"
#include "purplechanneljoindetails.h"
#include "purpleconversation.h"
#include "purplecreateconversationdetails.h"
#include "purplemessage.h"
#include "purpleprotocol.h"
#include "purpletyping.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PURPLE_PROTOCOL_CONVERSATION_DOMAIN:
 *
 * An error domain for [iface@ProtocolConversation] errors.
 *
 * Since: 3.0
 */
#define PURPLE_PROTOCOL_CONVERSATION_DOMAIN \
	(g_quark_from_static_string("purple-protocol-conversation")) \
	PURPLE_AVAILABLE_MACRO_IN_3_0

#define PURPLE_TYPE_PROTOCOL_CONVERSATION (purple_protocol_conversation_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolConversation, purple_protocol_conversation,
                    PURPLE, PROTOCOL_CONVERSATION, PurpleProtocol)

/**
 * PurpleProtocolConversation:
 *
 * This interface defines the behavior for interacting with conversations at
 * the protocol layer. These methods will primarily be called by the user
 * interface.
 *
 * Since: 3.0
 */
struct _PurpleProtocolConversationInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	PurpleCreateConversationDetails *(*get_create_conversation_details)(PurpleProtocolConversation *protocol, PurpleAccount *account);
	void (*create_conversation_async)(PurpleProtocolConversation *protocol, PurpleAccount *account, PurpleCreateConversationDetails *details, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	PurpleConversation *(*create_conversation_finish)(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

	void (*send_message_async)(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleMessage *message, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	gboolean (*send_message_finish)(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

	void (*set_topic_async)(PurpleProtocolConversation *protocol, PurpleConversation *conversation, const char *topic, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	gboolean (*set_topic_finish)(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

	PurpleChannelJoinDetails *(*get_channel_join_details)(PurpleProtocolConversation *protocol, PurpleAccount *account);
	void (*join_channel_async)(PurpleProtocolConversation *protocol, PurpleAccount *account, PurpleChannelJoinDetails *details, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	gboolean (*join_channel_finish)(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

	void (*set_avatar_async)(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleAvatar *avatar, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	gboolean (*set_avatar_finish)(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

	void (*send_typing)(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleTypingState state);

	/*< private >*/
	gpointer reserved[8];
};

/**
 * purple_protocol_conversation_implements_create_conversation:
 * @protocol: The instance.
 *
 * Checks if @protocol implements
 * [vfunc@ProtocolConversation.get_create_conversation_details],
 * [vfunc@ProtocolConversation.create_conversation_async], and
 * [vfunc@ProtocolConversation.create_conversation_finish].
 *
 * Returns: %TRUE if everything is implemented, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_conversation_implements_create_conversation(PurpleProtocolConversation *protocol);

/**
 * purple_protocol_conversation_get_create_conversation_details:
 * @protocol: The instance.
 * @account: The account.
 *
 * Gets a [class@CreateConversationDetails] from @protocol for @account.
 *
 * This can then be used in calls to
 * [method@ProtocolConversation.create_conversation_async] to create the
 * conversation.
 *
 * Returns: (transfer full): The details to create a conversation with
 *          @account.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleCreateConversationDetails *purple_protocol_conversation_get_create_conversation_details(PurpleProtocolConversation *protocol, PurpleAccount *account);

/**
 * purple_protocol_conversation_create_conversation_async:
 * @protocol: The instance.
 * @account: The account.
 * @details: (transfer full): The create conversation details.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (nullable) (scope async): The callback to call after the
 *            conversation has been created.
 * @data: (nullable): Optional user data to pass to @callback.
 *
 * Starts the process of creating a dm or group dm conversation on @account.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_conversation_create_conversation_async(PurpleProtocolConversation *protocol, PurpleAccount *account, PurpleCreateConversationDetails *details, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_conversation_create_conversation_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolConversation.create_conversation_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to
 * [method@ProtocolConversation.create_conversation_async]. This should be
 * called from the callback of that function to get the result of whether or
 * not the conversation was created successfully.
 *
 * It is the responsibility of the protocol to register the conversation with
 * the default [class@ConversationManager].
 *
 * Returns: (transfer full): The new conversation if successful, otherwise
 *          %NULL with @error possibly set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversation *purple_protocol_conversation_create_conversation_finish(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

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
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
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
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_conversation_send_message_finish(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

/**
 * purple_protocol_conversation_set_topic_async:
 * @protocol: The instance.
 * @conversation: The conversation whose topic to set.
 * @topic: (nullable): The new topic to set.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (nullable) (scope async): The callback to call after the message
 *            has been sent.
 * @data: (nullable): Optional user data to pass to @callback.
 *
 * Starts the process of setting the topic of @conversation to @topic.
 *
 * It is up to the protocol to define how [property@Conversation:topic] is
 * updated. It may be able to do this immediately based on a result from the
 * other end, or it might have to wait until another event comes in telling it
 * to update it. Regardless, user interfaces should not be updating the topic
 * directly.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_conversation_set_topic_async(PurpleProtocolConversation *protocol, PurpleConversation *conversation, const char *topic, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_conversation_set_topic_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolConversation.set_topic_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to
 * [method@ProtocolConversation.set_topic_async]. This should be called from
 * the callback of that function to get the result of whether or not the
 * message was sent successfully.
 *
 * Returns: %TRUE if the message was sent successfully, otherwise %FALSE with
 *          @error possibly set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_conversation_set_topic_finish(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

/**
 * purple_protocol_conversation_get_channel_join_details:
 * @protocol: The instance.
 * @account: The account that will be joining a channel.
 *
 * User interfaces will use this function to get an instance of
 * [class@ChannelJoinDetails] that can be presented to a user for them to edit.
 *
 * Returns: (transfer full): The new join channel details.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleChannelJoinDetails *purple_protocol_conversation_get_channel_join_details(PurpleProtocolConversation *protocol, PurpleAccount *account);

/**
 * purple_protocol_conversation_join_channel_async:
 * @protocol: The instance.
 * @account: The account that's joining the channel.
 * @details: The details of the channel that's being joined.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (nullable) (scope async): The callback to call after the message
 *            has been sent.
 * @data: (nullable): Optional user data to pass to @callback.
 *
 * Attempts to join the channel identified by @details using @account.
 *
 * If the channel is joined successfully, it is the responsibility of
 * @protocol to add the conversation to the [class@ConversationManager] during
 * this process.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_conversation_join_channel_async(PurpleProtocolConversation *protocol, PurpleAccount *account, PurpleChannelJoinDetails *details, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_conversation_join_channel_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolConversation.join_channel_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to
 * [method@ProtocolConversation.join_channel_async]. This should be called from
 * the callback of that function to get the result of whether or not the
 * channel was joined successfully.
 *
 * Returns: %TRUE if the channel was joined successfully, otherwise %FALSE with
 *          @error possibly set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_conversation_join_channel_finish(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

/**
 * purple_protocol_conversation_set_avatar_async:
 * @protocol: The instance.
 * @conversation: The conversation instance.
 * @avatar: (nullable): The new avatar.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (nullable) (scope async): The callback to call after the message
 *            has been sent.
 * @data: (nullable): Optional user data to pass to @callback.
 *
 * Sets the avatar for @conversation to @pixbuf. Pass %NULL to clear the
 * current avatar.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_conversation_set_avatar_async(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleAvatar *avatar, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_conversation_set_avatar_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolConversation.set_avatar_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to
 * [method@ProtocolConversation.set_avatar_async]. This should be called from
 * the callback of that function to get the result of whether or not the
 * avatar was set successfully.
 *
 * Returns: %TRUE if the avatar was set successfully, otherwise %FALSE with
 *          @error possibly set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_conversation_set_avatar_finish(PurpleProtocolConversation *protocol, GAsyncResult *result, GError **error);

/**
 * purple_protocol_conversation_send_typing:
 * @protocol: The instance.
 * @conversation: The conversation.
 * @state: The new typing state.
 *
 * Sends the libpurple user's typing state for the conversation.
 *
 * This should primarily be called by the user interface.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_conversation_send_typing(PurpleProtocolConversation *protocol, PurpleConversation *conversation, PurpleTypingState state);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_CONVERSATION_H */
