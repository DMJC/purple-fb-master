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
test_purple_authorization_request_accepted_counter_cb(G_GNUC_UNUSED PurpleAuthorizationRequest *request,
                                                      gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

static void
test_purple_authorization_request_denied_counter_cb(G_GNUC_UNUSED PurpleAuthorizationRequest *request,
                                                    G_GNUC_UNUSED const char *message,
                                                    gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

static void
test_purple_authorization_request_denied_message_cb(G_GNUC_UNUSED PurpleAuthorizationRequest *request,
                                                    const char *message,
                                                    gpointer data)
{
	char *expected = data;

	g_assert_cmpstr(message, ==, expected);
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_authorization_request_new(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleContact *contact = NULL;
	PurpleContact *contact1 = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);
	request = purple_authorization_request_new(contact);

	/* Make sure we got a valid authorization request. */
	g_assert_true(PURPLE_IS_AUTHORIZATION_REQUEST(request));

	/* Verify the contact is set properly. */
	contact1 = purple_authorization_request_get_contact(request);
	g_assert_true(contact1 == contact);

	g_assert_finalize_object(request);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(account);
}

static void
test_purple_authorization_request_properties(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleContact *contact = NULL;
	PurpleContact *contact1 = NULL;
	char *message = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);

	request = g_object_new(
		PURPLE_TYPE_AUTHORIZATION_REQUEST,
		"contact", contact,
		"message", "hello friend",
		NULL);

	g_assert_true(PURPLE_IS_AUTHORIZATION_REQUEST(request));

	g_object_get(
		request,
		"contact", &contact1,
		"message", &message,
		NULL);

	g_assert_true(contact1 == contact);
	g_clear_object(&contact1);

	g_assert_cmpstr(message, ==, "hello friend");
	g_clear_pointer(&message, g_free);

	g_assert_finalize_object(request);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(account);
}

static void
test_purple_authorization_request_accept(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleAuthorizationRequest *request = NULL;
		PurpleContact *contact = NULL;
		guint counter = 0;

		account = purple_account_new("test", "test");
		contact = purple_contact_new(account, NULL);
		request = purple_authorization_request_new(contact);

		g_signal_connect(request, "accepted",
		                 G_CALLBACK(test_purple_authorization_request_accepted_counter_cb),
		                 &counter);

		/* Accept the request and verify that the callback was called. */
		purple_authorization_request_accept(request);
		g_assert_cmpuint(counter, ==, 1);

		/* Accept the request again to trigger the critical. */
		purple_authorization_request_accept(request);
		g_assert_cmpuint(counter, ==, 1);

		/* Cleanup. */
		g_assert_finalize_object(request);
		g_assert_finalize_object(contact);
		g_assert_finalize_object(account);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-CRITICAL*request->handled*failed*");
}

static void
test_purple_authorization_request_accept_deny(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleAuthorizationRequest *request = NULL;
		PurpleContact *contact = NULL;
		guint counter = 0;

		account = purple_account_new("test", "test");
		contact = purple_contact_new(account, NULL);
		request = purple_authorization_request_new(contact);

		g_signal_connect(request, "accepted",
		                 G_CALLBACK(test_purple_authorization_request_accepted_counter_cb),
		                 &counter);
		g_signal_connect(request, "denied",
		                 G_CALLBACK(test_purple_authorization_request_denied_counter_cb),
		                 &counter);

		/* Accept the request and verify that the callback was called. */
		purple_authorization_request_accept(request);
		g_assert_cmpuint(counter, ==, 1);

		/* Deny the request to trigger the critical. */
		purple_authorization_request_deny(request, NULL);
		g_assert_cmpuint(counter, ==, 1);

		/* Cleanup. */
		g_assert_finalize_object(request);
		g_assert_finalize_object(contact);
		g_assert_finalize_object(account);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-CRITICAL*request->handled*failed*");
}

static void
test_purple_authorization_request_deny(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleAuthorizationRequest *request = NULL;
		PurpleContact *contact = NULL;
		guint counter = 0;

		account = purple_account_new("test", "test");
		contact = purple_contact_new(account, NULL);
		request = purple_authorization_request_new(contact);

		g_signal_connect(request, "denied",
		                 G_CALLBACK(test_purple_authorization_request_denied_counter_cb),
		                 &counter);

		/* Deny the request and verify that the callback was called. */
		purple_authorization_request_deny(request, NULL);
		g_assert_cmpuint(counter, ==, 1);

		/* Deny the request again to trigger the critical. */
		purple_authorization_request_deny(request, NULL);
		g_assert_cmpuint(counter, ==, 1);

		/* Cleanup. */
		g_assert_finalize_object(request);
		g_assert_finalize_object(contact);
		g_assert_finalize_object(account);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-CRITICAL*request->handled*failed*");
}

static void
test_purple_authorization_request_deny_accept(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleAuthorizationRequest *request = NULL;
		PurpleContact *contact = NULL;
		guint counter = 0;

		account = purple_account_new("test", "test");
		contact = purple_contact_new(account, NULL);
		request = purple_authorization_request_new(contact);

		g_signal_connect(request, "denied",
		                 G_CALLBACK(test_purple_authorization_request_denied_counter_cb),
		                 &counter);
		g_signal_connect(request, "accepted",
		                 G_CALLBACK(test_purple_authorization_request_accepted_counter_cb),
		                 &counter);

		/* Deny the request and verify that the callback was called. */
		purple_authorization_request_deny(request, NULL);
		g_assert_cmpuint(counter, ==, 1);

		/* Deny the request again to trigger the critical. */
		purple_authorization_request_accept(request);
		g_assert_cmpuint(counter, ==, 1);

		/* Cleanup. */
		g_assert_finalize_object(request);
		g_assert_finalize_object(contact);
		g_assert_finalize_object(account);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-CRITICAL*request->handled*failed*");
}

static void
test_purple_authorization_request_deny_message_null(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleContact *contact = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);
	request = purple_authorization_request_new(contact);

	g_signal_connect(request, "denied",
	                 G_CALLBACK(test_purple_authorization_request_denied_message_cb),
	                 NULL);

	/* Deny the request the signal handler is expecting a message of NULL. */
	purple_authorization_request_deny(request, NULL);

	/* Cleanup. */
	g_assert_finalize_object(request);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(account);
}

