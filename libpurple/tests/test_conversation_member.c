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

	info = purple_contact_info_new("abc123");

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	member = g_object_new(
		PURPLE_TYPE_CONVERSATION_MEMBER,
		"contact-info", info,
		NULL);

	/* Now use g_object_get to read all of the properties. */
	g_object_get(member,
		"contact-info", &info1,
		"tags", &tags,
		NULL);

	/* Compare all the things. */
	g_assert_true(info1 == info);
	g_assert_true(PURPLE_IS_TAGS(tags));

	/* Free/unref all the things. */
	g_clear_object(&info1);
	g_clear_object(&tags);

	g_clear_object(&info);
	g_clear_object(&member);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/conversation-member/new",
	                test_purple_conversation_member_new);
	g_test_add_func("/conversation-member/properties",
	                test_purple_conversation_member_properties);

	return g_test_run();
}
