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

#ifndef PIDGIN_TYPING_LABEL_H
#define PIDGIN_TYPING_LABEL_H

#include <glib.h>

#include <gtk/gtk.h>

#include "pidginconversation.h"
#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginTypingLabel:
 *
 * A [class@Gtk.Widget] for displaying who is typing in a
 * [class@Purple.Conversation].
 *
 * Since: 3.0
 */

#define PIDGIN_TYPE_TYPING_LABEL (pidgin_typing_label_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginTypingLabel, pidgin_typing_label, PIDGIN,
                     TYPING_LABEL, GtkBox)

/**
 * pidgin_typing_label_new:
 * @conversation: A [class@Purple.Conversation] instance.
 *
 * Creates the new typing label for @conversation.
 *
 * Returns: (transfer full): The new widget.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_typing_label_new(PurpleConversation *conversation);

/**
 * pidgin_typing_label_get_conversation:
 * @label: The instance.
 *
 * Gets the [class@Purple.Conversation] that @label was created for.
 *
 * Returns: (transfer none): The conversation.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
PurpleConversation *pidgin_typing_label_get_conversation(PidginTypingLabel *label);

G_END_DECLS

#endif /* PIDGIN_TYPING_LABEL_H */
