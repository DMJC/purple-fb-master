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

#ifndef PURPLE_MESSAGE_H
#define PURPLE_MESSAGE_H

#include <glib-object.h>

#include "purpleattachment.h"
#include "purplecontactinfo.h"
#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_MESSAGE purple_message_get_type()

/**
 * PurpleMessageFlags:
 * @PURPLE_MESSAGE_SEND: Outgoing message.
 * @PURPLE_MESSAGE_RECV: Incoming message.
 * @PURPLE_MESSAGE_SYSTEM: System message.
 * @PURPLE_MESSAGE_AUTO_RESP: Auto response.
 * @PURPLE_MESSAGE_ACTIVE_ONLY: Hint to the UI that this message should not be
 *                              shown in conversations which are only open for
 *                              internal UI purposes (e.g. for contact-aware
 *                              conversations).
 * @PURPLE_MESSAGE_NICK: Contains your nick.
 * @PURPLE_MESSAGE_NO_LOG: Do not log.
 * @PURPLE_MESSAGE_ERROR: Error message.
 * @PURPLE_MESSAGE_DELAYED: Delayed message.
 * @PURPLE_MESSAGE_RAW: "Raw" message - don't apply formatting.
 * @PURPLE_MESSAGE_NOTIFY: Message is a notification.
 * @PURPLE_MESSAGE_NO_LINKIFY: Message should not be auto-linkified.
 *                             Since: 2.1
 * @PURPLE_MESSAGE_INVISIBLE: Message should not be displayed.
 *                            Since: 2.2
 * @PURPLE_MESSAGE_FORWARDED: The message has been forward to the recipient.
 *                            Since: 3.0
 *
 * Flags applicable to a message. Most will have send, recv or system.
 */
typedef enum /*< flags >*/
{
	PURPLE_MESSAGE_SEND         = 1 << 0,
	PURPLE_MESSAGE_RECV         = 1 << 1,
	PURPLE_MESSAGE_SYSTEM       = 1 << 2,
	PURPLE_MESSAGE_AUTO_RESP    = 1 << 3,
	PURPLE_MESSAGE_ACTIVE_ONLY  = 1 << 4,
	PURPLE_MESSAGE_NICK         = 1 << 5,
	PURPLE_MESSAGE_NO_LOG       = 1 << 6,
	PURPLE_MESSAGE_ERROR        = 1 << 7,
	PURPLE_MESSAGE_DELAYED      = 1 << 8,
	PURPLE_MESSAGE_RAW          = 1 << 9,
	PURPLE_MESSAGE_NOTIFY       = 1 << 11,
	PURPLE_MESSAGE_NO_LINKIFY PURPLE_AVAILABLE_ENUMERATOR_IN_2_1 = 1 << 12,
	PURPLE_MESSAGE_INVISIBLE PURPLE_AVAILABLE_ENUMERATOR_IN_2_2 = 1 << 13,
	PURPLE_MESSAGE_FORWARDED PURPLE_AVAILABLE_ENUMERATOR_IN_3_0 = 1 << 15,
} PurpleMessageFlags;

/**
 * PurpleMessage:
 *
 * #PurpleMessage represents any message passed between users in libpurple.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleMessage, purple_message, PURPLE, MESSAGE, GObject)

/**
 * purple_message_new:
 * @author: The author.
 * @contents: The contents of the message.
 *
 * Creates a new message from @author with @contents.
 *
 * Returns: (transfer full): The new message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleMessage *purple_message_new(PurpleContactInfo *author, const char *contents);

/**
 * purple_message_new_outgoing:
 * @author: The author.
 * @contents: The contents.
 * @flags: The #PurpleMessageFlags.
 *
 * Creates new outgoing message to @recipient.
 *
 * You don't need to set the #PURPLE_MESSAGE_SEND flag.  If the message is not
 * plain text be sure to call purple_message_set_content_type().
 *
 * Returns: (transfer full): The new #PurpleMessage instance.
 *
 * Since: 3.0
 */
PURPLE_DEPRECATED_FOR(purple_message_new)
PurpleMessage *purple_message_new_outgoing(const char *author, const char *contents, PurpleMessageFlags flags);

