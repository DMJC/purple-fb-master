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

#ifndef PURPLE_MESSAGES_H
#define PURPLE_MESSAGES_H

#include <glib.h>
#include <gio/gio.h>

#include "purpleconversation.h"
#include "purplemessage.h"
#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_MESSAGES (purple_messages_get_type())

/**
 * PurpleMessages:
 *
 * A read-only collection of [class@Message]'s and the [class@Conversation]
 * that they belong to.
 *
 * This collection is meant to make it easy to pass around a number of related
 * messages. For example getting messages from a server or displaying a few
 * messages in a search result.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleMessages, purple_messages, PURPLE, MESSAGES,
                     GObject)

/**
 * purple_messages_new:
 * @conversation: The conversation.
 *
 * Creates a new instance for @conversation.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleMessages *purple_messages_new(PurpleConversation *conversation);

/**
 * purple_messages_get_conversation:
 * @messages: The instance.
 *
 * Gets the conversation from @messages.
 *
 * Returns: (transfer none): The conversation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversation *purple_messages_get_conversation(PurpleMessages *messages);

/**
 * purple_messages_add:
 * @messages: The instance.
 * @message: The message to add.
 *
 * Adds @message to @messages.
 *
 * @message will be sorted inserted according to @message's created timestamp
 * and no duplication checking is performed.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_messages_add(PurpleMessages *messages, PurpleMessage *message);

G_END_DECLS

#endif /* PURPLE_MESSAGES_H */
