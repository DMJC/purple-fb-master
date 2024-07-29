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
test_purple_conversation_notify_counter_cb(GObject *obj,
                                           G_GNUC_UNUSED GParamSpec *pspec,
                                           gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION(obj));

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_conversation_properties(void) {
	PurpleAccount *account = NULL;
	PurpleAccount *account1 = NULL;
	PurpleAvatar *avatar = NULL;
	PurpleAvatar *avatar1 = NULL;
	PurpleConnectionFlags features = 0;
	PurpleContactInfo *creator = NULL;
	PurpleContactInfo *creator1 = NULL;
	PurpleContactInfo *topic_author = NULL;
	PurpleContactInfo *topic_author1 = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationType type = PURPLE_CONVERSATION_TYPE_UNSET;
	PurpleTags *tags = NULL;
	GDateTime *created_on = NULL;
	GDateTime *created_on1 = NULL;
	GDateTime *topic_updated = NULL;
	GDateTime *topic_updated1 = NULL;
	GListModel *members = NULL;
	char *alias = NULL;
	char *description = NULL;
	char *global_id = NULL;
	char *id = NULL;
	char *name = NULL;
	char *title = NULL;
	char *title_for_display = NULL;
	char *topic = NULL;
	char *user_nickname = NULL;
	gboolean age_restricted = FALSE;
	gboolean favorite = FALSE;

	account = g_object_new(
		PURPLE_TYPE_ACCOUNT,
		"id", "test",
		"protocol-id", "test",
		NULL);

	avatar = g_object_new(PURPLE_TYPE_AVATAR, NULL);
	creator = purple_contact_info_new(NULL);
	created_on = g_date_time_new_now_utc();
	topic_author = purple_contact_info_new(NULL);
	topic_updated = g_date_time_new_now_utc();

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"age-restricted", TRUE,
		"alias", "talky talk",
		"avatar", avatar,
		"created-on", created_on,
		"creator", creator,
		"description", "to describe or not to describe...",
		"favorite", TRUE,
		"features", PURPLE_CONNECTION_FLAG_HTML,
		"id", "id1",
		"name", "name1",
		"title", "test conversation",
		"topic", "the topic...",
		"topic-author", topic_author,
		"topic-updated", topic_updated,
		"type", PURPLE_CONVERSATION_TYPE_THREAD,
		"user-nickname", "knick-knack",
		NULL);

	/* Now use g_object_get to read all of the properties. */
	g_object_get(conversation,
		"account", &account1,
		"age-restricted", &age_restricted,
		"alias", &alias,
		"avatar", &avatar1,
		"created-on", &created_on1,
		"creator", &creator1,
		"description", &description,
		"favorite", &favorite,
		"features", &features,
		"global-id", &global_id,
		"id", &id,
		"members", &members,
		"name", &name,
		"tags", &tags,
		"title", &title,
		"title-for-display", &title_for_display,
		"topic", &topic,
		"topic-author", &topic_author1,
		"topic-updated", &topic_updated1,
		"type", &type,
		"user-nickname", &user_nickname,
		NULL);

	/* Compare all the things. */
	g_assert_true(account1 == account);
	g_clear_object(&account1);

	g_assert_true(age_restricted);

	g_assert_cmpstr(alias, ==, "talky talk");
	g_clear_pointer(&alias, g_free);

	g_assert_true(avatar1 == avatar);
	g_clear_object(&avatar1);

	g_assert_nonnull(created_on1);
	g_assert_true(g_date_time_equal(created_on1, created_on));
	g_clear_pointer(&created_on1, g_date_time_unref);

	g_assert_true(creator1 == creator);
	g_clear_object(&creator1);

	g_assert_cmpstr(description, ==, "to describe or not to describe...");
	g_clear_pointer(&description, g_free);

	g_assert_true(favorite);

	g_assert_cmpint(features, ==, PURPLE_CONNECTION_FLAG_HTML);

	g_assert_cmpstr(global_id, ==, "test-id1");
	g_clear_pointer(&global_id, g_free);

	g_assert_cmpstr(id, ==, "id1");
	g_clear_pointer(&id, g_free);

	g_assert_true(G_IS_LIST_MODEL(members));
	g_clear_object(&members);

	g_assert_cmpstr(name, ==, "name1");
	g_clear_pointer(&name, g_free);

	g_assert_true(PURPLE_IS_TAGS(tags));
	g_clear_object(&tags);

	g_assert_cmpstr(title, ==, "test conversation");
	g_clear_pointer(&title, g_free);

	g_assert_cmpstr(title_for_display, ==, "talky talk");
	g_clear_pointer(&title_for_display, g_free);

	g_assert_cmpstr(topic, ==, "the topic...");
	g_clear_pointer(&topic, g_free);

	g_assert_true(topic_author == topic_author1);
	g_clear_object(&topic_author1);

	g_assert_nonnull(topic_updated1);
	g_assert_true(g_date_time_equal(topic_updated1, topic_updated));
	g_clear_pointer(&topic_updated1, g_date_time_unref);

	g_assert_cmpuint(type, ==, PURPLE_CONVERSATION_TYPE_THREAD);

	g_assert_cmpstr(user_nickname, ==, "knick-knack");
	g_clear_pointer(&user_nickname, g_free);

	g_clear_object(&avatar);
	g_clear_object(&topic_author);
	g_clear_pointer(&topic_updated, g_date_time_unref);
	g_clear_pointer(&created_on, g_date_time_unref);
	g_clear_object(&creator);
	g_clear_object(&account);
	g_clear_object(&conversation);
}

