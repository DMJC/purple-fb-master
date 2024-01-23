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

#ifndef PIDGIN_INFO_PANE_H
#define PIDGIN_INFO_PANE_H

#include <glib.h>

#include <gtk/gtk.h>

#include <purple.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginInfoPane:
 *
 * #PidginInfoPane is a widget that displays information above
 * #PidginConversations.
 *
 * Since: 3.0
 */

#define PIDGIN_TYPE_INFO_PANE (pidgin_info_pane_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginInfoPane, pidgin_info_pane, PIDGIN, INFO_PANE,
                     GtkBox)

/**
 * pidgin_info_pane_new:
 *
 * Creates a new #PidginInfoPane instance.
 *
 * Returns: (transfer full): The new #PidginInfoPane instance.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_info_pane_new(void);

/**
 * pidgin_info_pane_get_title:
 * @pane: The #PidginInfoPane instance.
 *
 * Gets the title of @pane.
 *
 * Returns: (transfer none) (nullable): The title or %NULL.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
const char *pidgin_info_pane_get_title(PidginInfoPane *pane);

/**
 * pidgin_info_pane_set_title:
 * @pane: The instance.
 * @title: (nullable): The new title.
 *
 * Sets the title of @pane.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_info_pane_set_title(PidginInfoPane *pane, const char *title);

/**
 * pidgin_info_pane_get_subtitle:
 * @pane: The #PidginInfoPane instance.
 *
 * Gets the subtitle of @pane.
 *
 * Returns: (transfer none) (nullable): The subtitle or %NULL.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
const char *pidgin_info_pane_get_subtitle(PidginInfoPane *pane);

/**
 * pidgin_info_pane_set_subtitle:
 * @pane: The instance.
 * @subtitle: (nullable): The new subtitle.
 *
 * Sets the subtitle of @pane.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_info_pane_set_subtitle(PidginInfoPane *pane, const char *subtitle);

/**
 * pidgin_info_pane_get_child:
 * @pane: The instance.
 *
 * Gets the widget that is being displayed at the end of @pane.
 *
 * Returns: (transfer none) (nullable): The child widget or %NULL if one is not
 *          set.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_info_pane_get_child(PidginInfoPane *pane);

/**
 * pidgin_info_pane_set_child:
 * @pane: The instance.
 * @widget: (nullable): The new child.
 *
 * Sets the child widget of @pane to @widget.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_info_pane_set_child(PidginInfoPane *pane, GtkWidget *widget);

G_END_DECLS

#endif /* PIDGIN_INFO_PANE_H */
