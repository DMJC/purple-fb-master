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

	details = purple_channel_join_details_new(FALSE, FALSE);

	g_assert_true(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	g_clear_object(&details);
}

static void
test_purple_channel_join_details_properties(void) {
	PurpleChannelJoinDetails *details;
	char *name = NULL;
	char *nickname = NULL;
	char *password = NULL;
	gboolean nickname_supported = FALSE;
	gboolean password_supported = FALSE;

	details = g_object_new(
		PURPLE_TYPE_CHANNEL_JOIN_DETAILS,
		"name", "name",
		"nickname", "nickname",
		"nickname-supported", TRUE,
		"password", "hunter2",
		"password-supported", TRUE,
		NULL);

	g_object_get(G_OBJECT(details),
		"name", &name,
		"nickname", &nickname,
		"nickname-supported", &nickname_supported,
		"password", &password,
		"password-supported", &password_supported,
		NULL);

	g_assert_cmpstr(name, ==, "name");
	g_clear_pointer(&name, g_free);

	g_assert_cmpstr(nickname, ==, "nickname");
	g_clear_pointer(&nickname, g_free);

	g_assert_true(nickname_supported);

	g_assert_cmpstr(password, ==, "hunter2");
	g_clear_pointer(&password, g_free);

	g_assert_true(password_supported);

	g_clear_object(&details);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/channel-join-details/new",
	                test_purple_channel_join_details_new);
	g_test_add_func("/channel-join-details/properties",
	                test_purple_channel_join_details_properties);

	return g_test_run();
}
