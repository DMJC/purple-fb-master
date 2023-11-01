/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#if !defined(PIDGIN_GLOBAL_HEADER_INSIDE) && !defined(PIDGIN_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PIDGIN_CONVERSATION_H
#define PIDGIN_CONVERSATION_H

#include <glib.h>

#include <adwaita.h>

#include <gtk/gtk.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginConversation:
 *
 * A [class@Gtk.Widget] for displaying a [class@Purple.Conversation].
 *
 * Since: 2.0.0
 */

#define PIDGIN_TYPE_CONVERSATION (pidgin_conversation_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginConversation, pidgin_conversation, PIDGIN,
                     CONVERSATION, GtkBox)

/**
 * pidgin_conversation_new:
 * @conversation: The [class@Purple.Conversation] to display.
 *
 * Creates the user interface for @conversation and returns it.
 *
 * Returns: (transfer full): The new widget.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_conversation_new(PurpleConversation *conversation);

/**
 * pidgin_conversation_from_purple_conversation:
 * @conversation: A [class@Purple.Conversation].
 *
 * Gets the [class@Pidgin.Conversation] that is displaying @conversation.
 *
 * Returns: (transfer none) (nullable): The [class@Pidgin.Conversation] displaying
 *          @conversation if one exists, otherwise %NULL.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_conversation_from_purple_conversation(PurpleConversation *conversation);

/**
 * pidgin_conversation_get_conversation:
 * @conversation: The instance.
 *
 * Gets the [class@Purple.Conversation] that @conversation is displaying.
 *
 * Returns: (transfer none): The [class@Purple.Conversation] that @conversation
 *          is displaying.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
PurpleConversation *pidgin_conversation_get_conversation(PidginConversation *conversation);

/**
 * pidgin_conversation_close:
 * @conversation: The instance.
 *
 * Close @conversation. This only closes the user interface portion of the
 * conversation and the user may remain in the conversation if the protocol
 * supports it.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_conversation_close(PidginConversation *conversation);

G_END_DECLS

#endif /* PIDGIN_CONVERSATION_H */
