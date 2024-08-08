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

#ifndef PURPLE_ACCOUNT_USER_SPLIT_H
#define PURPLE_ACCOUNT_USER_SPLIT_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

#define PURPLE_TYPE_ACCOUNT_USER_SPLIT (purple_account_user_split_get_type())

/**
 * PurpleAccountUserSplit:
 *
 * A username split.
 *
 * This is used by some protocols to separate the fields of the username
 * into more human-readable components.
 *
 * Since: 2.0
 */
typedef struct _PurpleAccountUserSplit	PurpleAccountUserSplit;

G_BEGIN_DECLS

PURPLE_AVAILABLE_IN_3_0
GType purple_account_user_split_get_type(void);

/**
 * purple_account_user_split_new:
 * @text:          The text of the option.
 * @default_value: The default value.
 * @sep:           The field separator.
 *
 * Creates a new account username split.
 *
 * Returns: The new user split.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleAccountUserSplit *purple_account_user_split_new(const char *text, const char *default_value, char sep);

/**
 * purple_account_user_split_copy:
 * @split: The split to copy.
 *
 * Creates a copy of @split.
 *
 * Returns: (transfer full): A copy of @split.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAccountUserSplit *purple_account_user_split_copy(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_destroy:
 * @split: The split to destroy.
 *
 * Destroys an account username split.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_user_split_destroy(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_get_text:
 * @split: The account username split.
 *
 * Returns the text for an account username split.
 *
 * Returns: The account username split's text.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_user_split_get_text(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_get_default_value:
 * @split: The account username split.
 *
 * Returns the default string value for an account split.
 *
 * Returns: The default string.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_user_split_get_default_value(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_get_separator:
 * @split: The account username split.
 *
 * Returns the field separator for an account split.
 *
 * Returns: The field separator.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
char purple_account_user_split_get_separator(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_get_reverse:
 * @split: The account username split.
 *
 * Returns the 'reverse' value for an account split.
 *
 * Returns: The 'reverse' value.
 *
 * Since: 2.1
 */
PURPLE_AVAILABLE_IN_2_1
gboolean purple_account_user_split_get_reverse(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_set_reverse:
 * @split:   The account username split.
 * @reverse: The 'reverse' value
 *
 * Sets the 'reverse' value for an account split.
 *
 * Since: 2.1
 */
PURPLE_AVAILABLE_IN_2_1
void purple_account_user_split_set_reverse(PurpleAccountUserSplit *split, gboolean reverse);

/**
 * purple_account_user_split_is_constant:
 * @split: The account username split.
 *
 * Returns the constant parameter for an account split.
 *
 * When split is constant, it does not need to be displayed
 * in configuration dialog.
 *
 * Returns: %TRUE, if the split is constant.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_account_user_split_is_constant(PurpleAccountUserSplit *split);

/**
 * purple_account_user_split_set_constant:
 * @split:    The account username split.
 * @constant: %TRUE, if the split is a constant part.
 *
 * Sets the constant parameter of account split.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_account_user_split_set_constant(PurpleAccountUserSplit *split, gboolean constant);

G_END_DECLS

#endif /* PURPLE_ACCOUNT_USER_SPLIT_H */
