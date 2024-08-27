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

#ifndef PURPLE_CONVERSATION_H
#define PURPLE_CONVERSATION_H

#include <glib.h>
#include <glib-object.h>

#include "purpleconversationmembers.h"
#include "purpletyping.h"
#include "purpleversion.h"

/**
 * PURPLE_CONVERSATION_DOMAIN:
 *
 * A GError domain for PurpleConversation.
 *
 * Since: 3.0
 */
#define PURPLE_CONVERSATION_DOMAIN \
	(g_quark_from_static_string("purple-conversatin")) \
	PURPLE_AVAILABLE_MACRO_IN_3_0

#define PURPLE_TYPE_CONVERSATION (purple_conversation_get_type())

PURPLE_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE(PurpleConversation, purple_conversation, PURPLE,
                     CONVERSATION, GObject)

#include "purpleavatar.h"
#include "purplecontactinfo.h"
#include "purpleconversationmember.h"
#include "purplemessage.h"

/**
 * PurpleConversationType:
 * @PURPLE_CONVERSATION_TYPE_UNSET: A value to specify that the property has
 *                                  not been set.
 * @PURPLE_CONVERSATION_TYPE_DM: A direct message between two contacts.
 * @PURPLE_CONVERSATION_TYPE_GROUP_DM: A direct message between a protocol
 *                                     dependent number of contacts.
 * @PURPLE_CONVERSATION_TYPE_CHANNEL: A multi-user chat that is available to
 *                                    all users on the protocol and has
 *                                    features like moderation.
 * @PURPLE_CONVERSATION_TYPE_THREAD: A thread is a subset of messages from a
 *                                   conversation that are related.
 *
 * The type of the conversation. This is mostly ignored, but could be useful in
 * ways we can't imagine right now. If you come up with one in the future,
 * please let us know!
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_TYPE_IN_3_0
typedef enum {
	PURPLE_CONVERSATION_TYPE_UNSET,
	PURPLE_CONVERSATION_TYPE_DM,
	PURPLE_CONVERSATION_TYPE_GROUP_DM,
	PURPLE_CONVERSATION_TYPE_CHANNEL,
	PURPLE_CONVERSATION_TYPE_THREAD,
} PurpleConversationType;

/**
 * PurpleConversation:
 *
 * A core representation of a conversation between two or more people.
 *
 * The conversation can be an IM or a chat.
 *
 * Note: When a conversation is destroyed with the last g_object_unref(), the
 *       specified conversation is removed from the parent window. If this
 *       conversation is the only one contained in the parent window, that
 *       window is also destroyed.
 *
 * Since: 2.0
 */

#include "purpleaccount.h"

G_BEGIN_DECLS

