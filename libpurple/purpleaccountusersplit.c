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

#include "purpleaccountusersplit.h"

/******************************************************************************
 * Structs
 *****************************************************************************/
struct _PurpleAccountUserSplit {
	char *text;
	char *default_value;
	char  field_sep;
	gboolean reverse;
	gboolean constant;
};

/******************************************************************************
 * Public API
 *****************************************************************************/
G_DEFINE_BOXED_TYPE(
	PurpleAccountUserSplit,
	purple_account_user_split,
	purple_account_user_split_copy,
	purple_account_user_split_destroy
);

PurpleAccountUserSplit *
purple_account_user_split_new(const char *text, const char *default_value, char sep)
{
	PurpleAccountUserSplit *split;

	g_return_val_if_fail(text != NULL, NULL);
	g_return_val_if_fail(sep != 0, NULL);

	split = g_new0(PurpleAccountUserSplit, 1);

	split->text = g_strdup(text);
	split->field_sep = sep;
	split->default_value = g_strdup(default_value);
	split->reverse = TRUE;

	return split;
}

PurpleAccountUserSplit *
purple_account_user_split_copy(PurpleAccountUserSplit *split) {
	PurpleAccountUserSplit *newsplit = NULL;

	newsplit = purple_account_user_split_new(split->text, split->default_value, split->field_sep);

	newsplit->reverse = split->reverse;
	newsplit->constant = split->constant;

	return newsplit;
}


void
purple_account_user_split_destroy(PurpleAccountUserSplit *split)
{
	g_return_if_fail(split != NULL);

	g_free(split->text);
	g_free(split->default_value);
	g_free(split);
}

const char *
purple_account_user_split_get_text(PurpleAccountUserSplit *split)
{
	g_return_val_if_fail(split != NULL, NULL);

	return split->text;
}

const char *
purple_account_user_split_get_default_value(PurpleAccountUserSplit *split)
{
	g_return_val_if_fail(split != NULL, NULL);

	return split->default_value;
}

char
purple_account_user_split_get_separator(PurpleAccountUserSplit *split)
{
	g_return_val_if_fail(split != NULL, 0);

	return split->field_sep;
}

gboolean
purple_account_user_split_get_reverse(PurpleAccountUserSplit *split)
{
	g_return_val_if_fail(split != NULL, FALSE);

	return split->reverse;
}

void
purple_account_user_split_set_reverse(PurpleAccountUserSplit *split, gboolean reverse)
{
	g_return_if_fail(split != NULL);

	split->reverse = reverse;
}

gboolean
purple_account_user_split_is_constant(PurpleAccountUserSplit *split)
{
	g_return_val_if_fail(split != NULL, FALSE);

	return split->constant;
}

void
purple_account_user_split_set_constant(PurpleAccountUserSplit *split,
	gboolean constant)
{
	g_return_if_fail(split != NULL);

	split->constant = constant;
}