static void
test_purple_conversation_set_topic_full(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleContactInfo *author = NULL;
	GDateTime *updated = NULL;
	GDateTime *updated1 = NULL;
	const char *topic = "this is the topic";

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));

	author = purple_contact_info_new(NULL);
	updated = g_date_time_new_now_utc();

	purple_conversation_set_topic_full(conversation, topic, author, updated);

	g_assert_cmpstr(purple_conversation_get_topic(conversation), ==, topic);
	g_assert_true(purple_conversation_get_topic_author(conversation) ==
	              author);

	updated1 = purple_conversation_get_topic_updated(conversation);
	g_assert_nonnull(updated1);
	g_assert_true(g_date_time_equal(updated1, updated));

	g_clear_pointer(&updated, g_date_time_unref);
	g_clear_object(&author);
	g_clear_object(&account);
	g_clear_object(&conversation);
}

static void
test_purple_conversation_title_for_display(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	const char *title_for_display = NULL;
	guint counter = 0;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"id", "id1",
		NULL);
	g_signal_connect(conversation, "notify::title-for-display",
	                 G_CALLBACK(test_purple_conversation_notify_counter_cb),
	                 &counter);

	/* Make sure the defaults are all as expected. */
	g_assert_cmpuint(counter, ==, 0);
	title_for_display = purple_conversation_get_title_for_display(conversation);
	g_assert_cmpstr(title_for_display, ==, "id1");

	/* Set the alias, verify that the notify was sent for name-for-display and
	 * that it returns the right value.
	 */
	purple_conversation_set_alias(conversation, "alias1");
	g_assert_cmpuint(counter, ==, 1);
	title_for_display = purple_conversation_get_title_for_display(conversation);
	g_assert_cmpstr(title_for_display, ==, "alias1");

	/* Set the title, which won't change the name for display because alias is
	 * set, but it should still emit the signal because we don't track the
	 * name-for-display in every setter.
	 */
	purple_conversation_set_title(conversation, "title1");
	g_assert_cmpuint(counter, ==, 2);
	title_for_display = purple_conversation_get_title_for_display(conversation);
	g_assert_cmpstr(title_for_display, ==, "alias1");

	/* Now set the alias to NULL which should emit the signal and
	 * title_for_display should now return title1.
	 */
	purple_conversation_set_alias(conversation, NULL);
	g_assert_cmpuint(counter, ==, 3);
	title_for_display = purple_conversation_get_title_for_display(conversation);
	g_assert_cmpstr(title_for_display, ==, "title1");

	/* Finally remove the title as well, which should emit the signal and
	 * get_title_for_display should return the id.
	 */
	purple_conversation_set_title(conversation, NULL);
	g_assert_cmpuint(counter, ==, 4);
	title_for_display = purple_conversation_get_title_for_display(conversation);
	g_assert_cmpstr(title_for_display, ==, "id1");

	/* We're done!! */
	g_assert_finalize_object(conversation);

	/* We can't use g_assert_finalize_object because accounts automatically
	 * get added to the account manager.
	 */
	g_clear_object(&account);
}

