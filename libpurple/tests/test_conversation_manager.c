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

#include "test_ui.h"

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
test_purple_conversation_manager_counter_cb(PurpleConversationManager *manager,
                                            PurpleConversation *conversation,
                                            gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION_MANAGER(manager));
	g_assert_true(PURPLE_IS_CONVERSATION(conversation));

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_conversation_manager_register_unregister(void) {
	PurpleAccount *account = NULL;
	PurpleConversationManager *manager = NULL;
	PurpleConversation *conversation = NULL;
	GList *conversations = NULL;
	guint registered_counter = 0, unregistered_counter = 0;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CONVERSATION_MANAGER, NULL);

	g_assert_true(PURPLE_IS_CONVERSATION_MANAGER(manager));

	/* Wire up our signals. */
	g_signal_connect(manager, "registered",
	                 G_CALLBACK(test_purple_conversation_manager_counter_cb),
	                 &registered_counter);
	g_signal_connect(manager, "unregistered",
	                 G_CALLBACK(test_purple_conversation_manager_counter_cb),
	                 &unregistered_counter);

	/* Create our account. */
	account = purple_account_new("test", "test");

	/* Create the conversation. */
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	/* Add the conversation to the manager. */
	result = purple_conversation_manager_register(manager, conversation);
	g_assert_true(result);

	/* Make sure the conversation is available. */
	conversations = purple_conversation_manager_get_all(manager);
	g_assert_nonnull(conversations);
	g_assert_cmpuint(g_list_length(conversations), ==, 1);
	g_list_free(conversations);

	/* Make sure the registered signal was called. */
	g_assert_cmpuint(registered_counter, ==, 1);

	/* Remove the contact. */
	result = purple_conversation_manager_unregister(manager, conversation);
	g_assert_true(result);
	g_assert_cmpuint(unregistered_counter, ==, 1);

	/* Clean up.*/
	g_clear_object(&conversation);
	g_clear_object(&account);
	g_clear_object(&manager);
}

/******************************************************************************
 * Signal Tests
 *****************************************************************************/
static void
test_purple_conversation_manager_signal_conversation_changed_cb(PurpleConversationManager *manager,
                                                                PurpleConversation *conversation,
                                                                GParamSpec *pspec,
                                                                gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION_MANAGER(manager));
	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_true(G_IS_PARAM_SPEC(pspec));

	*counter = *counter + 1;
}

static void
test_purple_conversation_manager_signal_conversation_changed(void) {
	PurpleAccount *account = NULL;
	PurpleConversationManager *manager = NULL;
	PurpleConversation *conversation = NULL;
	guint counter = 0;
	guint detailed_counter = 0;
	gboolean result = FALSE;

	manager = g_object_new(PURPLE_TYPE_CONVERSATION_MANAGER, NULL);
	g_signal_connect(manager, "conversation-changed",
	                 G_CALLBACK(test_purple_conversation_manager_signal_conversation_changed_cb),
	                 &counter);
	g_signal_connect(manager, "conversation-changed::title",
	                 G_CALLBACK(test_purple_conversation_manager_signal_conversation_changed_cb),
	                 &detailed_counter);

	account = purple_account_new("test", "test");

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);
	purple_conversation_manager_register(manager, conversation);

	/* Set the features and title of the conversation, and verify our counters. */
	purple_conversation_set_features(conversation, PURPLE_CONNECTION_FLAG_NO_IMAGES);
	purple_conversation_set_title(conversation, "A place for kool kats");

	g_assert_cmpuint(counter, ==, 2);
	g_assert_cmpuint(detailed_counter, ==, 1);

	/* Unregister the conversation, to make sure signals are no longer
	 * propagated.
	 */
	result = purple_conversation_manager_unregister(manager, conversation);
	g_assert_true(result);

	counter = 0;
	detailed_counter = 0;
	purple_conversation_set_title(conversation,
	                              "no longer a place for kool kats");
	g_assert_cmpuint(counter, ==, 0);
	g_assert_cmpuint(detailed_counter, ==, 0);

	/* Clean up. */
	g_clear_object(&conversation);
	g_clear_object(&account);
	g_clear_object(&manager);
}

