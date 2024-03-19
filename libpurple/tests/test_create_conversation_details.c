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

#include <glib.h>

#include <purple.h>

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_create_conversation_details_new(void) {
	PurpleCreateConversationDetails *details = NULL;

	details = purple_create_conversation_details_new(9);
	g_assert_true(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	g_assert_finalize_object(details);
}

static void
test_purple_create_conversation_details_properties(void) {
	PurpleCreateConversationDetails *details = NULL;
	GListStore *store = NULL;
	GListModel *model = NULL;
	guint max_participants = 0;

	store = g_list_store_new(PURPLE_TYPE_CONTACT);

	details = g_object_new(
		PURPLE_TYPE_CREATE_CONVERSATION_DETAILS,
		"max-participants", 9,
		"participants", store,
		NULL);

	g_assert_true(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	g_object_get(
		G_OBJECT(details),
		"max-participants", &max_participants,
		"participants", &model,
		NULL);

	g_assert_cmpuint(max_participants, ==, 9);

	g_assert_true(G_IS_LIST_MODEL(model));
	g_assert_true(model == G_LIST_MODEL(store));
	g_clear_object(&model);

	g_assert_finalize_object(details);
	g_assert_finalize_object(store);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/create-conversation-details/new",
	                test_purple_create_conversation_details_new);
	g_test_add_func("/create-conversation-details/properties",
	                test_purple_create_conversation_details_properties);

	return g_test_run();
}