/******************************************************************************
 * "is" tests
 *****************************************************************************/
static void
test_purple_conversation_is_dm(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_true(purple_conversation_is_dm(conversation));
	g_assert_false(purple_conversation_is_group_dm(conversation));
	g_assert_false(purple_conversation_is_channel(conversation));
	g_assert_false(purple_conversation_is_thread(conversation));

	g_assert_finalize_object(conversation);

	g_clear_object(&account);
}

static void
test_purple_conversation_is_group_dm(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_GROUP_DM,
		NULL);

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_false(purple_conversation_is_dm(conversation));
	g_assert_true(purple_conversation_is_group_dm(conversation));
	g_assert_false(purple_conversation_is_channel(conversation));
	g_assert_false(purple_conversation_is_thread(conversation));

	g_assert_finalize_object(conversation);

	g_clear_object(&account);
}

static void
test_purple_conversation_is_channel(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_CHANNEL,
		NULL);

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_false(purple_conversation_is_dm(conversation));
	g_assert_false(purple_conversation_is_group_dm(conversation));
	g_assert_true(purple_conversation_is_channel(conversation));
	g_assert_false(purple_conversation_is_thread(conversation));

	g_assert_finalize_object(conversation);

	g_clear_object(&account);
}

static void
test_purple_conversation_is_thread(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_THREAD,
		NULL);

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_false(purple_conversation_is_dm(conversation));
	g_assert_false(purple_conversation_is_group_dm(conversation));
	g_assert_false(purple_conversation_is_channel(conversation));
	g_assert_true(purple_conversation_is_thread(conversation));

	g_assert_finalize_object(conversation);

	g_clear_object(&account);
}

/******************************************************************************
 * Membership tests and helpers
 *****************************************************************************/
static void
test_purple_conversation_membership_signal_cb(PurpleConversation *conversation,
                                              PurpleConversationMember *member,
                                              gboolean announce,
                                              const char *message,
                                              gpointer data)
{
	/* We use int's for called to make sure it was only called once. */
	gint *called = data;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member));
	g_assert_true(announce);
	g_assert_cmpstr(message, ==, "announcement message");

	*called = *called + 1;
}

