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

#ifndef PURPLE_ACTION_H
#define PURPLE_ACTION_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_ACTION (purple_protocol_action_get_type())

PURPLE_AVAILABLE_TYPE_IN_3_0
typedef struct _PurpleProtocolAction PurpleProtocolAction;

PURPLE_AVAILABLE_TYPE_IN_3_0
typedef void (*PurpleProtocolActionCallback)(PurpleProtocolAction *action);

/**
 * PurpleActionMenu:
 *
 * A generic structure that contains information about an "action".  One
 * place this is used is by protocols to tell the core the list of available
 * right-click actions for a buddy list row.
 */
PURPLE_AVAILABLE_TYPE_IN_3_0
typedef struct _PurpleActionMenu PurpleActionMenu;

#include "connection.h"

/**
 * PurpleProtocolAction:
 * @label: A translated string to be shown in a user interface.
 * @callback: The function to call when the user wants to perform this action.
 * @connection: The connection that this action should be performed against.
 * @user_data: User data to pass to @callback.
 *
 * Represents an action that the protocol can perform. This shows up in the
 * Accounts menu, under a submenu with the name of the account.
 */
struct _PurpleProtocolAction {
	gchar *label;
	PurpleProtocolActionCallback callback;
	PurpleConnection *connection;
	gpointer user_data;
};

G_BEGIN_DECLS

/******************************************************************************
 * Menu Action API
 *****************************************************************************/

/**
 * purple_action_menu_new:
 * @label:    The text label to display for this action.
 * @callback: (scope notified): The function to be called when the action is used on
 *                 the selected item.
 * @data:     Additional data to be passed to the callback.
 * @children: (element-type PurpleActionMenu) (transfer full): Menu actions to
 *            be added as a submenu of this action.
 *
 * Creates a new PurpleActionMenu.
 *
 * Returns: (transfer full): The PurpleActionMenu.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleActionMenu *purple_action_menu_new(const gchar *label, GCallback callback, gpointer data, GList *children);

/**
 * purple_action_menu_free:
 * @act: The PurpleActionMenu to free.
 *
 * Frees a PurpleActionMenu
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_action_menu_free(PurpleActionMenu *act);

/**
 * purple_action_menu_get_label:
 * @act:	The PurpleActionMenu.
 *
 * Returns the label of the PurpleActionMenu.
 *
 * Returns: The label string.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *purple_action_menu_get_label(const PurpleActionMenu *act);

/**
 * purple_action_menu_get_callback:
 * @act:	The PurpleActionMenu.
 *
 * Returns the callback of the PurpleActionMenu.
 *
 * Returns: (transfer none): The callback function.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GCallback purple_action_menu_get_callback(const PurpleActionMenu *act);

/**
 * purple_action_menu_get_data:
 * @act:	The PurpleActionMenu.
 *
 * Returns the data stored in the PurpleActionMenu.
 *
 * Returns: The data.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gpointer purple_action_menu_get_data(const PurpleActionMenu *act);

/**
 * purple_action_menu_get_children:
 * @act:	The PurpleActionMenu.
 *
 * Returns the children of the PurpleActionMenu.
 *
 * Returns: (element-type PurpleActionMenu) (transfer none): The menu children.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GList* purple_action_menu_get_children(const PurpleActionMenu *act);

/**
 * purple_action_menu_set_label:
 * @act:   The menu action.
 * @label: The label for the menu action.
 *
 * Set the label to the PurpleActionMenu.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_action_menu_set_label(PurpleActionMenu *act, const gchar *label);

/**
 * purple_action_menu_set_callback:
 * @act:        The menu action.
 * @callback: (scope notified):  The callback.
 *
 * Set the callback that will be used by the PurpleActionMenu.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_action_menu_set_callback(PurpleActionMenu *act, GCallback callback);

/**
 * purple_action_menu_set_data:
 * @act:   The menu action.
 * @data:  The data used by this PurpleActionMenu
 *
 * Set the label to the PurpleActionMenu.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_action_menu_set_data(PurpleActionMenu *act, gpointer data);

/**
 * purple_action_menu_set_children:
 * @act:       The menu action.
 * @children: (element-type PurpleActionMenu) (transfer full): The menu children
 *
 * Set the children of the PurpleActionMenu.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_action_menu_set_children(PurpleActionMenu *act, GList *children);

/******************************************************************************
 * Protocol Action API
 *****************************************************************************/

/**
 * purple_protocol_action_get_type:
 *
 * Returns: The #GType for the #PurpleProtocolAction boxed structure.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GType purple_protocol_action_get_type(void);

/**
 * purple_protocol_action_new:
 * @label:    The description of the action to show to the user.
 * @callback: (scope call): The callback to call when the user selects this
 *            action.
 *
 * Allocates and returns a new PurpleProtocolAction. Use this to add actions in
 * a list in the get_actions function of the protocol.
 *
 * Returns: (transfer full): The new #PurpleProtocolAction.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleProtocolAction *purple_protocol_action_new(const gchar *label, PurpleProtocolActionCallback callback);

/**
 * purple_protocol_action_copy:
 * @action: The #PurpleProtocolAction to copy.
 *
 * Creates a newly allocated copy of @action.
 *
 * Returns: (transfer full): A copy of @action.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleProtocolAction *purple_protocol_action_copy(PurpleProtocolAction *action);

/**
 * purple_protocol_action_free:
 * @action: The PurpleProtocolAction to free.
 *
 * Frees a PurpleProtocolAction
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_action_free(PurpleProtocolAction *action);


G_END_DECLS

#endif /* PURPLE_ACTION_H */
