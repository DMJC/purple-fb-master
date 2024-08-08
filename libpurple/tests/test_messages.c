/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include <purple.h>

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
test_purple_messages_items_changed_counter(G_GNUC_UNUSED GListModel *model,
                                           G_GNUC_UNUSED guint position,
                                           G_GNUC_UNUSED guint removed,
                                           G_GNUC_UNUSED guint added,
                                           gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_message_new_with_conversation(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessages *messages = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	messages = purple_messages_new(conversation);
	g_assert_true(PURPLE_IS_MESSAGES(messages));
	g_assert_true(G_IS_LIST_MODEL(messages));

	g_assert_finalize_object(messages);
	g_assert_finalize_object(conversation);
	g_clear_object(&account);
}

static void
test_purple_message_new_without_conversation(void) {
	if(g_test_subprocess()) {
		PurpleMessages *messages = NULL;

		messages = purple_messages_new(NULL);
		g_assert_null(messages);

		g_assert_not_reached();
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*CRITICAL*IS_CONVERSATION*failed*");
}

static void
test_purple_messages_properties(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation1 = NULL;
	PurpleConversation *conversation2 = NULL;
	PurpleMessages *messages = NULL;

	account = purple_account_new("test", "test");
	conversation1 = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	messages = g_object_new(
		PURPLE_TYPE_MESSAGES,
		"conversation", conversation1,
		NULL);

	g_object_get(
		G_OBJECT(messages),
		"conversation", &conversation2,
		NULL);

	g_assert_true(conversation2 == conversation1);
	g_clear_object(&conversation2);

	g_assert_finalize_object(messages);
	g_assert_finalize_object(conversation1);
	g_clear_object(&account);
}

static void
test_purple_messages_add_single(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessage *message1 = NULL;
	PurpleMessage *message2 = NULL;
	PurpleMessages *messages = NULL;
	guint counter = 0;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	messages = purple_messages_new(conversation);
	g_signal_connect(messages, "items-changed",
	                 G_CALLBACK(test_purple_messages_items_changed_counter),
	                 &counter);

	message1 = purple_message_new(purple_account_get_contact_info(account),
	                              "test message");
	purple_messages_add(messages, message1);
	g_assert_cmpuint(counter, ==, 1);

	message2 = g_list_model_get_item(G_LIST_MODEL(messages), 0);
	g_assert_true(PURPLE_IS_MESSAGE(message2));
	g_assert_true(message2 == message1);
	g_clear_object(&message2);

	g_assert_finalize_object(messages);
	g_assert_finalize_object(message1);
	g_assert_finalize_object(conversation);
	g_clear_object(&account);
}

static void
test_purple_messages_add_multiple(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessage *message = NULL;
	PurpleMessage *message1 = NULL;
	PurpleMessage *message2 = NULL;
	PurpleMessages *messages = NULL;
	GDateTime *dt1 = NULL;
	GDateTime *dt2 = NULL;
	GTimeZone *zone = NULL;
	guint counter = 0;

	/* This test adds two messages to the collection, the first one has an
	 * older timestamp than the first, which lets us test the automatic
	 * sorting.
	 */

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	messages = purple_messages_new(conversation);
	g_signal_connect(messages, "items-changed",
	                 G_CALLBACK(test_purple_messages_items_changed_counter),
	                 &counter);

	zone = g_time_zone_new_utc();

	message1 = purple_message_new(purple_account_get_contact_info(account),
	                              "second message");
	dt1 = g_date_time_new_from_iso8601("2024-08-07T03:07:33+0000", zone);
	purple_message_set_timestamp(message1, dt1);
	g_clear_pointer(&dt1, g_date_time_unref);
	purple_messages_add(messages, message1);
	g_assert_cmpuint(counter, ==, 1);

	message2 = purple_message_new(purple_account_get_contact_info(account),
	                              "first message");
	dt2 = g_date_time_new_from_iso8601("2024-08-07T03:06:33+0000", zone);
	purple_message_set_timestamp(message2, dt2);
	g_clear_pointer(&dt2, g_date_time_unref);
	purple_messages_add(messages, message2);
	g_assert_cmpuint(counter, ==, 2);

	/* Make sure that the first item in the list is message2. */
	message = g_list_model_get_item(G_LIST_MODEL(messages), 0);
	g_assert_true(PURPLE_IS_MESSAGE(message));
	g_assert_true(message == message2);
	g_clear_object(&message);

	/* Make sure that the second item in the list is message1. */
	message = g_list_model_get_item(G_LIST_MODEL(messages), 1);
	g_assert_true(PURPLE_IS_MESSAGE(message));
	g_assert_true(message == message1);
	g_clear_object(&message);

	g_clear_pointer(&zone, g_time_zone_unref);
	g_assert_finalize_object(messages);
	g_assert_finalize_object(message1);
	g_assert_finalize_object(message2);
	g_assert_finalize_object(conversation);
	g_clear_object(&account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/messages/new/with-conversation",
	                test_purple_message_new_with_conversation);
	g_test_add_func("/messages/new/without-conversation",
	                test_purple_message_new_without_conversation);

	g_test_add_func("/messages/properties", test_purple_messages_properties);

	g_test_add_func("/messages/add/single", test_purple_messages_add_single);
	g_test_add_func("/messages/add/multiple",
	                test_purple_messages_add_multiple);

	return g_test_run();
}