static void
test_purple_authorization_request_deny_message_non_null(void) {
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleContact *contact = NULL;

	account = purple_account_new("test", "test");
	contact = purple_contact_new(account, NULL);
	request = purple_authorization_request_new(contact);

	g_signal_connect(request, "denied",
	                 G_CALLBACK(test_purple_authorization_request_denied_message_cb),
	                 "this is a message");

	/* Deny the request the signal handler is expecting the above message. */
	purple_authorization_request_deny(request, "this is a message");

	/* Cleanup. */
	g_assert_finalize_object(request);
	g_assert_finalize_object(contact);
	g_assert_finalize_object(account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/request-authorization/new",
	                test_purple_authorization_request_new);
	g_test_add_func("/request-authorization/properties",
	                test_purple_authorization_request_properties);

	g_test_add_func("/request-authorization/accept",
	                test_purple_authorization_request_accept);
	g_test_add_func("/request-authorization/accept-deny",
	                test_purple_authorization_request_accept_deny);
	g_test_add_func("/request-authorization/deny",
	                test_purple_authorization_request_deny);
	g_test_add_func("/request-authorization/deny-accept",
	                test_purple_authorization_request_deny_accept);

	g_test_add_func("/request-authorization/deny-message/null",
	                test_purple_authorization_request_deny_message_null);
	g_test_add_func("/request-authorization/deny-message/non-null",
	                test_purple_authorization_request_deny_message_non_null);

	return g_test_run();
}
