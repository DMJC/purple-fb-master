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

#include <glib/gi18n-lib.h>

#include "purpleconversationuiops.h"

/******************************************************************************
 * Helpers
 *****************************************************************************/
static PurpleConversationUiOps *
purple_conversation_ui_ops_copy(PurpleConversationUiOps *ops)
{
	PurpleConversationUiOps *ops_new;

	g_return_val_if_fail(ops != NULL, NULL);

	ops_new = g_new(PurpleConversationUiOps, 1);
	*ops_new = *ops;

	return ops_new;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GType
purple_conversation_ui_ops_get_type(void)
{
	static GType type = 0;

	if (type == 0) {
		type = g_boxed_type_register_static("PurpleConversationUiOps",
				(GBoxedCopyFunc)purple_conversation_ui_ops_copy,
				(GBoxedFreeFunc)g_free);
	}

	return type;
}
