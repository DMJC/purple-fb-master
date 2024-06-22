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
test_purple_channel_join_details_new(void) {
	PurpleChannelJoinDetails *details = NULL;

	details = purple_channel_join_details_new(0, FALSE, 0, FALSE, 0);

	g_assert_true(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	g_clear_object(&details);
}

static void
test_purple_channel_join_details_properties(void) {
	PurpleChannelJoinDetails *details;
	char *name = NULL;
	char *nickname = NULL;
	char *password = NULL;
	int name_max_length = 0;
	int nickname_max_length = 0;
	int password_max_length = 0;
	gboolean nickname_supported = FALSE;
	gboolean password_supported = FALSE;

	details = g_object_new(
		PURPLE_TYPE_CHANNEL_JOIN_DETAILS,
		"name", "name",
		"name-max-length", 42,
		"nickname", "nickname",
		"nickname-max-length", 1337,
		"nickname-supported", TRUE,
		"password", "hunter2",
		"password-max-length", 8,
		"password-supported", TRUE,
		NULL);

	g_object_get(
		details,
		"name", &name,
		"name-max-length", &name_max_length,
		"nickname", &nickname,
		"nickname-max-length", &nickname_max_length,
		"nickname-supported", &nickname_supported,
		"password", &password,
		"password-max-length", &password_max_length,
		"password-supported", &password_supported,
		NULL);

	g_assert_cmpstr(name, ==, "name");
	g_clear_pointer(&name, g_free);

	g_assert_cmpint(name_max_length, ==, 42);

	g_assert_cmpstr(nickname, ==, "nickname");
	g_clear_pointer(&nickname, g_free);

	g_assert_cmpint(nickname_max_length, ==, 1337);

	g_assert_true(nickname_supported);

	g_assert_cmpstr(password, ==, "hunter2");
	g_clear_pointer(&password, g_free);

	g_assert_cmpint(password_max_length, ==, 8);

	g_assert_true(password_supported);

	g_clear_object(&details);
}

static void
test_purple_channel_join_details_merge(void) {
	PurpleChannelJoinDetails *destination = NULL;
	PurpleChannelJoinDetails *source = NULL;
	const char *str = NULL;
	gboolean supported = FALSE;

	source = purple_channel_join_details_new(16, TRUE, 16, TRUE, 0);
	purple_channel_join_details_set_name(source, "name");
	purple_channel_join_details_set_nickname(source, "nickname");
	purple_channel_join_details_set_password(source, "password");

	destination = purple_channel_join_details_new(0, FALSE, 0, FALSE, 0);

	purple_channel_join_details_merge(source, destination);

	str = purple_channel_join_details_get_name(destination);
	g_assert_cmpstr(str, ==, purple_channel_join_details_get_name(source));

	supported = purple_channel_join_details_get_nickname_supported(destination);
	g_assert_true(supported == purple_channel_join_details_get_nickname_supported(source));

	str = purple_channel_join_details_get_nickname(destination);
	g_assert_cmpstr(str, ==, purple_channel_join_details_get_nickname(source));

	supported = purple_channel_join_details_get_password_supported(destination);
	g_assert_true(supported == purple_channel_join_details_get_password_supported(source));

	str = purple_channel_join_details_get_password(destination);
	g_assert_cmpstr(str, ==, purple_channel_join_details_get_password(source));

	g_clear_object(&source);
	g_clear_object(&destination);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_set_nonfatal_assertions();

	g_test_add_func("/channel-join-details/new",
	                test_purple_channel_join_details_new);
	g_test_add_func("/channel-join-details/properties",
	                test_purple_channel_join_details_properties);
	g_test_add_func("/channel-join-details/merge",
	                test_purple_channel_join_details_merge);

	return g_test_run();
}