static void
test_purple_conversation_members_add_remove(void) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *info = NULL;
	PurpleContactInfo *member_info = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *conversation_manager = NULL;
	PurpleConversationMember *member = NULL;
	PurpleConversationMember *member1 = NULL;
	PurpleConversationMember *test_member = NULL;
	gboolean removed = FALSE;
	gint added_called = 0;
	gint removed_called = 0;

	/* Create our instances. The id is just a uuid 4 to help us avoid a
	 * g_warning.
	 */
	info = purple_contact_info_new("745c50ba-1189-48d9-827c-051783026c96");
	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"name", "test-conversation",
		NULL);

	/* Make sure the account got added as a member. */
	test_member = purple_conversation_find_member(conversation,
	                                              purple_account_get_contact_info(account));
	g_assert_nonnull(test_member);

	/* Connect our signals. */
	g_signal_connect(conversation, "member-added",
	                 G_CALLBACK(test_purple_conversation_membership_signal_cb),
	                 &added_called);
	g_signal_connect(conversation, "member-removed",
	                 G_CALLBACK(test_purple_conversation_membership_signal_cb),
	                 &removed_called);

	/* Add the member. */
	member = purple_conversation_add_member(conversation, info, TRUE,
	                                        "announcement message");
	g_assert_cmpint(added_called, ==, 1);
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member));

	test_member = purple_conversation_find_member(conversation, info);
	g_assert_true(member == test_member);

	/* Add our own reference to the returned member as we use it later to
	 * verify that double remove doesn't do anything.
	 */
	g_object_ref(member);

	/* Try to add the member again, which would just return the existing
	 * member and not emit the signal.
	 */
	member1 = purple_conversation_add_member(conversation, info, TRUE,
	                                         "announcement message");
	g_assert_cmpint(added_called, ==, 1);
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member1));
	g_assert_true(member1 == member);

	/* Now remove the member and verify the signal was called. */
	member_info = purple_conversation_member_get_contact_info(member);
	removed = purple_conversation_remove_member(conversation, member_info,
	                                            TRUE, "announcement message");
	g_assert_true(removed);
	g_assert_cmpint(removed_called, ==, 1);

	test_member = purple_conversation_find_member(conversation, info);
	g_assert_null(test_member);

	/* Try to remove the member again and verify that nothing was removed and
	 * that the signal wasn't emitted.
	 */
	member_info = purple_conversation_member_get_contact_info(member);
	removed = purple_conversation_remove_member(conversation, member_info,
	                                            TRUE, "announcement message");
	g_assert_false(removed);
	g_assert_cmpint(removed_called, ==, 1);

	/* TODO: Conversations are automatically registered on construction for
	 * legacy reasons, so we need to explicitly unregister to clean them up,
	 * but this can go away once that stops happening. */
	conversation_manager = purple_conversation_manager_get_default();
	purple_conversation_manager_unregister(conversation_manager, conversation);

	/* Clean up everything. */
	g_clear_object(&info);
	g_clear_object(&member);
	g_clear_object(&account);
	g_clear_object(&conversation);
}

/******************************************************************************
 * Message Tests
 *****************************************************************************/
static void
test_purple_conversation_message_write_one(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessage *message = NULL;
	GListModel *messages = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	messages = purple_conversation_get_messages(conversation);
	g_assert_nonnull(messages);
	g_assert_cmpuint(g_list_model_get_n_items(messages), ==, 0);

	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"contents", "hello world!",
		NULL);

	purple_conversation_write_message(conversation, message);
	g_assert_cmpuint(g_list_model_get_n_items(messages), ==, 1);

	g_clear_object(&message);
	g_clear_object(&account);
	g_clear_object(&conversation);
}

/******************************************************************************
 * Signal tests
 *****************************************************************************/
static void
test_purple_conversation_present_cb(PurpleConversation *conversation,
                                    gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));

	*counter = *counter + 1;
}

static void
test_purple_conversation_signals_present(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	guint counter = 0;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		"name", "bleh",
		NULL);

	g_signal_connect(conversation, "present",
	                 G_CALLBACK(test_purple_conversation_present_cb),
	                 &counter);

	g_assert_cmpuint(counter, ==, 0);
	purple_conversation_present(conversation);
	g_assert_cmpuint(counter, ==, 1);

	g_assert_finalize_object(conversation);
	g_clear_object(&account);
}

/******************************************************************************
 * generate_title tests
 *****************************************************************************/
static void
test_purple_conversation_generate_title_empty(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	const char *title = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);

	title = purple_conversation_get_title(conversation);
	g_assert_null(title);

	purple_conversation_set_title(conversation, "test");
	title = purple_conversation_get_title(conversation);
	g_assert_cmpstr(title, ==, "test");

	/* There are no members in this conversation, so calling generate_title
	 * doesn't change the title.
	 */
	purple_conversation_generate_title(conversation);
	title = purple_conversation_get_title(conversation);
	g_assert_cmpstr(title, ==, "test");

	g_assert_finalize_object(conversation);
	g_clear_object(&account);
}

