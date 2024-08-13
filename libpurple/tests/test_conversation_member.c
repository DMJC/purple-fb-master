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
test_purple_conversation_member_notify_counter(GObject *self,
                                               G_GNUC_UNUSED GParamSpec *pspec,
                                               gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(self));

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_conversation_member_new(void) {
	PurpleContactInfo *info = NULL;
	PurpleConversationMember *member = NULL;

	info = purple_contact_info_new(NULL);
	g_assert_true(PURPLE_IS_CONTACT_INFO(info));

	member = purple_conversation_member_new(info);
	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(member));

	g_clear_object(&info);
	g_clear_object(&member);
}

static void
test_purple_conversation_member_properties(void) {
	PurpleContactInfo *info = NULL;
	PurpleContactInfo *info1 = NULL;
	PurpleConversationMember *member = NULL;
	PurpleTags *tags = NULL;
	PurpleTypingState typing_state = PURPLE_TYPING_STATE_NONE;
	char *name_for_display = NULL;
	char *nickname = NULL;

	info = purple_contact_info_new("abc123");

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	member = g_object_new(
		PURPLE_TYPE_CONVERSATION_MEMBER,
		"contact-info", info,
		"nickname", "pidgy",
		"typing-state", PURPLE_TYPING_STATE_TYPING,
		NULL);

	/* Now use g_object_get to read all of the properties. */
	g_object_get(member,
		"contact-info", &info1,
		"name-for-display", &name_for_display,
		"nickname", &nickname,
		"tags", &tags,
		"typing-state", &typing_state,
		NULL);

	g_assert_true(info1 == info);
	g_clear_object(&info1);

	g_assert_cmpstr(name_for_display, ==, "pidgy");
	g_clear_pointer(&name_for_display, g_free);

	g_assert_cmpstr(nickname, ==, "pidgy");
	g_clear_pointer(&nickname, g_free);

	g_assert_true(PURPLE_IS_TAGS(tags));
	g_clear_object(&tags);

	g_assert_cmpint(typing_state, ==, PURPLE_TYPING_STATE_TYPING);

	g_assert_finalize_object(member);
	g_assert_finalize_object(info);
}

static void
test_purple_conversation_member_name_for_display(void) {
	PurpleContactInfo *info = NULL;
	PurpleConversationMember *member = NULL;
	const char *name_for_display = NULL;
	guint counter = 0;

	info = purple_contact_info_new("abc123");
	member = purple_conversation_member_new(info);
	g_signal_connect(member, "notify::name-for-display",
	                 G_CALLBACK(test_purple_conversation_member_notify_counter),
	                 &counter);

	/* Make sure our counter is still 0. */
	g_assert_cmpuint(counter, ==, 0);

	/* Make sure the default falls back to the contact info's id. */
	name_for_display = purple_conversation_member_get_name_for_display(member);
	g_assert_cmpstr(name_for_display, ==, "abc123");

	/* Set the username on the contact info and make sure it propagates up. */
	purple_contact_info_set_username(info, "tron");
	name_for_display = purple_conversation_member_get_name_for_display(member);
	g_assert_cmpstr(name_for_display, ==, "tron");
	g_assert_cmpuint(counter, ==, 1);

	/* Set the nickname on the conversation member and make sure that takes
	 * precedence.
	 */
	purple_conversation_member_set_nickname(member, "rinzler");
	name_for_display = purple_conversation_member_get_name_for_display(member);
	g_assert_cmpstr(name_for_display, ==, "rinzler");
	g_assert_cmpuint(counter, ==, 2);

	/* Remove the nickname and verify it falls back to the value from the
	 * contact info.
	 */
	purple_conversation_member_set_nickname(member, NULL);
	name_for_display = purple_conversation_member_get_name_for_display(member);
	g_assert_cmpstr(name_for_display, ==, "tron");
	g_assert_cmpuint(counter, ==, 3);

	g_assert_finalize_object(member);
	g_assert_finalize_object(info);
}

/******************************************************************************
 * Typing State Timeout
 *****************************************************************************/
static void
test_purple_conversation_manager_timeout_notify(GObject *obj,
                                                G_GNUC_UNUSED GParamSpec *pspec,
                                                gpointer data)
{
	PurpleConversationMember *member = NULL;
	PurpleTypingState state = PURPLE_TYPING_STATE_NONE;
	gboolean *done = data;
	static guint count = 0;

	g_assert_true(PURPLE_IS_CONVERSATION_MEMBER(obj));
	member = PURPLE_CONVERSATION_MEMBER(obj);

	state = purple_conversation_member_get_typing_state(member);
	if(count == 0) {
		g_assert_cmpuint(state, ==, PURPLE_TYPING_STATE_TYPING);
	} else if(count == 1) {
		g_assert_cmpuint(state, ==, PURPLE_TYPING_STATE_NONE);
	} else {
		g_assert_not_reached();
	}

	/* Increment count each time we're called. */
	count++;
	if(count >= 2) {
		*done = TRUE;
	}
}

static gboolean
test_purple_conversation_member_typing_state_fail_safe(gpointer data) {
	gboolean *done = data;

	g_assert_not_reached();

	*done = TRUE;

	return G_SOURCE_REMOVE;
}

static void
test_purple_conversation_member_typing_state_timeout(void) {
	PurpleContactInfo *info = NULL;
	PurpleConversationMember *member = NULL;
	PurpleTypingState typing_state = PURPLE_TYPING_STATE_TYPING;
	gboolean done = FALSE;

	/* Create the member and add a notify callback on the typing-state property
	 * so we can check it and exit the main loop.
	 */
	info = purple_contact_info_new(NULL);
	member = purple_conversation_member_new(info);
	g_signal_connect(member, "notify::typing-state",
	                 G_CALLBACK(test_purple_conversation_manager_timeout_notify),
	                 &done);

	/* Set the state to typing with a timeout of 1 second. */
	purple_conversation_member_set_typing_state(member,
	                                            PURPLE_TYPING_STATE_TYPING, 1);

	/* We add a fail safe to set done after 2 seconds. */
	g_timeout_add_seconds(2,
	                      test_purple_conversation_member_typing_state_fail_safe,
	                      &done);

	while(!done) {
		g_main_context_iteration(NULL, FALSE);
	}

	/* Verify that our state got reset back to PURPLE_TYPING_STATE_NONE. */
	typing_state = purple_conversation_member_get_typing_state(member);
	g_assert_cmpint(typing_state, ==, PURPLE_TYPING_STATE_NONE);

	/* Clean everything up. */
	g_clear_object(&info);
	g_clear_object(&member);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/conversation-member/new",
	                test_purple_conversation_member_new);
	g_test_add_func("/conversation-member/properties",
	                test_purple_conversation_member_properties);
	g_test_add_func("/conversation-member/name-for-display",
	                test_purple_conversation_member_name_for_display);

	g_test_add_func("/conversation-member/typing-state/timeout",
	                test_purple_conversation_member_typing_state_timeout);

	return g_test_run();
}
