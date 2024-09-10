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
test_purple_notification_add_contact_notify_cb(GObject *obj,
                                               G_GNUC_UNUSED GParamSpec *pspec,
                                               gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_NOTIFICATION(obj));
	g_assert_true(PURPLE_IS_NOTIFICATION_ADD_CONTACT(obj));

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_notification_add_contact_new(void) {
	PurpleAccount *account = NULL;
	PurpleAddContactRequest *request = NULL;
	PurpleContact *contact = NULL;
	PurpleNotification *notification = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, "remote");
	request = purple_add_contact_request_new(contact);
	notification = purple_notification_add_contact_new(NULL, request);

	g_assert_true(PURPLE_IS_NOTIFICATION(notification));
	g_assert_true(PURPLE_IS_NOTIFICATION_ADD_CONTACT(notification));

	g_assert_finalize_object(notification);

	g_clear_object(&request);
	g_clear_object(&contact);
	g_clear_object(&account);
}

static void
test_purple_notification_add_contact_properties(void) {
	PurpleAccount *account = NULL;
	PurpleAccount *account1 = NULL;
	PurpleAddContactRequest *request = NULL;
	PurpleAddContactRequest *request1 = NULL;
	PurpleContact *contact = NULL;
	PurpleNotification *notification = NULL;
	char *id = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, "username");
	request = purple_add_contact_request_new(contact);

	notification = g_object_new(
		PURPLE_TYPE_NOTIFICATION_ADD_CONTACT,
		"id", "notification1",
		"request", request,
		NULL);

	g_object_get(
		notification,
		"account", &account1,
		"id", &id,
		"request", &request1,
		NULL);

	g_assert_true(account1 == account);
	g_clear_object(&account1);

	g_assert_cmpstr(id, ==, "notification1");
	g_clear_pointer(&id, g_free);

	g_assert_true(request1 == request);
	g_clear_object(&request1);

	g_assert_finalize_object(notification);
	g_assert_finalize_object(request);
	g_assert_finalize_object(contact);

	g_clear_object(&account);
}

static void
test_purple_notification_add_contact_updates_title(void) {
	PurpleAccount *account = NULL;
	PurpleAddContactRequest *request = NULL;
	PurpleContact *contact = NULL;
	PurpleNotification *notification = NULL;
	const char *title = NULL;
	guint counter = 0;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, "remote-username");
	request = purple_add_contact_request_new(contact);

	notification = purple_notification_add_contact_new(NULL, request);
	g_signal_connect(notification, "notify::title",
	                 G_CALLBACK(test_purple_notification_add_contact_notify_cb),
	                 &counter);

	/* Make sure the id is in the title after construction. */
	title = purple_notification_get_title(notification);
	g_assert_true(g_pattern_match_simple("*remote-username*", title));

	/* Set the alias and make sure the title gets updated. */
	g_assert_cmpuint(counter, ==, 0);
	purple_contact_info_set_alias(PURPLE_CONTACT_INFO(contact), "test-alias");
	g_assert_cmpuint(counter, ==, 1);

	title = purple_notification_get_title(notification);
	g_assert_true(g_pattern_match_simple("*test-alias*", title));

	g_assert_finalize_object(notification);
	g_assert_finalize_object(request);
	g_assert_finalize_object(contact);

	g_clear_object(&account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/notification/add-contact/new",
	                test_purple_notification_add_contact_new);
	g_test_add_func("/notification/add-contact/properties",
	                test_purple_notification_add_contact_properties);
	g_test_add_func("/notification/add-contact/updates-title",
	                test_purple_notification_add_contact_updates_title);

	return g_test_run();
}