static void
test_purple_conversation_generate_title_dm(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;
	const char *title = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);

	title = purple_conversation_get_title(conversation);
	g_assert_null(title);

	contact = purple_contact_new(account, NULL);
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact), "Alice");
	purple_conversation_add_member(conversation, PURPLE_CONTACT_INFO(contact),
	                               FALSE, NULL);

	title = purple_conversation_get_title(conversation);
	g_assert_cmpstr(title, ==, "Alice");

	/* Make sure the title updates when the display name changes. */
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact), "alice!");
	title = purple_conversation_get_title(conversation);
	g_assert_cmpstr(title, ==, "alice!");

	g_assert_finalize_object(conversation);
	g_assert_finalize_object(contact);
	g_clear_object(&account);
}

static void
test_purple_conversation_generate_title_group_dm(void) {
	PurpleAccount *account = NULL;
	PurpleContact *contact1 = NULL;
	PurpleContact *contact2 = NULL;
	PurpleContact *contact3 = NULL;
	PurpleConversation *conversation = NULL;
	const char *title = NULL;

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_GROUP_DM,
		NULL);

	title = purple_conversation_get_title(conversation);
	g_assert_null(title);

	contact1 = purple_contact_new(account, NULL);
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact1), "Alice");
	purple_conversation_add_member(conversation, PURPLE_CONTACT_INFO(contact1),
	                               FALSE, NULL);

	contact2 = purple_contact_new(account, NULL);
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact2), "Bob");
	purple_conversation_add_member(conversation, PURPLE_CONTACT_INFO(contact2),
	                               FALSE, NULL);

	contact3 = purple_contact_new(account, NULL);
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact3), "Eve");
	purple_conversation_add_member(conversation, PURPLE_CONTACT_INFO(contact3),
	                               FALSE, NULL);

	title = purple_conversation_get_title(conversation);
	g_assert_cmpstr(title, ==, "Alice, Bob, Eve");

	/* Change some names around and verify the title was generated properly. */
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact2), "Robert");
	purple_contact_info_set_username(PURPLE_CONTACT_INFO(contact3), "Evelyn");

	title = purple_conversation_get_title(conversation);
	g_assert_cmpstr(title, ==, "Alice, Robert, Evelyn");

	g_assert_finalize_object(conversation);
	g_assert_finalize_object(contact1);
	g_assert_finalize_object(contact2);
	g_assert_finalize_object(contact3);
	g_clear_object(&account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	gint ret = 0;

	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	g_test_add_func("/conversation/properties",
	                test_purple_conversation_properties);
	g_test_add_func("/conversation/set-topic-full",
	                test_purple_conversation_set_topic_full);
	g_test_add_func("/conversation/title-for-display",
	                test_purple_conversation_title_for_display);

	g_test_add_func("/conversation/is-dm", test_purple_conversation_is_dm);
	g_test_add_func("/conversation/is-group-dm",
	                test_purple_conversation_is_group_dm);
	g_test_add_func("/conversation/is-channel",
	                test_purple_conversation_is_channel);
	g_test_add_func("/conversation/is-thread",
	                test_purple_conversation_is_thread);

	g_test_add_func("/conversation/members/add-remove",
	                test_purple_conversation_members_add_remove);

	g_test_add_func("/conversation/message/write-one",
	                test_purple_conversation_message_write_one);

	g_test_add_func("/conversation/signals/present",
	                test_purple_conversation_signals_present);

	g_test_add_func("/conversation/generate-title/empty",
	                test_purple_conversation_generate_title_empty);
	g_test_add_func("/conversation/generate-title/dm",
	                test_purple_conversation_generate_title_dm);
	g_test_add_func("/conversation/generate-title/group-dm",
	                test_purple_conversation_generate_title_group_dm);

	ret = g_test_run();

	test_ui_purple_uninit();

	return ret;
}