/**
 * purple_conversation_is_dm:
 * @conversation: The instance.
 *
 * Checks if @conversation is a direct message or not.
 *
 * This is a quick helper around manually checking the results of
 * [method@Conversation.get_conversation_type].
 *
 * Returns: %TRUE if @conversation is a direct message, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_is_dm(PurpleConversation *conversation);

/**
 * purple_conversation_is_group_dm:
 * @conversation: The instance.
 *
 * Checks if @conversation is a group direct message or not.
 *
 * This is a quick helper around manually checking the results of
 * [method@Conversation.get_conversation_type].
 *
 * Returns: %TRUE if @conversation is a group direct message, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_is_group_dm(PurpleConversation *conversation);

/**
 * purple_conversation_is_channel:
 * @conversation: The instance.
 *
 * Checks if @conversation is a channel or not.
 *
 * This is a quick helper around manually checking the results of
 * [method@Conversation.get_conversation_type].
 *
 * Returns: %TRUE if @conversation is a channel, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_is_channel(PurpleConversation *conversation);

/**
 * purple_conversation_is_thread:
 * @conversation: The instance.
 *
 * Checks if @conversation is a thread or not.
 *
 * This is a quick helper around manually checking the results of
 * [method@Conversation.get_conversation_type].
 *
 * Returns: %TRUE if @conversation is a thread, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_is_thread(PurpleConversation *conversation);

/**
 * purple_conversation_present:
 * @conversation: The conversation to present
 *
 * Emits [signal@Conversation::present].
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_conversation_present(PurpleConversation *conversation);

/**
 * purple_conversation_get_id:
 * @conversation: The instance.
 *
 * Gets the opaque identifier from @conversation.
 *
 * This value is specific to the protocol and could conflict with the same
 * conversation on other accounts. To get a libpurple unique id see
 * [method@Conversation.get_global_id].
 *
 * Returns: (nullable): The id of @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_conversation_get_id(PurpleConversation *conversation);

/**
 * purple_conversation_get_global_id:
 * @conversation: The instance.
 *
 * Gets a globally unique id for @conversation.
 *
 * This id ties the [property@Conversation:id] to the [class@Account] that the
 * conversation is for.
 *
 * Returns: (transfer full): A globally unique id of @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
char *purple_conversation_get_global_id(PurpleConversation *conversation);

/**
 * purple_conversation_get_conversation_type:
 * @conversation: The instance.
 *
 * Gets the type of @conversation.
 *
 * Returns: The [enum@ConversationType] of @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversationType purple_conversation_get_conversation_type(PurpleConversation *conversation);

/**
 * purple_conversation_get_account:
 * @conversation: The conversation.
 *
 * Returns the specified conversation's purple_account.
 *
 * This purple_account represents the user using purple, not the person the user
 * is having a conversation/chat/flame with.
 *
 * Returns: (transfer none): The conversation's purple_account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleAccount *purple_conversation_get_account(PurpleConversation *conversation);

/**
 * purple_conversation_get_connection:
 * @conversation: The conversation.
 *
 * Returns the specified conversation's purple_connection.
 *
 * Returns: (transfer none): The conversation's purple_connection.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConnection *purple_conversation_get_connection(PurpleConversation *conversation);

/**
 * purple_conversation_set_title:
 * @conversation: The conversation.
 * @title: The title.
 *
 * Sets the specified conversation's title.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_conversation_set_title(PurpleConversation *conversation, const char *title);

/**
 * purple_conversation_get_title:
 * @conversation: The conversation.
 *
 * Returns the specified conversation's title.
 *
 * Returns: The title.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_conversation_get_title(PurpleConversation *conversation);

/**
 * purple_conversation_generate_title:
 * @conversation: The instance.
 *
 * Sets the title for @conversation, which must be a DM or Group DM, to a comma
 * separated string of the display names for each [class@ConversationMember].
 *
 * If @conversation is not a DM or Group DM, no changes will be made.
 *
 * If no members are found, [property@Conversation:title] will not be changed.
 *
 * If the title is updated, [property@Conversation:title-generated] will be
 * updated as well.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_generate_title(PurpleConversation *conversation);

/**
 * purple_conversation_get_title_generated:
 * @conversation: The instance.
 *
 * Gets whether or not the current title was automatically generated via
 * [method@Conversation.generate_title].
 *
 * Returns: %TRUE if the title was automatically generated.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_title_generated(PurpleConversation *conversation);

/**
 * purple_conversation_set_name:
 * @conversation: The conversation.
 * @name: The conversation's name.
 *
 * Sets the specified conversation's name.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_conversation_set_name(PurpleConversation *conversation, const char *name);

/**
 * purple_conversation_get_name:
 * @conversation: The conversation.
 *
 * Returns the specified conversation's name.
 *
 * Returns: The conversation's name. If the conversation is an IM with a
 *          PurpleBuddy, then it's the name of the PurpleBuddy.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_conversation_get_name(PurpleConversation *conversation);

/**
 * purple_conversation_write_message:
 * @conversation: The conversation.
 * @message: The message to write.
 *
 * Writes to a chat or an IM.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_write_message(PurpleConversation *conversation, PurpleMessage *message);

/**
 * purple_conversation_send_message_async:
 * @conversation: The conversation.
 * @message: The message to send.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (nullable) (scope async): The callback to call after the
 *            conversation has been created.
 * @data: (nullable): Optional user data to pass to @callback.
 *
 * Sends @message to @conversation.
 *
 * This will lookup the account and protocol plugin that @conversation belongs
 * to and call [method@ProtocolConversation.send_message_async] for @message.
 *
 * You will need to call [method@Conversation.send_message_finish] from
 * @callback.
 *
 * This will also reset [property@Conversation:typing-state] to `none`.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_send_message_async(PurpleConversation *conversation, PurpleMessage *message, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_conversation_send_message_finish:
 * @conversation: The instance.
 * @result: The result.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@Conversation.send_message_async].
 *
 * Returns: %TRUE on success, otherwise %FALSE with error set.
 *
 * Since: 3.0
 */
