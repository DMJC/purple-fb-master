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

#ifndef PURPLE_CONVERSATION_MEMBERS_H
#define PURPLE_CONVERSATION_MEMBERS_H

#include <glib.h>
#include <gio/gio.h>

#include "purpleconversationmember.h"
#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_CONVERSATION_MEMBERS (purple_conversation_members_get_type())

/**
 * PurpleConversationMembers:
 *
 * A collection of [class@ConversationMember]s.
 *
 * This collection will notify the properties of each
 * [class@ConversationMember] and emit the
 * [signal@Gio.ListModel::items-changed] signal when any of them changes.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleConversationMembers, purple_conversation_members,
                     PURPLE, CONVERSATION_MEMBERS, GObject)

/**
 * purple_conversation_members_add_member:
 * @members: The instance.
 * @info: The [class@ContactInfo] of the person joining.
 * @announce: Whether this addition should be announced or not.
 * @message: (nullable): An optional message to be used with @announce.
 *
 * Looks for an existing [class@ConversationMember] for @info in
 * @members and returns it if found. If not, a new [class@ConversationMember]
 * is created.
 *
 * > This method is intended to be called by a protocol plugin to directly
 * > manage the membership state of the @conversation.
 *
 * This will also emit the [signal@ConversationMembers::member-added] signal if
 * an existing member was not found.
 *
 * The @announce and @message parameters will be used when emitting the
 * [signal@ConversationMembers::member-added] signal. Announce and message are
 * meant for protocols to more properly define their behavior. For example, on
 * IRC you will typically be told when a user joins a chat but on Twitch this
 * isn't announced.
 *
 * Returns: (transfer none): The [class@ConversationMember] that was created or
 *          found.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversationMember *purple_conversation_members_add_member(PurpleConversationMembers *members, PurpleContactInfo *info, gboolean announce, const char *message);

/**
 * purple_conversation_members_find_member:
 * @members: The instance.
 * @info: A [class@ContactInfo instance]
 *
 * Finds the [class@ConversationMember] for @info if they are in @members.
 *
 * Returns: (transfer none) (nullable): The [class@ConversationMember]
 *          if found or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversationMember *purple_conversation_members_find_member(PurpleConversationMembers *members, PurpleContactInfo *info);

/**
 * purple_conversation_members_has_member:
 * @members: The instance.
 * @info: The [class@ContactInfo] to look for.
 * @position: (out) (optional): A return address for the position of the member
 *            in the collection.
 *
 * Checks if @info is in @members.
 *
 * If @info is found, and @position is not %NULL, it will be set to the
 * position of @info in @members.
 *
 * Returns: %TRUE if @info is in @members otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_members_has_member(PurpleConversationMembers *members, PurpleContactInfo *info, guint *position);

/**
 * purple_conversation_members_new:
 *
 * Creates a new instance.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversationMembers *purple_conversation_members_new(void);

/**
 * purple_conversation_members_remove_member:
 * @members: The instance.
 * @info: The [class@ContactInfo] of the person leaving.
 * @announce: Whether or not this removal should be announced.
 * @message: (nullable): An optional message for the announcement.
 *
 * Attempts to remove @info from the @members.
 *
 * If found, @info is removed and the
 * [signal@ConversationMembers::member-removed] signal is emitted with
 * @announce and @message as parameters.
 *
 * > This method is intended to be called by a protocol plugin to directly
 * > manage the membership state of the @conversation.
 *
 * Returns: %TRUE if @member was found and removed from @members, otherwise
 *          %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_conversation_members_remove_member(PurpleConversationMembers *members, PurpleContactInfo *info, gboolean announce, const char *message);

G_END_DECLS

#endif /* PURPLE_CONVERSATION_MEMBERS_H */

