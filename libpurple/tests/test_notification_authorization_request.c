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
test_purple_notification_authorization_request_notify_cb(GObject *obj,
                                                         G_GNUC_UNUSED GParamSpec *pspec,
                                                         gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_NOTIFICATION(obj));

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_notification_authorization_request_new(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleAuthorizationRequest *request1 = NULL;
	PurpleNotification *notification = NULL;
	PurpleNotificationAuthorizationRequest *auth_notification = NULL;
	const char *id = NULL;

	account = purple_account_new("test", "test");
	request = purple_authorization_request_new(account, "remote-username");
	notification = purple_notification_authorization_request_new("id",
	                                                             request);

	g_assert_true(PURPLE_IS_NOTIFICATION(notification));
	g_assert_true(PURPLE_IS_NOTIFICATION_AUTHORIZATION_REQUEST(notification));

	id = purple_notification_get_id(PURPLE_NOTIFICATION(notification));
	g_assert_cmpstr(id, ==, "id");

	auth_notification = PURPLE_NOTIFICATION_AUTHORIZATION_REQUEST(notification);
	request1 = purple_notification_authorization_request_get_request(auth_notification);
	g_assert_true(request1 == request);

	g_assert_finalize_object(notification);

	g_clear_object(&request);
	g_clear_object(&account);
}

static void
test_purple_notification_authorization_request_properties(void) {
	PurpleAccount *account = NULL;
	PurpleAccount *account1 = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleAuthorizationRequest *request1 = NULL;
	PurpleNotification *notification = NULL;
	char *id = NULL;

	account = purple_account_new("test", "test");
	request = purple_authorization_request_new(account, "username");

	notification = g_object_new(
		PURPLE_TYPE_NOTIFICATION_AUTHORIZATION_REQUEST,
		"authorization-request", request,
		"id", "id1",
		NULL);

	g_object_get(
		notification,
		"account", &account1,
		"authorization-request", &request1,
		"id", &id,
		NULL);

	g_assert_true(account1 == account);
	g_clear_object(&account1);

	g_assert_true(request1 == request);
	g_clear_object(&request1);

	g_assert_cmpstr(id, ==, "id1");
	g_clear_pointer(&id, g_free);

	g_assert_finalize_object(notification);

	g_clear_object(&request);
	g_clear_object(&account);
}

static void
test_purple_notification_authorization_request_updates_title(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleNotification *notification = NULL;
	guint counter = 0;

	account = purple_account_new("test", "test");
	request = purple_authorization_request_new(account, "remote-username");

	notification = purple_notification_authorization_request_new(NULL,
	                                                             request);
	g_signal_connect(notification, "notify::title",
	                 G_CALLBACK(test_purple_notification_authorization_request_notify_cb),
	                 &counter);

	g_assert_cmpuint(counter, ==, 0);

	purple_authorization_request_set_alias(request, "foo");

	g_assert_cmpuint(counter, ==, 1);

	g_assert_finalize_object(notification);

	g_clear_object(&request);
	g_clear_object(&account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/notification-request-authorization/new",
	                test_purple_notification_authorization_request_new);
	g_test_add_func("/notification-request-authorization/properties",
	                test_purple_notification_authorization_request_properties);
	g_test_add_func("/notification-request-authorization/updates-title",
	                test_purple_notification_authorization_request_updates_title);

	return g_test_run();
}
