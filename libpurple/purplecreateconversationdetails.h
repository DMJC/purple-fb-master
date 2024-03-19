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

#ifndef PURPLE_CREATE_CONVERSATION_DETAILS_H
#define PURPLE_CREATE_CONVERSATION_DETAILS_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_CREATE_CONVERSATION_DETAILS (purple_create_conversation_details_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleCreateConversationDetails, purple_create_conversation_details, PURPLE, CREATE_CONVERSATION_DETAILS, GObject)

/**
 * PurpleCreateConversationDetails:
 *
 * The details that are necessary for a [class@Protocol] to create a
 * conversation.
 *
 * This is only used to create direct messages and group direct messages. The
 * user interface will ask a protocol for a [class@CreateConversationDetails]
 * and then fill it out and pass it back to the protocol to actually create a
 * conversation.
 *
 * Since: 3.0
 */

/**
 * purple_create_conversation_details_new:
 * @max_participants: The maximum number of participants. %0 to say unlimited.
 *
 * Creates a new [class@CreateConversationDetails].
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleCreateConversationDetails *purple_create_conversation_details_new(guint max_participants);

/**
 * purple_create_conversation_details_get_max_participants:
 * @details: The instance.
 *
 * Gets the maximum number of participants that are supported not including the
 * libpurple user.
 *
 * Returns: The maximum number of participants.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
guint purple_create_conversation_details_get_max_participants(PurpleCreateConversationDetails *details);

/**
 * purple_create_conversation_details_get_participants:
 * @details: The instance.
 *
 * Gets the participants to add to the conversation.
 *
 * Returns: (transfer none) (nullable): The participants.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GListModel *purple_create_conversation_details_get_participants(PurpleCreateConversationDetails *details);

/**
 * purple_create_conversation_details_set_participants:
 * @details: The instance.
 * @participants: (nullable) (transfer none): The new participants.
 *
 * Sets the participants to @participants.
 *
 * Participants must have an item type of [class@Contact] and the number of
 * items must be less than or equal to
 * [property@CreateConversationDetails:max-participants].
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_create_conversation_details_set_participants(PurpleCreateConversationDetails *details, GListModel *participants);

G_END_DECLS

#endif /* PURPLE_CREATE_CONVERSATION_DETAILS_H */
