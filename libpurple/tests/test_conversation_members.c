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
 * Membership Tests
 *****************************************************************************/
static void
test_purple_conversation_members_membership_signal_cb(PurpleConversationMembers *members,
                                                      PurpleConversationMember *member,
                                                      gboolean announce,
                                                      const char *message,
                                                      gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION_MEMBERS(members));
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member));
	g_assert_true(announce);
	g_assert_cmpstr(message, ==, "announcement message");

	*counter = *counter + 1;
}

static void
test_purple_conversation_members_add_remove(void) {
	PurpleContactInfo *info = NULL;
	PurpleContactInfo *member_info = NULL;
	PurpleConversationMember *member = NULL;
	PurpleConversationMember *member1 = NULL;
	PurpleConversationMember *test_member = NULL;
	PurpleConversationMembers *members = NULL;
	gboolean removed = FALSE;
	guint added_counter = 0;
	guint removed_counter = 0;

	/* Create our instances. The id is just a uuid 4 to help us avoid a
	 * g_warning.
	 */
	members = purple_conversation_members_new();
	info = purple_contact_info_new("745c50ba-1189-48d9-827c-051783026c96");

	/* Connect our signals. */
	g_signal_connect(members, "member-added",
	                 G_CALLBACK(test_purple_conversation_members_membership_signal_cb),
	                 &added_counter);
	g_signal_connect(members, "member-removed",
	                 G_CALLBACK(test_purple_conversation_members_membership_signal_cb),
	                 &removed_counter);

	/* Add the member. */
	added_counter = 0;
	removed_counter = 0;
	member = purple_conversation_members_add_member(members, info, TRUE,
	                                                "announcement message");
	g_assert_cmpint(added_counter, ==, 1);
	g_assert_cmpint(removed_counter, ==, 0);
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member));

	test_member = purple_conversation_members_find_member(members, info);
	g_assert_true(member == test_member);

	/* Add our own reference to the returned member as we use it later to
	 * verify that double remove doesn't do anything.
	 */
	g_object_ref(member);

	/* Try to add the member again, which would just return the existing
	 * member and not emit the signal.
	 */
	added_counter = 0;
	removed_counter = 0;
	member1 = purple_conversation_members_add_member(members, info, TRUE,
	                                                 "announcement message");
	g_assert_cmpint(added_counter, ==, 0);
	g_assert_cmpint(removed_counter, ==, 0);
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member1));
	g_assert_true(member1 == member);

	/* Now remove the member and verify the signal was called. */
	added_counter = 0;
	removed_counter = 0;
	member_info = purple_conversation_member_get_contact_info(member);
	removed = purple_conversation_members_remove_member(members, member_info,
	                                                    TRUE,
	                                                    "announcement message");
	g_assert_true(removed);
	g_assert_cmpint(added_counter, ==, 0);
	g_assert_cmpint(removed_counter, ==, 1);

	test_member = purple_conversation_members_find_member(members, info);
	g_assert_null(test_member);

	/* Try to remove the member again and verify that nothing was removed and
	 * that the signal wasn't emitted.
	 */
	added_counter = 0;
	removed_counter = 0;
	member_info = purple_conversation_member_get_contact_info(member);
	removed = purple_conversation_members_remove_member(members, member_info,
	                                                    TRUE,
	                                                    "announcement message");
	g_assert_false(removed);
	g_assert_cmpint(added_counter, ==, 0);
	g_assert_cmpint(removed_counter, ==, 0);

	/* Clean up everything. */
	g_assert_finalize_object(members);
	g_assert_finalize_object(member);
	g_assert_finalize_object(info);
}

/******************************************************************************
 * Items Changed Tests
 *****************************************************************************/
static void
test_purple_conversation_members_item_changed_cb(GListModel *model,
                                                 guint position,
                                                 guint removed,
                                                 guint added,
                                                 gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION_MEMBERS(model));

	g_assert_cmpuint(position, ==, 1);
	g_assert_cmpuint(removed, ==, 0);
	g_assert_cmpuint(added, ==, 0);

	*counter = *counter + 1;
}

static void
test_purple_conversation_members_item_changed(void) {
	PurpleContactInfo *info = NULL;
	PurpleConversationMembers *members = NULL;
	PurpleConversationMember *member = NULL;
	guint counter = 0;

	members = purple_conversation_members_new();

	/* Add the first member, this is just so we can verify that position is non
	 * zero in the callback handler.
	 */
	info = purple_contact_info_new("first");
	purple_conversation_members_add_member(members, info, FALSE, NULL);
	g_clear_object(&info);

	/* Add the second member and hold on to their pointer as we're going to
	 * make changes to it, to verify that the items-changed signal gets called.
	 */
	info = purple_contact_info_new("second");
	member = purple_conversation_members_add_member(members, info, FALSE, NULL);
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member));

	/* Connect the items-changed signal. We do this after everything is added
	 * to avoid having to deal with the additions which were already verified
	 * by the membership test.
	 */
	g_signal_connect(members, "items-changed",
	                 G_CALLBACK(test_purple_conversation_members_item_changed_cb),
	                 &counter);

	/* Change the alias on the contact info and verify that the signal got
	 * called.
	 */
	counter = 0;
	purple_contact_info_set_alias(info, "aliased");
	g_assert_cmpuint(counter, ==, 1);

	/* Change the nickname on the member. The callback only gets called once,
	 * because the members is only watching the name-for-display property.
	 */
	counter = 0;
	purple_conversation_member_set_nickname(member, "nickname");
	g_assert_cmpuint(counter, ==, 1);

	/* Change the typing-state on the member. */
	counter = 0;
	purple_conversation_member_set_typing_state(member,
	                                            PURPLE_TYPING_STATE_TYPING, 6);
	g_assert_cmpuint(counter, ==, 1);

	g_assert_finalize_object(members);
	g_assert_finalize_object(info);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/conversation-members/add-remove",
	                test_purple_conversation_members_add_remove);
	g_test_add_func("/conversation-members/items-changed",
	                test_purple_conversation_members_item_changed);

	return g_test_run();
}
