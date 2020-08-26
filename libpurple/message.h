/* purple
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301 USA
 */

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_MESSAGE_H
#define PURPLE_MESSAGE_H
/**
 * SECTION:message
 * @include:message.h
 * @section_id: libpurple-message
 * @short_description: serializable messages
 * @title: Message model
 *
 * #PurpleMessage object collects data about a certain (incoming or outgoing) message.
 * It (TODO: will be) serializable, so it can be stored in log and retrieved
 * with any metadata.
 */

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * PURPLE_TYPE_MESSAGE:
 *
 * The standard _get_type macro for #PurpleMessage.
 */
#define PURPLE_TYPE_MESSAGE  purple_message_get_type()

/**
 * purple_message_get_type:
 *
 * Returns: the #GType for a message.
 */
G_DECLARE_FINAL_TYPE(PurpleMessage, purple_message, PURPLE, MESSAGE, GObject)

/* conversations.h depends on PurpleMessage and currently PurpleMessageFlag is
 * in conversations.h.
 */
#include <conversation.h>

/**
 * purple_message_new_outgoing:
 * @who: Message's recipient.
 * @contents: The contents of a message.
 * @flags: The message flags.
 *
 * Creates new outgoing message (the user is the author).
 *
 * You don't need to set the #PURPLE_MESSAGE_SEND flag.
 *
 * Returns: the new #PurpleMessage.
 */
PurpleMessage *
purple_message_new_outgoing(const gchar *who, const gchar *contents,
	PurpleMessageFlags flags);

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
 */
PurpleMessage *
purple_message_new_incoming(const gchar *who, const gchar *contents,
	PurpleMessageFlags flags, guint64 timestamp);

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
 */
PurpleMessage *
purple_message_new_system(const gchar *contents, PurpleMessageFlags flags);

/**
 * purple_message_get_id:
 * @msg: The message.
 *
 * Returns the unique identifier of the message. These identifiers are not
 * serialized - it's a per-session id.
 *
 * Returns: the global identifier of @msg.
 */
guint
purple_message_get_id(PurpleMessage *msg);

/**
 * purple_message_find_by_id:
 * @id: The message identifier.
 *
 * Finds the message with a given @id.
 *
 * Returns: (transfer none): The #PurpleMessage, or %NULL if not found.
 */
PurpleMessage *
purple_message_find_by_id(guint id);

/**
 * purple_message_get_author:
 * @msg: The message.
 *
 * Returns the author of the message - his screen name (not a local alias).
 *
 * Returns: the author of @msg.
 */
const gchar *
purple_message_get_author(PurpleMessage *msg);

/**
 * purple_message_get_recipient:
 * @msg: The message.
 *
 * Returns the recipient of the message - his screen name (not a local alias).
 *
 * Returns: the recipient of @msg.
 */
const gchar *
purple_message_get_recipient(PurpleMessage *msg);

/**
 * purple_message_set_author_alias:
 * @msg: The message.
 * @alias: The alias.
 *
 * Sets the alias of @msg's author. You don't normally need to call this.
 */
void
purple_message_set_author_alias(PurpleMessage *msg, const gchar *alias);

/**
 * purple_message_get_author_alias:
 * @msg: The message.
 *
 * Returns the alias of @msg author.
 *
 * Returns: the @msg author's alias.
 */
const gchar *
purple_message_get_author_alias(PurpleMessage *msg);

/**
 * purple_message_set_contents:
 * @msg: The message.
 * @cont: The contents.
 *
 * Sets the contents of the @msg. It might be HTML.
 */
void
purple_message_set_contents(PurpleMessage *msg, const gchar *cont);

/**
 * purple_message_get_contents:
 * @msg: The message.
 *
 * Returns the contents of the message.
 *
 * Returns: the contents of @msg.
 */
const gchar *
purple_message_get_contents(PurpleMessage *msg);

/**
 * purple_message_is_empty:
 * @msg: The message.
 *
 * Checks, if the message's body is empty.
 *
 * Returns: %TRUE, if @msg is empty.
 */
gboolean
purple_message_is_empty(PurpleMessage *msg);

/**
 * purple_message_set_time:
 * @msg: The message.
 * @msgtime: The timestamp of a message.
 *
 * Sets the @msg's timestamp. It should be a date of posting, but it can be
 * a date of receiving (if the former is not available).
 */
void
purple_message_set_time(PurpleMessage *msg, guint64 msgtime);

/**
 * purple_message_get_time:
 * @msg: The message.
 *
 * Returns a @msg's timestamp.
 *
 * Returns: @msg's timestamp.
 */
guint64
purple_message_get_time(PurpleMessage *msg);

/**
 * purple_message_set_flags:
 * @msg: The message.
 * @flags: The message flags.
 *
 * Sets flags for @msg. It shouldn't be in a conflict with a message type,
 * so use it carefully.
 */
void
purple_message_set_flags(PurpleMessage *msg, PurpleMessageFlags flags);

/**
 * purple_message_get_flags:
 * @msg: The message.
 *
 * Returns the flags of a @msg.
 *
 * Returns: the flags of a @msg.
 */
PurpleMessageFlags
purple_message_get_flags(PurpleMessage *msg);

G_END_DECLS

#endif /* PURPLE_MESSAGE_H */