gboolean purple_conversation_send_message_finish(PurpleConversation *conversation, GAsyncResult *result, GError **error);

/**
 * purple_conversation_has_focus:
 * @conversation: The conversation.
 *
 * Determines if a conversation has focus
 *
 * Returns: %TRUE if the conversation has focus, %FALSE if
 * it does not or the UI does not have a concept of conversation focus
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_conversation_has_focus(PurpleConversation *conversation);

/**
 * purple_conversation_get_age_restricted:
 * @conversation: The instance.
 *
 * Gets whether or not @conversation is age restricted.
 *
 * Returns: %TRUE if age restricted, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_age_restricted(PurpleConversation *conversation);

/**
 * purple_conversation_set_age_restricted:
 * @conversation: The instance.
 * @age_restricted: The new value.
 *
 * Sets whether or not @conversation is age restricted to the value of
 * @age_restricted.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_age_restricted(PurpleConversation *conversation, gboolean age_restricted);

/**
 * purple_conversation_get_description:
 * @conversation: The instance.
 *
 * Gets the description of @conversation.
 *
 * Returns: The description of @conversation or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_conversation_get_description(PurpleConversation *conversation);

/**
 * purple_conversation_set_description:
 * @conversation: The instance.
 * @description: (nullable): The new description.
 *
 * Sets the description of @conversation to @description.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_description(PurpleConversation *conversation, const char *description);

/**
 * purple_conversation_get_topic:
 * @conversation: The instance.
 *
 * Gets the topic for @conversation.
 *
 * Returns: (nullable): The topic of @conversation or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_conversation_get_topic(PurpleConversation *conversation);

/**
 * purple_conversation_set_topic:
 * @conversation: The instance.
 * @topic: (nullable): The new topic.
 *
 * Sets the topic of @conversation to @topic.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_topic(PurpleConversation *conversation, const char *topic);

/**
 * purple_conversation_set_topic_full:
 * @conversation: The instance:
 * @topic: (nullable): The new topic.
 * @author: (nullable): The author of the new topic.
 * @updated: (nullable): The date time of the new topic.
 *
 * Sets everything about the topic all at once.
 *
 * See also [method@Conversation.set_topic],
 * [method@Conversation.set_topic_author], and
 * [method@Conversation.set_topic_updated].
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_topic_full(PurpleConversation *conversation, const char *topic, PurpleContactInfo *author, GDateTime *updated);

/**
 * purple_conversation_get_topic_author:
 * @conversation: The instance.
 *
 * Gets the author of the topic for @conversation.
 *
 * Returns: (transfer none) (nullable): The author of the topic or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleContactInfo *purple_conversation_get_topic_author(PurpleConversation *conversation);

/**
 * purple_conversation_set_topic_author:
 * @conversation: The instance.
 * @author: (nullable): The new author of the topic.
 *
 * Sets the author of the topic for @conversation to @author.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_topic_author(PurpleConversation *conversation, PurpleContactInfo *author);

/**
 * purple_conversation_get_topic_updated:
 * @conversation: The instance.
 *
 * Gets the last time that the topic was updated.
 *
 * Returns: (transfer none) (nullable): The last time the topic was updated or
 *          %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GDateTime *purple_conversation_get_topic_updated(PurpleConversation *conversation);

/**
 * purple_conversation_set_topic_updated:
 * @conversation: The instance.
 * @updated: (nullable): The time of the last update.
 *
 * Sets the time that the topic was lasted updated for @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_topic_updated(PurpleConversation *conversation, GDateTime *updated);

/**
 * purple_conversation_get_user_nickname:
 * @conversation: The instance.
 *
 * Gets the nickname for the user in @conversation.
 *
 * Returns: (nullable): The nickname for the user in @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_conversation_get_user_nickname(PurpleConversation *conversation);

/**
 * purple_conversation_set_user_nickname:
 * @conversation: The instance.
 * @nickname: (nullable): The new nickname for the user.
 *
 * Sets the user's nickname in @conversation to @nickname.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_user_nickname(PurpleConversation *conversation, const char *nickname);

/**
 * purple_conversation_get_favorite:
 * @conversation: The instance.
 *
 * Gets whether or not @conversation is a favorite conversation.
 *
 * Returns: %TRUE if @conversation has been set a as a favorite, otherwise
 *          %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_favorite(PurpleConversation *conversation);

/**
 * purple_conversation_set_favorite:
 * @conversation: The instance.
 * @favorite: %TRUE to mark as a favorite.
 *
 * Sets whether or not @conversation is a favorite to @favorite.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_favorite(PurpleConversation *conversation, gboolean favorite);

/**
 * purple_conversation_get_created_on:
 * @conversation: The instance.
 *
 * Gets a [struct@GLib.DateTime] for when @conversation was created.
 *
 * Returns: (transfer none) (nullable): The creation time of @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GDateTime *purple_conversation_get_created_on(PurpleConversation *conversation);

/**
 * purple_conversation_set_created_on:
 * @conversation: The instance.
 * @created_on: (nullable): The new created on value.
 *
 * Sets the creation time of @conversation to @created_on.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_created_on(PurpleConversation *conversation, GDateTime *created_on);

/**
 * purple_conversation_get_creator:
 * @conversation: The instance.
 *
 * Gets the [class@ContactInfo] that created @conversation.
 *
 * Returns: (transfer none): The creator or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleContactInfo *purple_conversation_get_creator(PurpleConversation *conversation);

/**
 * purple_conversation_set_creator:
 * @conversation: The instance.
 * @creator: (nullable) (transfer none): The new creator.
 *
 * Sets the creator of @conversation to @creator.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_creator(PurpleConversation *conversation, PurpleContactInfo *creator);

/**
 * purple_conversation_get_online:
 * @conversation: The instance.
 *
 * Gets whether or not @conversation is online.
 *
 * A conversation is considered offline when messages cannot be sent or
 * received.
 *
 * Returns: %TRUE if messages can be sent and received from @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_online(PurpleConversation *conversation);

/**
 * purple_conversation_set_online:
 * @conversation: The instance.
 * @online: Whether or not the conversation is online.
 *
 * Sets whether or not the conversation is online, or able to send and receive
 * messages.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_online(PurpleConversation *conversation, gboolean online);

/**
 * purple_conversation_get_federated:
 * @conversation: The instance.
 *
 * Gets whether or not @conversation is federated.
 *
 * Depending on the underlying protocol, conversations can be federated, which
 * means they can be running on a different server. When a protocol creates a
 * federated conversation, it takes the responsibility of managing the
 * [property@Conversation:online] state of the conversation.
 *
 * Returns: %TRUE if @conversation is federated otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_federated(PurpleConversation *conversation);

/**
 * purple_conversation_get_tags:
 * @conversation: The instance.
 *
 * Gets the [class@Tags] object from @conversation.
 *
 * Returns: (transfer none): The tags from @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleTags *purple_conversation_get_tags(PurpleConversation *conversation);

/**
 * purple_conversation_get_members:
 * @conversation: The instance.
 *
 * Gets the members that are in @conversation.
 *
 * Returns: (transfer none): The members in @conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversationMembers *purple_conversation_get_members(PurpleConversation *conversation);

/**
 * purple_conversation_get_messages:
 * @conversation: The instance.
 *
 * Gets the list of all messages in @conversation.
 *
 * Returns: (transfer none): The list of messages.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GListModel *purple_conversation_get_messages(PurpleConversation *conversation);

/**
 * purple_conversation_get_avatar:
 * @conversation: The instance.
 *
 * Gets the [class@Avatar] from @conversation if set.
 *
 * > Note: Not all protocols support this and user interfaces generally use
 * the avatar of a contact info for a direct message.
 *
 * Returns: (transfer none) (nullable): The avatar for the conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAvatar *purple_conversation_get_avatar(PurpleConversation *conversation);

/**
 * purple_conversation_set_avatar:
 * @conversation: The instance.
 * @avatar: (nullable): The new avatar to set.
 *
 * Sets the avatar of @conversation to @avatar.
 *
 * > Note: Not all protocols support this and user interfaces generally use
 * the avatar of a contact info for a direct message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_avatar(PurpleConversation *conversation, PurpleAvatar *avatar);

/**
 * purple_conversation_get_title_for_display:
 * @conversation: The instance.
 *
 * Gets the title that should be displayed for @conversation.
 *
 * A full breakdown for how this works can be found in the documentation for
 * [property@Conversation:title-for-display].
 *
 * Returns: The title to display.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_conversation_get_title_for_display(PurpleConversation *conversation);

/**
 * purple_conversation_get_alias:
 * @conversation: The instance.
 *
 * Gets the libpurple user defined alias for @conversation.
 *
 * Returns: (nullable): The alias or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_conversation_get_alias(PurpleConversation *conversation);

/**
 * purple_conversation_set_alias:
 * @conversation: The instance.
 * @alias: (nullable): The new alias.
 *
 * Sets the libpurple user defined alias of @conversation to @alias.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_alias(PurpleConversation *conversation, const char *alias);

/**
 * purple_conversation_get_needs_attention:
 * @conversation: The instance.
 *
 * Gets whether or not @conversation needs attention.
 *
 * Returns: %TRUE if @conversation needs attention, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_needs_attention(PurpleConversation *conversation);

/**
 * purple_conversation_set_needs_attention:
 * @conversation: The instance.
 * @needs_attention: %TRUE to mark @conversation as needing attention.
 *
 * Marks @conversation as needing attention or not.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_needs_attention(PurpleConversation *conversation, gboolean needs_attention);

/**
 * purple_conversation_get_typing_state:
 * @conversation: The instance.
 *
 * Gets the [enum@TypingState] for the libpurple user in @conversation.
 *
 * Returns: The typing state of the libpurple user.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleTypingState purple_conversation_get_typing_state(PurpleConversation *conversation);

/**
 * purple_conversation_set_typing_state:
 * @conversation: The instance.
 * @typing_state: The new state.
 *
 * Sets the [enum@TypingState] of the libpurple user in @conversation to
 * @typing_state.
 *
 * This will also call [method@ProtocolConversation.send_typing] if it is
 * implemented.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_typing_state(PurpleConversation *conversation, PurpleTypingState typing_state);

/**
 * purple_conversation_get_logging:
 * @conversation: The instance.
 *
 * Gets whether or not @conversation is currently being logged.
 *
 * Returns: %TRUE if @conversation is being logged, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_get_logging(PurpleConversation *conversation);

/**
 * purple_conversation_set_logging:
 * @conversation: The instance.
 * @logging: The new logging state.
 *
 * Sets the logging state of @conversation to @logging.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_conversation_set_logging(PurpleConversation *conversation, gboolean logging);

G_END_DECLS

#endif /* PURPLE_CONVERSATION_H */
