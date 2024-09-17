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
test_purple_notification_link_new(void) {
	PurpleNotification *notification = NULL;

	notification = purple_notification_link_new(NULL, "insert title", NULL,
	                                            "https://pidgin.im/");

	g_assert_true(PURPLE_IS_NOTIFICATION(notification));
	g_assert_true(PURPLE_IS_NOTIFICATION_LINK(notification));

	g_assert_finalize_object(notification);
}

static void
test_purple_notification_link_properties(void) {
	PurpleNotification *notification = NULL;
	char *id = NULL;
	char *title = NULL;
	char *link_text = NULL;
	char *link_uri = NULL;

	notification = g_object_new(
		PURPLE_TYPE_NOTIFICATION_LINK,
		"id", "notification1",
		"title", "title1",
		"link-text", "pidgin.im",
		"link-uri", "https://pidgin.im/",
		NULL);

	g_object_get(
		notification,
		"id", &id,
		"title", &title,
		"link-text", &link_text,
		"link-uri", &link_uri,
		NULL);

	g_assert_cmpstr(id, ==, "notification1");
	g_clear_pointer(&id, g_free);

	g_assert_cmpstr(title, ==, "title1");
	g_clear_pointer(&title, g_free);

	g_assert_cmpstr(link_text, ==, "pidgin.im");
	g_clear_pointer(&link_text, g_free);

	g_assert_cmpstr(link_uri, ==, "https://pidgin.im/");
	g_clear_pointer(&link_uri, g_free);

	g_assert_finalize_object(notification);
}

static void
test_purple_notification_link_null_link_text(void) {
	PurpleNotification *notification = NULL;
	const char *link_text = NULL;

	notification = purple_notification_link_new(NULL, "title1", NULL,
	                                            "https://pidgin.im/");

	link_text = purple_notification_link_get_link_text(PURPLE_NOTIFICATION_LINK(notification));
	g_assert_cmpstr(link_text, ==, "https://pidgin.im/");

	g_assert_finalize_object(notification);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/notification/link/new",
	                test_purple_notification_link_new);
	g_test_add_func("/notification/link/properties",
	                test_purple_notification_link_properties);
	g_test_add_func("/notification/link/null-link-text",
	                test_purple_notification_link_null_link_text);

	return g_test_run();
}
