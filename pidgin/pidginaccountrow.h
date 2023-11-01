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

#ifndef PIDGIN_ACCOUNT_ROW_H
#define PIDGIN_ACCOUNT_ROW_H

#include <glib.h>

#include <purple.h>

#include <gtk/gtk.h>

#include <adwaita.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginAccountRow:
 *
 * A [class@Adw.ComboRow] subclass that allows the user to select an
 * [class@Purple.Account].
 *
 * Since: 3.0.0
 */

#define PIDGIN_TYPE_ACCOUNT_ROW (pidgin_account_row_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginAccountRow, pidgin_account_row, PIDGIN, ACCOUNT_ROW,
                     AdwComboRow)

/**
 * pidgin_account_row_new:
 *
 * Creates a new instance.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_account_row_new(void);

/**
 * pidgin_account_row_get_account:
 * @row: The instance.
 *
 * Gets the [class@Purple.Account] that is currently selected.
 *
 * Returns: (transfer none): The account if one is selected otherwise %NULL if
 *          the model is empty.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
PurpleAccount *pidgin_account_row_get_account(PidginAccountRow *row);

/**
 * pidgin_account_row_set_account:
 * @row: The instance.
 * @account: (transfer none) (nullable): The [class@Purple.Account] to select.
 *
 * Sets the selected account in @row to @account. If @account is %NULL, then
 * the first item if one exists will be selected.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_account_row_set_account(PidginAccountRow *row, PurpleAccount *account);

/**
 * pidgin_account_row_get_filter:
 * @row: The instance.
 *
 * Gets the filter that is being used to choose which accounts to display in
 * @row.
 *
 * Returns: (transfer none) (nullable): The filter being used or %NULL.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkFilter *pidgin_account_row_get_filter(PidginAccountRow *row);

/**
 * pidgin_account_row_set_filter:
 * @row: The instance.
 * @filter: (nullable): The new filter.
 *
 * Sets the filter for @row to @filter. If @filter is %NULL, any existing
 * filter will be removed.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_account_row_set_filter(PidginAccountRow *row, GtkFilter *filter);

G_END_DECLS

#endif /* PIDGIN_ACCOUNT_ROW_H */