/**
 * purple_message_new_incoming:
 * @who: Message's author.
 * @contents: The contents of a message.
 * @flags: The message flags.
 * @timestamp: The time of transmitting a message. May be 0 for a current time.
 *
 * Creates new incoming message (the user is the recipient).
 *
 * You don't need to set the #PURPLE_MESSAGE_RECV flag.
 *
 * Returns: the new #PurpleMessage.
 *
 * Since: 3.0
 */
PURPLE_DEPRECATED_FOR(purple_message_new)
PurpleMessage *purple_message_new_incoming(const char *who, const char *contents, PurpleMessageFlags flags, guint64 timestamp);

/**
 * purple_message_new_system:
 * @contents: The contents of a message.
 * @flags: The message flags.
 *
 * Creates new system message.
 *
 * You don't need to set the #PURPLE_MESSAGE_SYSTEM flag.
 *
 * Returns: the new #PurpleMessage.
 *
 * Since: 3.0
 */
PURPLE_DEPRECATED_FOR(purple_message_new)
PurpleMessage *purple_message_new_system(const char *contents, PurpleMessageFlags flags);

/**
 * purple_message_get_action:
 * @message: The instance.
 *
 * Gets whether or not @message is an action.
 *
 * See also [property@Message:action] for more information.
 *
 * Returns: %TRUE if @message is an action, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_get_action(PurpleMessage *message);

/**
 * purple_message_set_action:
 * @message: The instance.
 * @action: Whether or not @message is an action.
 *
 * Sets whether or not @message is an action.
 *
 * See also [property@Message:action] for more information.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_action(PurpleMessage *message, gboolean action);

/**
 * purple_message_get_author:
 * @message: The message.
 *
 * Returns the author of the message, not a local alias.
 *
 * Returns: the author of @message.
 *
 * Deprecated: 3.0: this is being repurposed in the future.
 */
PURPLE_DEPRECATED_FOR(purple_message_get_author_name)
const char *purple_message_get_author(PurpleMessage *message);