static void
test_purple_conversation_manager_signal_present_conversation(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;
	guint counter = 0;
	gboolean ret = FALSE;

	account = purple_account_new("test", "test");

	manager = g_object_new(PURPLE_TYPE_CONVERSATION_MANAGER, NULL);
	g_signal_connect(manager, "present-conversation",
	                 G_CALLBACK(test_purple_conversation_manager_counter_cb),
	                 &counter);

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);

	ret = purple_conversation_manager_register(manager, conversation);
	g_assert_true(ret);

	g_assert_cmpuint(counter, ==, 0);
	purple_conversation_present(conversation);
	g_assert_cmpuint(counter, ==, 1);

	ret = purple_conversation_manager_unregister(manager, conversation);
	g_assert_true(ret);

	g_assert_finalize_object(manager);
	g_assert_finalize_object(conversation);
	g_clear_object(&account);
}

/******************************************************************************
 * find-dm tests
 *****************************************************************************/
static void
test_purple_conversation_manager_find_dm_empty(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_CONVERSATION_MANAGER, NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	conversation = purple_conversation_manager_find_dm(manager, contact);
	g_assert_null(conversation);

	g_assert_finalize_object(contact);
	g_assert_finalize_object(manager);

	g_clear_object(&account);
}

static void
test_purple_conversation_manager_find_dm_exists(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation1 = NULL;
	PurpleConversation *conversation2 = NULL;
	PurpleConversationManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_CONVERSATION_MANAGER, NULL);

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account,
	                             "a9780f2a-eeb5-4d6b-89cb-52e5dad3973f");

	conversation1 = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);
	purple_conversation_manager_register(manager, conversation1);
	purple_conversation_add_member(conversation1, PURPLE_CONTACT_INFO(contact),
	                               FALSE, NULL);

	conversation2 = purple_conversation_manager_find_dm(manager, contact);
	g_assert_nonnull(conversation2);
	g_assert_true(conversation1 == conversation2);

	purple_conversation_manager_unregister(manager, conversation1);

	g_assert_finalize_object(conversation1);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(manager);

	g_clear_object(&account);
}

/* This test makes sure we hit all of the conditional code. */
static void
test_purple_conversation_manager_find_dm_does_not_exist(void) {
	PurpleAccount *account1 = NULL;
	PurpleAccount *account2 = NULL;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation1 = NULL;
	PurpleConversation *conversation2 = NULL;
	PurpleConversation *conversation3 = NULL;
	PurpleConversation *conversation4 = NULL;
	PurpleConversationManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_CONVERSATION_MANAGER, NULL);

	account1 = purple_account_new("test1", "test1");
	account2 = purple_account_new("test2", "test2");

	contact = purple_contact_new(account1, NULL);

	/* Create a conversation with the contact's account that's not a dm. */
	conversation1 = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account1,
		"type", PURPLE_CONVERSATION_TYPE_CHANNEL,
		NULL);
	purple_conversation_manager_register(manager, conversation1);

	/* Create a conversation with the contact's account that is a dm but not
	 * with the contact.
	 */
	conversation2 = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account1,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);
	purple_conversation_manager_register(manager, conversation2);

	/* Create a conversation with a different account. */
	conversation3 = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account2,
		"type", PURPLE_CONVERSATION_TYPE_CHANNEL,
		NULL);
	purple_conversation_manager_register(manager, conversation2);

	conversation4 = purple_conversation_manager_find_dm(manager, contact);
	g_assert_null(conversation4);

	purple_conversation_manager_unregister(manager, conversation1);
	purple_conversation_manager_unregister(manager, conversation2);
	purple_conversation_manager_unregister(manager, conversation3);

	g_assert_finalize_object(conversation1);
	g_assert_finalize_object(conversation2);
	g_assert_finalize_object(conversation3);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(manager);

	g_clear_object(&account1);
	g_clear_object(&account2);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	gint ret = 0;

	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	g_test_add_func("/conversation-manager/register-unregister",
	                test_purple_conversation_manager_register_unregister);

	g_test_add_func("/conversation-manager/signals/conversation-changed",
	                test_purple_conversation_manager_signal_conversation_changed);
	g_test_add_func("/conversation-manager/signals/present-conversation",
	                test_purple_conversation_manager_signal_present_conversation);

	g_test_add_func("/conversation-manager/find-dm/empty",
	                test_purple_conversation_manager_find_dm_empty);
	g_test_add_func("/conversation-manager/find-dm/exists",
	                test_purple_conversation_manager_find_dm_exists);
	g_test_add_func("/conversation-manager/find-dm/does-not-exist",
	                test_purple_conversation_manager_find_dm_does_not_exist);

	ret = g_test_run();

	test_ui_purple_uninit();

	return ret;
}
