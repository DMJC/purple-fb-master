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
	PurpleNotification *notification = NULL;

	account = purple_account_new("test", "test");
	request = purple_authorization_request_new(account, "remote-username");
	notification = purple_notification_authorization_request_new(request);

	g_assert_true(PURPLE_IS_NOTIFICATION(notification));
	g_assert_true(PURPLE_IS_NOTIFICATION_AUTHORIZATION_REQUEST(notification));

	g_assert_finalize_object(notification);

	g_clear_object(&request);
	g_clear_object(&account);
}

static void
test_purple_notification_authorization_request_properties(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleAuthorizationRequest *request1 = NULL;
	PurpleNotification *notification = NULL;

	account = purple_account_new("test", "test");
	request = purple_authorization_request_new(account, "username");

	notification = g_object_new(
		PURPLE_TYPE_NOTIFICATION_AUTHORIZATION_REQUEST,
		"account", account,
		"authorization-request", request,
		NULL);

	g_object_get(
		notification,
		"authorization-request", &request1,
		NULL);

	g_assert_true(request1 == request);
	g_clear_object(&request1);

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

	notification = purple_notification_authorization_request_new(request);
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
gint
main(gint argc, gchar *argv[]) {
	gint ret = 0;

	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	g_test_add_func("/notification-request-authorization/new",
	                test_purple_notification_authorization_request_new);
	g_test_add_func("/notification-request-authorization/properties",
	                test_purple_notification_authorization_request_properties);
	g_test_add_func("/notification-request-authorization/updates-title",
	                test_purple_notification_authorization_request_updates_title);

	ret = g_test_run();

	test_ui_purple_uninit();

	return ret;
}
