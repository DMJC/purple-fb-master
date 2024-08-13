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
test_purple_notification_destroy_data_cb(gpointer data) {
	guint *counter = data;

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_notification_new(void) {
	PurpleAccount *account1 = NULL;
	PurpleAccount *account2 = NULL;
	PurpleNotification *notification = NULL;
	PurpleNotificationType type = PURPLE_NOTIFICATION_TYPE_UNKNOWN;
	GDateTime *created_timestamp = NULL;
	const char *id = NULL;

	account1 = purple_account_new("test", "test");

	notification = purple_notification_new(PURPLE_NOTIFICATION_TYPE_GENERIC,
	                                       account1,
	                                       NULL,
	                                       NULL);

	/* Make sure we got a valid notification. */
	g_assert_true(PURPLE_IS_NOTIFICATION(notification));

	/* Check the type. */
	type = purple_notification_get_notification_type(notification);
	g_assert_cmpint(PURPLE_NOTIFICATION_TYPE_GENERIC, ==, type);

	/* Verify the account is set properly. */
	account2 = purple_notification_get_account(notification);
	g_assert_nonnull(account2);
	g_assert_true(account1 == account2);

	/* Make sure that the id was generated. */
	id = purple_notification_get_id(notification);
	g_assert_nonnull(id);

	/* Make sure that the created-timestamp was set. */
	created_timestamp = purple_notification_get_created_timestamp(notification);
	g_assert_nonnull(created_timestamp);

	g_clear_object(&account1);
	g_assert_finalize_object(notification);
}

static void
test_purple_notification_destroy_data_func(void) {
	PurpleNotification *notification = NULL;
	guint counter = 0;

	/* Create the notification. */
	notification = purple_notification_new(PURPLE_NOTIFICATION_TYPE_GENERIC,
	                                       NULL,
	                                       &counter,
	                                       test_purple_notification_destroy_data_cb);

	/* Sanity check. */
	g_assert_true(PURPLE_IS_NOTIFICATION(notification));

	/* Unref it to force the destroy callback to be called. */
	g_assert_finalize_object(notification);

	/* Make sure the callback was called. */
	g_assert_cmpuint(counter, ==, 1);
}

static void
test_purple_notification_properties(void) {
	PurpleNotification *notification = NULL;
	GDateTime *created_timestamp = NULL;
	GDateTime *created_timestamp1 = NULL;
	char *icon_name1 = NULL;
	char *subtitle1 = NULL;
	char *title1 = NULL;
	gboolean interactive = FALSE;
	gboolean read = FALSE;

	created_timestamp = g_date_time_new_now_utc();

	notification = g_object_new(
		PURPLE_TYPE_NOTIFICATION,
		"created-timestamp", created_timestamp,
		"icon-name", "icon-name",
		"interactive", TRUE,
		"read", TRUE,
		"subtitle", "subtitle",
		"title", "title",
		"type", PURPLE_NOTIFICATION_TYPE_GENERIC,
		NULL);

	g_assert_true(PURPLE_IS_NOTIFICATION(notification));

	g_object_get(
		notification,
		"created-timestamp", &created_timestamp1,
		"icon-name", &icon_name1,
		"interactive", &interactive,
		"read", &read,
		"subtitle", &subtitle1,
		"title", &title1,
		NULL);

	g_assert_nonnull(created_timestamp1);
	g_assert_true(g_date_time_equal(created_timestamp, created_timestamp1));
	g_date_time_unref(created_timestamp1);

	g_assert_cmpstr(title1, ==, "title");
	g_clear_pointer(&title1, g_free);

	g_assert_cmpstr(subtitle1, ==, "subtitle");
	g_clear_pointer(&subtitle1, g_free);

	g_assert_cmpstr(icon_name1, ==, "icon-name");
	g_clear_pointer(&icon_name1, g_free);

	g_assert_true(read);

	g_assert_true(interactive);

	g_date_time_unref(created_timestamp);

	g_assert_finalize_object(notification);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/notification/new",
	                test_purple_notification_new);
	g_test_add_func("/notification/destroy-data-func",
	                test_purple_notification_destroy_data_func);
	g_test_add_func("/notification/properties",
	                test_purple_notification_properties);

	return g_test_run();
}
