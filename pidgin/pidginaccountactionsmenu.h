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

#ifndef PIDGIN_ACCOUNT_ACTIONS_MENU_H
#define PIDGIN_ACCOUNT_ACTIONS_MENU_H

#include <gtk/gtk.h>

#include <purple.h>

G_BEGIN_DECLS

/**
 * PidginAccountActionsMenu:
 *
 * A #GtkMenu that provides an interface for users to edit accounts and
 * activate the account's actions.
 */

#define PIDGIN_TYPE_ACCOUNT_ACTIONS_MENU (pidgin_account_actions_menu_get_type())
G_DECLARE_FINAL_TYPE(PidginAccountActionsMenu, pidgin_account_actions_menu,
                     PIDGIN, ACCOUNT_ACTIONS_MENU, GtkMenu)

/**
 * pidgin_account_actions_menu_new:
 * @account: The #PurpleAccount that this menu is for.
 *
 * Creates a new #PidginAccountActionsMenu for @account.
 *
 * Returns: (transfer full): The new #PidginAccountActionsMenu instance.
 */
GtkWidget *pidgin_account_actions_menu_new(PurpleAccount *account);

/**
 * pidgin_account_actions_menu_get_account:
 * @menu: The #PidginAccountActionsMenu instance.
 *
 * Gets the #PurpleAccount associated with @menu.
 *
 * Returns: (transfer none): The #PurpleAccount associated with @menu.
 */
PurpleAccount *pidgin_account_actions_menu_get_account(PidginAccountActionsMenu *menu);

G_END_DECLS

#endif /* PIDGIN_ACCOUNT_ACTIONS_MENU_H */