/**
 * purple_message_get_author_name:
 * @message: The message.
 *
 * Returns the author of the message, not a local alias.
 *
 * Returns: the author of @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_message_get_author_name(PurpleMessage *message);

/**
 * purple_message_get_author_name_color:
 * @message: The #PurpleMessage instance.
 *
 * Gets the author's name color for @message.
 *
 * Returns: (transfer none): The hex color for the author of @message's name.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_message_get_author_name_color(PurpleMessage *message);

/**
 * purple_message_set_author_name_color:
 * @message: The #PurpleMessage instance.
 * @color: The hex color code for the author of @message.
 *
 * Sets the author's name color of @message to @color. This is the color that
 * will be used to display the author's name in a user interface. The user
 * interface might not use this exact color, as it might need to adapt for
 * contrast or limits on the number of colors.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_author_name_color(PurpleMessage *message, const char *color);

/**
 * purple_message_get_author_alias:
 * @message: The message.
 *
 * Returns the alias of @message author.
 *
 * Returns: the @message author's alias.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_message_get_author_alias(PurpleMessage *message);

/**
 * purple_message_set_author_alias:
 * @message: The message.
 * @alias: The alias.
 *
 * Sets the alias of @message's author. You don't normally need to call this.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_author_alias(PurpleMessage *message, const char *alias);

/**
 * purple_message_get_contents:
 * @message: The message.
 *
 * Returns the contents of the message.
 *
 * Returns: the contents of @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_message_get_contents(PurpleMessage *message);

/**
 * purple_message_set_contents:
 * @message: The message.
 * @cont: The contents.
 *
 * Sets the contents of the @message. It might be HTML.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_contents(PurpleMessage *message, const char *cont);

/**
 * purple_message_get_delivered:
 * @message: The instance.
 *
 * Gets whether or not the message was delivered.
 *
 * > Note: If a protocol does not support delivery receipts this will always
 * return %FALSE.
 *
 * If the protocol does support delivery receipts, [property@Message:error] may
 * be set as well if delivery failed.
 *
 * Returns: %TRUE if @message has been delivered and the protocol supports
 *          delivery notifications, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_get_delivered(PurpleMessage *message);

/**
 * purple_message_set_delivered:
 * @message: The instance.
 * @delivered: Whether or not the message was delivered.
 *
 * Sets whether or not a message was delivered.
 *
 * > Note: Setting this will also set [property@Message:delivered-at]. If
 * @delivered is %TRUE it will be set to the current time, otherwise it will be
 * unset.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_delivered(PurpleMessage *message, gboolean delivered);

/**
 * purple_message_get_delivered_at:
 * @message: The instance.
 *
 * Gets the time that @message was delivered if it was delivered, otherwise
 * %NULL.
 *
 * Returns: (transfer none) (nullable): The delivery time of @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GDateTime *purple_message_get_delivered_at(PurpleMessage *message);

/**
 * purple_message_set_delivered_at:
 * @message: The instance.
 * @datetime: (nullable): The time of delivery.
 *
 * Sets the delivery time of @message to @datetime.
 *
 * > Note: Setting this will also set [property@Message:delivered]. If
 * @datetime is %NULL it will be set to %FALSE, otherwise %TRUE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_delivered_at(PurpleMessage *message, GDateTime *datetime);

/**
 * purple_message_get_edited:
 * @message: The instance.
 *
 * Gets whether or not @message has been edited.
 *
 * Returns: %TRUE if edited, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_get_edited(PurpleMessage *message);

/**
 * purple_message_set_edited:
 * @message: The instance.
 * @edited: The new edited state.
 *
 * Sets the edited state of @message to @edited.
 *
 * > Note: Setting this will also set [property@Message:edited-at]. If
 * @edited is %TRUE it will be set to the current time, otherwise it will be
 * unset.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_edited(PurpleMessage *message, gboolean edited);

/**
 * purple_message_get_event:
 * @message: The instance.
 *
 * Gets whether or not @message is for an event.
 *
 * Returns: %TRUE if the message is for an event, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_get_event(PurpleMessage *message);

/**
 * purple_message_set_event:
 * @message: The instance.
 * @event: The new event state.
 *
 * Sets whether or not @message is for an event.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_event(PurpleMessage *message, gboolean event);

/**
 * purple_message_get_edited_at:
 * @message: The instance.
 *
 * Gets the time that @message was last edited. If @message has never been
 * edited this will be %NULL.
 *
 * Returns: (transfer none) (nullable): The last edit time of @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GDateTime *purple_message_get_edited_at(PurpleMessage *message);

/**
 * purple_message_set_edited_at:
 * @message: The instance.
 * @datetime: (nullable): The time of the last edit.
 *
 * Sets the last edit time of @message to @datetime.
 *
 * > Note: Setting this will also set [property@Message:edited]. If
 * @datetime is %NULL it will be set to %FALSE, otherwise %TRUE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_edited_at(PurpleMessage *message, GDateTime *datetime);

/**
 * purple_message_get_error:
 * @message: The instance.
 *
 * Gets the error from @message.
 *
 * Returns: (nullable) (transfer none): The error from @message or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GError *purple_message_get_error(PurpleMessage *message);

/**
 * purple_message_set_error:
 * @message: The instance.
 * @error: (nullable) (transfer none): The error to set.
 *
 * Sets the error of @message to @error. Primarily this will be used for
 * delivery failure.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_error(PurpleMessage *message, GError *error);

/**
 * purple_message_get_flags:
 * @message: The message.
 *
 * Returns the flags of a @message.
 *
 * Returns: the flags of a @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleMessageFlags purple_message_get_flags(PurpleMessage *message);

/**
 * purple_message_set_flags:
 * @message: The message.
 * @flags: The message flags.
 *
 * Sets flags for @message. It shouldn't be in a conflict with a message type,
 * so use it carefully.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_flags(PurpleMessage *message, PurpleMessageFlags flags);

/**
 * purple_message_get_id:
 * @message: The message.
 *
 * Returns the unique identifier of the message. These identifiers are not
 * serialized - it's a per-session id.
 *
 * Returns: the global identifier of @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_message_get_id(PurpleMessage *message);

/**
 * purple_message_set_id:
 * @message: The instance.
 * @id: (nullable): The new id to set.
 *
 * Sets the id of @message to @id.
 *
 * > Note: This should really only be used by protocol plugins to update an id
 * of a sent message when the server has assigned the final id to the message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_id(PurpleMessage *message, const char *id);

/**
 * purple_message_is_empty:
 * @message: The message.
 *
 * Checks, if the message's body is empty.
 *
 * Returns: %TRUE, if @message is empty.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_is_empty(PurpleMessage *message);

/**
 * purple_message_get_notice:
 * @message: The instance.
 *
 * Gets whether or not @message is a notice.
 *
 * Returns: %TRUE if @message is a notice, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_get_notice(PurpleMessage *message);

/**
 * purple_message_set_notice:
 * @message: The instance.
 * @notice: %TRUE if this message is a NOTICE.
 *
 * Sets whether or not @message is a notice.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_notice(PurpleMessage *message, gboolean notice);

/**
 * purple_message_get_system:
 * @message: The instance.
 *
 * Gets whether or not @message is a system message.
 *
 * Returns: %TRUE if @message is a system message, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_get_system(PurpleMessage *message);

/**
 * purple_message_set_system:
 * @message: The instance.
 * @system: %TRUE to mark @message as a system message.
 *
 * Sets whether or not @message is a system message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_system(PurpleMessage *message, gboolean system);

/**
 * purple_message_get_timestamp:
 * @message: The message.
 *
 * Returns a @message's timestamp.  If @message does not currently have a
 * timestamp, the current time will be set as the time stamp and returned.
 *
 * Returns: (transfer none): The #GDateTime timestamp from @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GDateTime *purple_message_get_timestamp(PurpleMessage *message);

/**
 * purple_message_set_timestamp:
 * @message: The #PurpleMessage instance.
 * @timestamp: (nullable): The #GDateTime of the message.
 *
 * Sets the timestamp of @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_timestamp(PurpleMessage *message, GDateTime *timestamp);

/**
 * purple_message_set_timestamp_now:
 * @message: The instance.
 *
 * Calls [method@Message.set_timestamp] with the current UTC time.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_set_timestamp_now(PurpleMessage *message);

/**
 * purple_message_format_timestamp:
 * @message: The #PurpleMessage instance.
 * @format: The format to output the time stamp as.
 *
 * Formats the timestamp of @message and returns it.
 *
 * Returns: The formatted timestamp.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
char *purple_message_format_timestamp(PurpleMessage *message, const char *format);

/**
 * purple_message_add_attachment:
 * @message: The #PurpleMessage instance.
 * @attachment: The #PurpleAttachment instance.
 *
 * Adds @attachment to @message.
 *
 * Returns: %TRUE if an attachment with the same ID did not already exist.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_add_attachment(PurpleMessage *message, PurpleAttachment *attachment);

/**
 * purple_message_remove_attachment:
 * @message: The #PurpleMessage instance.
 * @id: The id of the #PurpleAttachment
 *
 * Removes the #PurpleAttachment identified by @id if it exists.
 *
 * Returns: %TRUE if the #PurpleAttachment was found and removed, %FALSE
 *          otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_message_remove_attachment(PurpleMessage *message, guint64 id);

/**
 * purple_message_get_attachment:
 * @message: The #PurpleMessage instance.
 * @id: The id of the #PurpleAttachment to get.
 *
 * Retrieves the #PurpleAttachment identified by @id from @message.
 *
 * Returns: (transfer full): The #PurpleAttachment if it was found, otherwise
 *                           %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAttachment *purple_message_get_attachment(PurpleMessage *message, guint64 id);

/**
 * purple_message_foreach_attachment:
 * @message: The #PurpleMessage instance.
 * @func: (scope call): The #PurpleAttachmentForeachFunc to call.
 * @data: User data to pass to @func.
 *
 * Calls @func for each #PurpleAttachment that's attached to @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_foreach_attachment(PurpleMessage *message, PurpleAttachmentForeachFunc func, gpointer data);

/**
 * purple_message_clear_attachments:
 * @message: The #PurpleMessage instance.
 *
 * Removes all attachments from @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_message_clear_attachments(PurpleMessage *message);

G_END_DECLS

#endif /* PURPLE_MESSAGE_H */
