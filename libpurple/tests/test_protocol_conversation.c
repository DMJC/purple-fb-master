/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <purple.h>

#include "test_ui.h"

/******************************************************************************
 * Globals
 *****************************************************************************/

/* Since we're using GTask to test asynchronous functions, we need to use a
 * main loop.
 */
static GMainLoop *loop = NULL;

#define TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN (g_quark_from_static_string("test-protocol-conversation"))

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
test_purple_protocol_conversation_timeout_cb(gpointer data) {
	g_main_loop_quit(data);

	g_warning("timed out waiting for the callback function to be called");

	return G_SOURCE_REMOVE;
}

/******************************************************************************
 * TestProtocolConversationEmpty implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolConversationEmpty,
                     test_purple_protocol_conversation_empty,
                     TEST_PURPLE, PROTOCOL_CONVERSATION_EMPTY, PurpleProtocol)

struct _TestPurpleProtocolConversationEmpty {
	PurpleProtocol parent;
};

static void
test_purple_protocol_conversation_empty_iface_init(G_GNUC_UNUSED PurpleProtocolConversationInterface *iface)
{
}

G_DEFINE_FINAL_TYPE_WITH_CODE(TestPurpleProtocolConversationEmpty,
                              test_purple_protocol_conversation_empty,
                              PURPLE_TYPE_PROTOCOL,
                              G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_CONVERSATION,
                                                    test_purple_protocol_conversation_empty_iface_init))

static void
test_purple_protocol_conversation_empty_init(G_GNUC_UNUSED TestPurpleProtocolConversationEmpty *empty)
{
}

static void
test_purple_protocol_conversation_empty_class_init(G_GNUC_UNUSED TestPurpleProtocolConversationEmptyClass *klass)
{
}

/******************************************************************************
 * TestProtocolConversationEmpty Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_empty_implements_create_conversation(void) {
	PurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
	                        NULL);

	g_assert_false(purple_protocol_conversation_implements_create_conversation(protocol));

	g_assert_finalize_object(protocol);
}

static void
test_purple_protocol_conversation_empty_get_create_conversation_details(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleCreateConversationDetails *details = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");
		details = purple_protocol_conversation_get_create_conversation_details(protocol,
		                                                                       account);

		g_assert_null(details);
		g_assert_finalize_object(account);
		g_assert_finalize_object(protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*create_conversation_details*");
}

static void
test_purple_protocol_conversation_empty_create_conversation_async_cb(GObject *source,
                                                                     GAsyncResult *result,
                                                                     G_GNUC_UNUSED gpointer data)
{
	PurpleConversation *conversation = NULL;
	GError *error = NULL;

	g_assert_true(PURPLE_IS_PROTOCOL_CONVERSATION(source));

	conversation = purple_protocol_conversation_create_conversation_finish(PURPLE_PROTOCOL_CONVERSATION(source),
	                                                                       result,
	                                                                       &error);
	g_assert_error(error, PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
	g_clear_error(&error);
	g_assert_null(conversation);
}

static void
test_purple_protocol_conversation_empty_create_conversation_async(void) {
	PurpleAccount *account = NULL;
	PurpleCreateConversationDetails *details = NULL;
	PurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
	                        NULL);
	account = purple_account_new("test", "test");
	details = purple_create_conversation_details_new(0);

	purple_protocol_conversation_create_conversation_async(protocol,
	                                                       account,
	                                                       details,
	                                                       NULL,
	                                                       test_purple_protocol_conversation_empty_create_conversation_async_cb,
	                                                       NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(account);
	g_assert_finalize_object(protocol);
}

static void
test_purple_protocol_conversation_empty_create_conversation_finish(void) {
	if(g_test_subprocess()) {
		PurpleProtocolConversation *protocol = NULL;
		PurpleConversation *conversation = NULL;
		GError *error = NULL;
		GTask *task = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);

		task = g_task_new(protocol, NULL, NULL, NULL);

		conversation = purple_protocol_conversation_create_conversation_finish(protocol,
		                                                                       G_ASYNC_RESULT(task),
		                                                                       &error);
		g_assert_no_error(error);
		g_assert_null(conversation);

		g_assert_finalize_object(task);
		g_assert_finalize_object(protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*create_conversation_finish*");
}

/******************************************************************************
 * Empty Leave Conversation Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_empty_leave_conversation_async_cb(GObject *source,
                                                                    GAsyncResult *result,
                                                                    G_GNUC_UNUSED gpointer data)
{
	GError *error = NULL;
	gboolean left = FALSE;

	g_assert_true(PURPLE_IS_PROTOCOL_CONVERSATION(source));

	left = purple_protocol_conversation_leave_conversation_finish(PURPLE_PROTOCOL_CONVERSATION(source),
	                                                              result,
	                                                              &error);
	g_assert_error(error, PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
	g_clear_error(&error);
	g_assert_false(left);
}

static void
test_purple_protocol_conversation_empty_leave_conversation_async(void) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
	                        NULL);
	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	purple_protocol_conversation_leave_conversation_async(protocol,
	                                                      conversation,
	                                                      NULL,
                                                          test_purple_protocol_conversation_empty_leave_conversation_async_cb,
	                                                      NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(conversation);
	g_assert_finalize_object(account);
	g_assert_finalize_object(protocol);
}

static void
test_purple_protocol_conversation_empty_leave_conversation_finish(void) {
	if(g_test_subprocess()) {
		PurpleProtocolConversation *protocol = NULL;
		GError *error = NULL;
		GTask *task = NULL;
		gboolean left = FALSE;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);

		task = g_task_new(protocol, NULL, NULL, NULL);

		left = purple_protocol_conversation_leave_conversation_finish(protocol,
		                                                              G_ASYNC_RESULT(task),
		                                                              &error);
		g_assert_no_error(error);
		g_assert_false(left);

		g_assert_finalize_object(task);
		g_assert_finalize_object(protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*leave_conversation_finish*");
}

static void
test_purple_protocol_conversation_empty_send_message_async(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleConversation *conversation = NULL;
		PurpleMessage *message = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");
		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"type", PURPLE_CONVERSATION_TYPE_DM,
			NULL);
		message = g_object_new(PURPLE_TYPE_MESSAGE, NULL);

		purple_protocol_conversation_send_message_async(protocol, conversation,
		                                                message, NULL, NULL,
		                                                NULL);

		g_clear_object(&account);
		g_clear_object(&conversation);
		g_clear_object(&message);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*send_message_async*");
}

static void
test_purple_protocol_conversation_empty_send_message_finish(void) {
	if(g_test_subprocess()) {
		PurpleProtocolConversation *protocol = NULL;
		GError *error = NULL;
		GTask *task = NULL;
		gboolean result = FALSE;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);

		task = g_task_new(protocol, NULL, NULL, NULL);

		result = purple_protocol_conversation_send_message_finish(protocol,
		                                                          G_ASYNC_RESULT(task),
		                                                          &error);
		g_assert_no_error(error);
		g_assert_false(result);

		g_clear_object(&task);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*send_message_finish*");
}

static void
test_purple_protocol_conversation_empty_set_topic_async(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleConversation *conversation = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");
		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"type", PURPLE_CONVERSATION_TYPE_DM,
			NULL);

		purple_protocol_conversation_set_topic_async(protocol, conversation,
		                                             "what a topic!", NULL,
		                                             NULL, NULL);

		g_clear_object(&account);
		g_clear_object(&conversation);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*set_topic_async*");
}

static void
test_purple_protocol_conversation_empty_set_topic_finish(void) {
	if(g_test_subprocess()) {
		PurpleProtocolConversation *protocol = NULL;
		GError *error = NULL;
		GTask *task = NULL;
		gboolean result = FALSE;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);

		task = g_task_new(protocol, NULL, NULL, NULL);

		result = purple_protocol_conversation_set_topic_finish(protocol,
		                                                       G_ASYNC_RESULT(task),
		                                                       &error);
		g_assert_no_error(error);
		g_assert_false(result);

		g_clear_object(&task);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*set_topic_finish*");
}

static void
test_purple_protocol_conversation_empty_get_channel_join_details(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleChannelJoinDetails *result = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");

		result = purple_protocol_conversation_get_channel_join_details(protocol,
		                                                               account);

		g_assert_null(result);

		g_clear_object(&account);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*get_channel_join_details*");
}

static void
test_purple_protocol_conversation_empty_join_channel_async(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleChannelJoinDetails *details = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");
		details = purple_channel_join_details_new(FALSE, FALSE);

		purple_protocol_conversation_join_channel_async(protocol, account,
		                                                details, NULL, NULL,
		                                                NULL);

		g_clear_object(&account);
		g_clear_object(&details);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*join_channel_async*");
}

static void
test_purple_protocol_conversation_empty_join_channel_finish(void) {
	if(g_test_subprocess()) {
		PurpleProtocolConversation *protocol = NULL;
		GError *error = NULL;
		GTask *task = NULL;
		gboolean result = FALSE;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);

		task = g_task_new(protocol, NULL, NULL, NULL);

		result = purple_protocol_conversation_join_channel_finish(protocol,
		                                                          G_ASYNC_RESULT(task),
		                                                          &error);
		g_assert_no_error(error);
		g_assert_false(result);

		g_clear_object(&task);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*join_channel_finish*");
}

static void
test_purple_protocol_conversation_empty_set_avatar_async(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleConversation *conversation = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");
		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"type", PURPLE_CONVERSATION_TYPE_DM,
			NULL);

		purple_protocol_conversation_set_avatar_async(protocol, conversation,
		                                              NULL, NULL, NULL, NULL);

		g_clear_object(&account);
		g_clear_object(&conversation);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*set_avatar_async*");
}

static void
test_purple_protocol_conversation_empty_set_avatar_finish(void) {
	if(g_test_subprocess()) {
		PurpleProtocolConversation *protocol = NULL;
		GError *error = NULL;
		GTask *task = NULL;
		gboolean result = FALSE;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);

		task = g_task_new(protocol, NULL, NULL, NULL);

		result = purple_protocol_conversation_set_avatar_finish(protocol,
		                                                        G_ASYNC_RESULT(task),
		                                                        &error);
		g_assert_no_error(error);
		g_assert_false(result);

		g_clear_object(&task);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*set_avatar_finish*");
}

static void
test_purple_protocol_conversation_empty_send_typing(void) {
	if(g_test_subprocess()) {
		PurpleAccount *account = NULL;
		PurpleConversation *conversation = NULL;
		PurpleProtocolConversation *protocol = NULL;

		protocol = g_object_new(test_purple_protocol_conversation_empty_get_type(),
		                        NULL);
		account = purple_account_new("test", "test");
		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			"type", PURPLE_CONVERSATION_TYPE_DM,
			NULL);

		purple_protocol_conversation_send_typing(protocol, conversation,
		                                         PURPLE_TYPING_STATE_PAUSED);

		g_clear_object(&account);
		g_clear_object(&conversation);
		g_clear_object(&protocol);
	}

	g_test_trap_subprocess(NULL, 0, 0);
	g_test_trap_assert_stderr("*Purple-WARNING*TestPurpleProtocolConversationEmpty*send_typing*");
}

/******************************************************************************
 * TestProtocolConversation Implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolConversation,
                     test_purple_protocol_conversation, TEST_PURPLE,
                     PROTOCOL_CONVERSATION, PurpleProtocol)

struct _TestPurpleProtocolConversation {
	PurpleProtocol parent;

	gboolean should_error;

	guint get_create_conversation_details;
	guint create_conversation_async;
	guint create_conversation_finish;

	guint leave_conversation_async;
	guint leave_conversation_finish;

	guint send_message_async;
	guint send_message_finish;

	guint set_topic_async;
	guint set_topic_finish;

	guint get_channel_join_details;
	guint join_channel_async;
	guint join_channel_finish;

	guint set_avatar_async;
	guint set_avatar_finish;

	guint send_typing;
};

static PurpleCreateConversationDetails *
test_purple_protocol_conversation_get_create_conversation_details(PurpleProtocolConversation *protocol,
                                                                  PurpleAccount *account)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->get_create_conversation_details += 1;

	g_assert_true(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_assert_true(PURPLE_IS_ACCOUNT(account));

	return purple_create_conversation_details_new(10);
}

static void
test_purple_protocol_conversation_create_conversation_async(PurpleProtocolConversation *protocol,
                                                            PurpleAccount *account,
                                                            PurpleCreateConversationDetails *details,
                                                            GCancellable *cancellable,
                                                            GAsyncReadyCallback callback,
                                                            gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	GTask *task = NULL;

	g_assert_true(PURPLE_IS_ACCOUNT(account));
	g_assert_true(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	/* details is transfer full and we're done with it. */
	g_object_unref(details);

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->create_conversation_async += 1;

	task = g_task_new(protocol, cancellable, callback, data);
	if(test_protocol->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		PurpleConversation *conversation = NULL;

		conversation = g_object_new(
			PURPLE_TYPE_CONVERSATION,
			"account", account,
			NULL);
		g_task_return_pointer(task, conversation, g_object_unref);
	}

	g_clear_object(&task);
}

static PurpleConversation *
test_purple_protocol_conversation_create_conversation_finish(PurpleProtocolConversation *protocol,
                                                             GAsyncResult *result,
                                                             GError **error)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->create_conversation_finish += 1;

	return g_task_propagate_pointer(G_TASK(result), error);
}

static void
test_purple_protocol_conversation_leave_conversation_async(PurpleProtocolConversation *protocol,
                                                           PurpleConversation *conversation,
                                                           GCancellable *cancellable,
                                                           GAsyncReadyCallback callback,
                                                           gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	GTask *task = NULL;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->leave_conversation_async += 1;

	task = g_task_new(protocol, cancellable, callback, data);
	if(test_protocol->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_conversation_leave_conversation_finish(PurpleProtocolConversation *protocol,
                                                            GAsyncResult *result,
                                                            GError **error)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->leave_conversation_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_conversation_send_message_async(PurpleProtocolConversation *protocol,
                                                     PurpleConversation *conversation,
                                                     PurpleMessage *message,
                                                     GCancellable *cancellable,
                                                     GAsyncReadyCallback callback,
                                                     gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	GTask *task = NULL;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_true(PURPLE_IS_MESSAGE(message));

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->send_message_async += 1;

	task = g_task_new(protocol, cancellable, callback, data);
	if(test_protocol->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_conversation_send_message_finish(PurpleProtocolConversation *protocol,
                                                      GAsyncResult *result,
                                                      GError **error)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->send_message_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static PurpleChannelJoinDetails *
test_purple_protocol_conversation_get_channel_join_details(PurpleProtocolConversation *protocol,
                                                           PurpleAccount *account)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->get_channel_join_details += 1;

	g_assert_true(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_assert_true(PURPLE_IS_ACCOUNT(account));

	return purple_channel_join_details_new(TRUE, TRUE);
}

static void
test_purple_protocol_conversation_join_channel_async(PurpleProtocolConversation *protocol,
                                                     PurpleAccount *account,
                                                     PurpleChannelJoinDetails *details,
                                                     GCancellable *cancellable,
                                                     GAsyncReadyCallback callback,
                                                     gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	GTask *task = NULL;

	g_assert_true(PURPLE_IS_ACCOUNT(account));
	g_assert_true(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->join_channel_async += 1;

	task = g_task_new(protocol, cancellable, callback, data);
	if(test_protocol->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_conversation_join_channel_finish(PurpleProtocolConversation *protocol,
                                                      GAsyncResult *result,
                                                      GError **error)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->join_channel_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_conversation_set_topic_async(PurpleProtocolConversation *protocol,
                                                  PurpleConversation *conversation,
                                                  const char *topic,
                                                  GCancellable *cancellable,
                                                  GAsyncReadyCallback callback,
                                                  gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	GTask *task = NULL;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_false(purple_strempty(topic));

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->set_topic_async += 1;

	task = g_task_new(protocol, cancellable, callback, data);
	if(test_protocol->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_conversation_set_topic_finish(PurpleProtocolConversation *protocol,
                                                   GAsyncResult *result,
                                                   GError **error)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->set_topic_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_conversation_set_avatar_async(PurpleProtocolConversation *protocol,
                                                   PurpleConversation *conversation,
                                                   G_GNUC_UNUSED PurpleAvatar *avatar,
                                                   GCancellable *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	GTask *task = NULL;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->set_avatar_async += 1;

	task = g_task_new(protocol, cancellable, callback, data);
	if(test_protocol->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_conversation_set_avatar_finish(PurpleProtocolConversation *protocol,
                                                    GAsyncResult *result,
                                                    GError **error)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->set_avatar_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_conversation_send_typing(PurpleProtocolConversation *protocol,
                                              PurpleConversation *conversation,
                                              PurpleTypingState state)
{
	TestPurpleProtocolConversation *test_protocol = NULL;

	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);
	test_protocol->send_typing += 1;

	g_assert_true(PURPLE_IS_CONVERSATION(conversation));
	g_assert_true(state == PURPLE_TYPING_STATE_TYPING);
}

static void
test_purple_protocol_conversation_iface_init(PurpleProtocolConversationInterface *iface) {
	iface->get_create_conversation_details = test_purple_protocol_conversation_get_create_conversation_details;
	iface->create_conversation_async = test_purple_protocol_conversation_create_conversation_async;
	iface->create_conversation_finish = test_purple_protocol_conversation_create_conversation_finish;

	iface->leave_conversation_async = test_purple_protocol_conversation_leave_conversation_async;
	iface->leave_conversation_finish = test_purple_protocol_conversation_leave_conversation_finish;

	iface->send_message_async = test_purple_protocol_conversation_send_message_async;
	iface->send_message_finish = test_purple_protocol_conversation_send_message_finish;

	iface->set_topic_async = test_purple_protocol_conversation_set_topic_async;
	iface->set_topic_finish = test_purple_protocol_conversation_set_topic_finish;

	iface->get_channel_join_details = test_purple_protocol_conversation_get_channel_join_details;
	iface->join_channel_async = test_purple_protocol_conversation_join_channel_async;
	iface->join_channel_finish = test_purple_protocol_conversation_join_channel_finish;

	iface->set_avatar_async = test_purple_protocol_conversation_set_avatar_async;
	iface->set_avatar_finish = test_purple_protocol_conversation_set_avatar_finish;

	iface->send_typing = test_purple_protocol_conversation_send_typing;
}

G_DEFINE_FINAL_TYPE_WITH_CODE(TestPurpleProtocolConversation,
                              test_purple_protocol_conversation,
                              PURPLE_TYPE_PROTOCOL,
                              G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_CONVERSATION,
                                                    test_purple_protocol_conversation_iface_init))

static void
test_purple_protocol_conversation_init(TestPurpleProtocolConversation *protocol)
{
	protocol->get_create_conversation_details = 0;
	protocol->create_conversation_async = 0;
	protocol->create_conversation_finish = 0;

	protocol->leave_conversation_async = 0;
	protocol->leave_conversation_finish = 0;

	protocol->send_message_async = 0;
	protocol->send_message_finish = 0;

	protocol->set_topic_async = 0;
	protocol->set_topic_finish = 0;

	protocol->get_channel_join_details = 0;
	protocol->join_channel_async = 0;
	protocol->join_channel_finish = 0;

	protocol->set_avatar_async = 0;
	protocol->set_avatar_finish = 0;

	protocol->send_typing = 0;
}

static void
test_purple_protocol_conversation_class_init(G_GNUC_UNUSED TestPurpleProtocolConversationClass *klass)
{
}

/******************************************************************************
 * TestProtocolConversation CreateConversation Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_implements_create_conversation(void) {
	PurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);

	g_assert_true(purple_protocol_conversation_implements_create_conversation(protocol));

	g_assert_finalize_object(protocol);
}

static void
test_purple_protocol_conversation_get_create_conversation_details_normal(void)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleAccount *account = NULL;
	PurpleCreateConversationDetails *details = NULL;
	PurpleProtocolConversation *protocol = NULL;

	test_protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                             NULL);
	protocol = PURPLE_PROTOCOL_CONVERSATION(test_protocol);

	account = purple_account_new("test", "test");

	details = purple_protocol_conversation_get_create_conversation_details(protocol,
	                                                                       account);

	g_assert_true(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	g_assert_cmpuint(test_protocol->get_create_conversation_details, ==, 1);

	g_assert_finalize_object(details);
	g_assert_finalize_object(account);
	g_assert_finalize_object(test_protocol);
}

static void
test_purple_protocol_conversation_create_conversation_cb(GObject *obj,
                                                         GAsyncResult *result,
                                                         G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleConversation *conversation = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;

	protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(obj);

	conversation = purple_protocol_conversation_create_conversation_finish(protocol,
	                                                                       result,
	                                                                       &error);

	if(test_protocol->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_null(conversation);
	} else {
		g_assert_no_error(error);
		g_assert_true(PURPLE_IS_CONVERSATION(conversation));
		g_clear_object(&conversation);
	}
}

static void
test_purple_protocol_conversation_create_conversation_normal(gconstpointer data)
{
	TestPurpleProtocolConversation *protocol = NULL;
	PurpleAccount *account = NULL;
	PurpleCreateConversationDetails *details = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol->should_error = GPOINTER_TO_INT(data);

	account = purple_account_new("test", "test");
	details = purple_create_conversation_details_new(10);

	purple_protocol_conversation_create_conversation_async(PURPLE_PROTOCOL_CONVERSATION(protocol),
	                                                       account,
	                                                       details,
	                                                       NULL,
	                                                       test_purple_protocol_conversation_create_conversation_cb,
	                                                       NULL);

	while(g_main_context_iteration(NULL, FALSE));

	g_assert_cmpuint(protocol->create_conversation_async, ==, 1);
	g_assert_cmpuint(protocol->create_conversation_finish, ==, 1);

	g_assert_finalize_object(protocol);

	/* We can't use g_assert_finalize_object as current, PurpleAccount's
	 * presence is a PurpleAccountPresence that holds a reference to the
	 * account and we have no way to break that cycle. Trying to use dispose
	 * doesn't work because the account and the presence can't even reach a
	 * reference count of zero.
	 */
	g_clear_object(&account);
}

/******************************************************************************
 * TestProtocolConversation LeaveConversation Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_implements_leave_conversation(void) {
	PurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);

	g_assert_true(purple_protocol_conversation_implements_leave_conversation(protocol));

	g_assert_finalize_object(protocol);
}

static void
test_purple_protocol_conversation_leave_conversation_cb(GObject *obj,
                                                        GAsyncResult *result,
                                                        G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;
	gboolean left = FALSE;

	protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(obj);

	left = purple_protocol_conversation_leave_conversation_finish(protocol,
	                                                              result,
	                                                              &error);

	if(test_protocol->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(left);
	} else {
		g_assert_no_error(error);
		g_assert_true(left);
	}
}

static void
test_purple_protocol_conversation_leave_conversation_normal(gconstpointer data)
{
	TestPurpleProtocolConversation *protocol = NULL;
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol->should_error = GPOINTER_TO_INT(data);

	account = purple_account_new("test", "test");
	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		NULL);

	purple_protocol_conversation_leave_conversation_async(PURPLE_PROTOCOL_CONVERSATION(protocol),
	                                                      conversation,
	                                                      NULL,
	                                                      test_purple_protocol_conversation_leave_conversation_cb,
	                                                      NULL);

	while(g_main_context_iteration(NULL, FALSE));

	g_assert_cmpuint(protocol->leave_conversation_async, ==, 1);
	g_assert_cmpuint(protocol->leave_conversation_finish, ==, 1);

	g_assert_finalize_object(conversation);
	g_assert_finalize_object(account);
	g_assert_finalize_object(protocol);
}

/******************************************************************************
 * TestProtocolConversation SendMessage Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_send_message_cb(GObject *obj,
                                                  GAsyncResult *res,
                                                  G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(obj);

	result = purple_protocol_conversation_send_message_finish(protocol, res,
	                                                          &error);

	if(test_protocol->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}

	g_main_loop_quit(loop);
}

static gboolean
test_purple_protocol_conversation_send_message_idle(gpointer data) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleMessage *message = NULL;
	PurpleProtocolConversation *protocol = data;

	account = purple_account_new("test", "test");
	g_object_set_data_full(G_OBJECT(protocol), "account", account, g_object_unref);

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"name", "this is required at the moment",
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);
	g_object_set_data_full(G_OBJECT(protocol), "conversation", conversation,
	                       g_object_unref);

	message = g_object_new(PURPLE_TYPE_MESSAGE, NULL);
	g_object_set_data_full(G_OBJECT(protocol), "message", message,
	                       g_object_unref);

	purple_protocol_conversation_send_message_async(protocol, conversation,
	                                                message, NULL,
	                                                test_purple_protocol_conversation_send_message_cb,
	                                                NULL);

	return G_SOURCE_REMOVE;
}

static void
test_purple_protocol_conversation_send_message_normal(gconstpointer data) {
	TestPurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol->should_error = GPOINTER_TO_INT(data);

	g_idle_add(test_purple_protocol_conversation_send_message_idle, protocol);
	g_timeout_add_seconds(10, test_purple_protocol_conversation_timeout_cb,
	                      loop);

	g_main_loop_run(loop);

	g_assert_cmpuint(protocol->send_message_async, ==, 1);
	g_assert_cmpuint(protocol->send_message_finish, ==, 1);

	g_clear_object(&protocol);
}

/******************************************************************************
 * TestProtocolConversation SetTopic Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_set_topic_cb(GObject *obj,
                                               GAsyncResult *res,
                                               G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(obj);

	result = purple_protocol_conversation_set_topic_finish(protocol, res,
	                                                       &error);

	if(test_protocol->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}

	g_main_loop_quit(loop);
}

static gboolean
test_purple_protocol_conversation_set_topic_idle(gpointer data) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleProtocolConversation *protocol = data;

	account = purple_account_new("test", "test");
	g_object_set_data_full(G_OBJECT(protocol), "account", account, g_object_unref);

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"name", "this is required at the moment",
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);
	g_object_set_data_full(G_OBJECT(protocol), "conversation", conversation,
	                       g_object_unref);

	purple_protocol_conversation_set_topic_async(protocol, conversation,
	                                             "woo hoo", NULL,
	                                             test_purple_protocol_conversation_set_topic_cb,
	                                             NULL);

	return G_SOURCE_REMOVE;
}

static void
test_purple_protocol_conversation_set_topic_normal(gconstpointer data) {
	TestPurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol->should_error = GPOINTER_TO_INT(data);

	g_idle_add(test_purple_protocol_conversation_set_topic_idle, protocol);
	g_timeout_add_seconds(10, test_purple_protocol_conversation_timeout_cb,
	                      loop);

	g_main_loop_run(loop);

	g_assert_cmpuint(protocol->set_topic_async, ==, 1);
	g_assert_cmpuint(protocol->set_topic_finish, ==, 1);

	g_clear_object(&protocol);
}

/******************************************************************************
 * TestProtocolConversation Channel Join Tests
 ****************************************************************************/
static void
test_purple_protocol_conversation_get_channel_join_details_normal(void) {
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleAccount *account = NULL;
	PurpleChannelJoinDetails *details = NULL;
	PurpleProtocolConversation *protocol = NULL;

	test_protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol = PURPLE_PROTOCOL_CONVERSATION(test_protocol);

	account = purple_account_new("test", "test");

	details = purple_protocol_conversation_get_channel_join_details(protocol,
	                                                                account);

	g_assert_true(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	g_assert_cmpuint(test_protocol->get_channel_join_details, ==, 1);

	g_clear_object(&details);
	g_clear_object(&account);
	g_clear_object(&test_protocol);
}

static void
test_purple_protocol_conversation_join_channel_cb(GObject *obj,
                                                  GAsyncResult *res,
                                                  G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(obj);

	result = purple_protocol_conversation_join_channel_finish(protocol, res,
	                                                          &error);

	if(test_protocol->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}

	g_main_loop_quit(loop);
}

static gboolean
test_purple_protocol_conversation_join_channel_idle(gpointer data) {
	PurpleAccount *account = NULL;
	PurpleChannelJoinDetails *details = NULL;
	PurpleProtocolConversation *protocol = data;

	account = purple_account_new("test", "test");
	g_object_set_data_full(G_OBJECT(protocol), "account", account,
	                       g_object_unref);

	details = purple_channel_join_details_new(FALSE, FALSE);
	g_object_set_data_full(G_OBJECT(protocol), "details", details,
	                       g_object_unref);

	purple_protocol_conversation_join_channel_async(protocol, account, details,
	                                                NULL,
	                                                test_purple_protocol_conversation_join_channel_cb,
	                                                NULL);

	return G_SOURCE_REMOVE;
}

static void
test_purple_protocol_conversation_join_channel_normal(gconstpointer data) {
	TestPurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol->should_error = GPOINTER_TO_INT(data);

	g_idle_add(test_purple_protocol_conversation_join_channel_idle, protocol);
	g_timeout_add_seconds(10, test_purple_protocol_conversation_timeout_cb,
	                      loop);

	g_main_loop_run(loop);

	g_assert_cmpuint(protocol->join_channel_async, ==, 1);
	g_assert_cmpuint(protocol->join_channel_finish, ==, 1);

	g_clear_object(&protocol);
}

/******************************************************************************
 * TestProtocolConversation SetAvatar Tests
 *****************************************************************************/
static void
test_purple_protocol_conversation_set_avatar_cb(GObject *obj,
                                                GAsyncResult *res,
                                                G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleProtocolConversation *protocol = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	protocol = PURPLE_PROTOCOL_CONVERSATION(obj);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(obj);

	result = purple_protocol_conversation_set_avatar_finish(protocol, res,
	                                                        &error);

	if(test_protocol->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}

	g_main_loop_quit(loop);
}

static gboolean
test_purple_protocol_conversation_set_avatar_idle(gpointer data) {
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleProtocolConversation *protocol = data;

	account = purple_account_new("test", "test");
	g_object_set_data_full(G_OBJECT(protocol), "account", account,
	                       g_object_unref);

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"name", "this is required at the moment",
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);
	g_object_set_data_full(G_OBJECT(protocol), "conversation", conversation,
	                       g_object_unref);

	purple_protocol_conversation_set_avatar_async(protocol, conversation,
	                                              NULL, NULL,
	                                              test_purple_protocol_conversation_set_avatar_cb,
	                                              NULL);

	return G_SOURCE_REMOVE;
}

static void
test_purple_protocol_conversation_set_avatar_normal(gconstpointer data) {
	TestPurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	protocol->should_error = GPOINTER_TO_INT(data);

	g_idle_add(test_purple_protocol_conversation_set_avatar_idle, protocol);
	g_timeout_add_seconds(10, test_purple_protocol_conversation_timeout_cb,
	                      loop);

	g_main_loop_run(loop);

	g_assert_cmpuint(protocol->set_avatar_async, ==, 1);
	g_assert_cmpuint(protocol->set_avatar_finish, ==, 1);

	g_clear_object(&protocol);
}

/******************************************************************************
 * TestProtocolConversation Send Typing Tests
 ****************************************************************************/
static void
test_purple_protocol_conversation_send_typing_normal(void) {
	TestPurpleProtocolConversation *test_protocol = NULL;
	PurpleAccount *account = NULL;
	PurpleConversation *conversation = NULL;
	PurpleProtocolConversation *protocol = NULL;

	protocol = g_object_new(test_purple_protocol_conversation_get_type(),
	                        NULL);
	test_protocol = TEST_PURPLE_PROTOCOL_CONVERSATION(protocol);

	account = purple_account_new("test", "test");

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"name", "this is required at the moment",
		"type", PURPLE_CONVERSATION_TYPE_DM,
		NULL);

	test_purple_protocol_conversation_send_typing(protocol, conversation,
	                                              PURPLE_TYPING_STATE_TYPING);

	g_assert_cmpuint(test_protocol->send_typing, ==, 1);

	g_clear_object(&conversation);
	g_clear_object(&account);
	g_clear_object(&protocol);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(int argc, char **argv) {
	int ret = 0;

	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	loop = g_main_loop_new(NULL, FALSE);

	/* Empty create conversation tests. */
	g_test_add_func("/protocol-conversation/empty/implements-create-conversation",
	                test_purple_protocol_conversation_empty_implements_create_conversation);
	g_test_add_func("/protocol-conversation/empty/get-create-conversation-details",
	                test_purple_protocol_conversation_empty_get_create_conversation_details);
	g_test_add_func("/protocol-conversation/empty/create-conversation-async",
	                test_purple_protocol_conversation_empty_create_conversation_async);
	g_test_add_func("/protocol-conversation/empty/create-conversation-finish",
	                test_purple_protocol_conversation_empty_create_conversation_finish);

	/* Empty leave conversation tests. */
	g_test_add_func("/protocol-conversation/empty/leave-conversation-async",
	                test_purple_protocol_conversation_empty_leave_conversation_async);
	g_test_add_func("/protocol-conversation/empty/leave-conversation-finish",
	                test_purple_protocol_conversation_empty_leave_conversation_finish);

	/* Empty send message tests. */
	g_test_add_func("/protocol-conversation/empty/send-message-async",
	                test_purple_protocol_conversation_empty_send_message_async);
	g_test_add_func("/protocol-conversation/empty/send-message-finish",
	                test_purple_protocol_conversation_empty_send_message_finish);
	g_test_add_func("/protocol-conversation/empty/set-topic-async",
	                test_purple_protocol_conversation_empty_set_topic_async);
	g_test_add_func("/protocol-conversation/empty/set-topic-finish",
	                test_purple_protocol_conversation_empty_set_topic_finish);
	g_test_add_func("/protocol-conversation/empty/set-avatar-async",
	                test_purple_protocol_conversation_empty_set_avatar_async);
	g_test_add_func("/protocol-conversation/empty/set-avatar-finish",
	                test_purple_protocol_conversation_empty_set_avatar_finish);

	/* Empty join channel tests. */
	g_test_add_func("/protocol-conversation/empty/get-channel-join-details",
	                test_purple_protocol_conversation_empty_get_channel_join_details);
	g_test_add_func("/protocol-conversation/empty/join_channel_async",
	                test_purple_protocol_conversation_empty_join_channel_async);
	g_test_add_func("/protocol-conversation/empty/join_channel_finish",
	                test_purple_protocol_conversation_empty_join_channel_finish);

	/* Empty send typing tests. */
	g_test_add_func("/protocol-conversation/empty/send-typing",
	                test_purple_protocol_conversation_empty_send_typing);

	/* Normal create conversation tests. */
	g_test_add_func("/protocol-conversation/normal/implements-create-conversation",
	                test_purple_protocol_conversation_implements_create_conversation);
	g_test_add_func("/protocol-conversation/normal/get-create-conversation-details-normal",
	                test_purple_protocol_conversation_get_create_conversation_details_normal);
	g_test_add_data_func("/protocol-conversation/normal/create-conversation-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_create_conversation_normal);
	g_test_add_data_func("/protocol-conversation/normal/create-conversation-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_create_conversation_normal);

	/* Normal leave conversation tests. */
	g_test_add_func("/protocol-conversation/normal/implements-leave-conversation",
	                test_purple_protocol_conversation_implements_leave_conversation);
	g_test_add_data_func("/protocol-conversation/normal/leave-conversation-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_leave_conversation_normal);
	g_test_add_data_func("/protocol-conversation/normal/leave-conversation-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_leave_conversation_normal);

	/* Normal send message tests. */
	g_test_add_data_func("/protocol-conversation/normal/send-message-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_send_message_normal);
	g_test_add_data_func("/protocol-conversation/normal/send-message-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_send_message_normal);

	/* Normal join channel tests. */
	g_test_add_func("/protocol-conversation/normal/get-channel-join-details",
	                test_purple_protocol_conversation_get_channel_join_details_normal);
	g_test_add_data_func("/protocol-conversation/normal/join-channel-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_join_channel_normal);
	g_test_add_data_func("/protocol-conversation/normal/join-channel-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_join_channel_normal);

	/* Normal set topic tests. */
	g_test_add_data_func("/protocol-contacts/normal/set-topic-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_set_topic_normal);
	g_test_add_data_func("/protocol-contacts/normal/set-topic-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_set_topic_normal);

	/* Normal set avatar tests. */
	g_test_add_data_func("/protocol-contacts/normal/set-avatar-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_set_avatar_normal);
	g_test_add_data_func("/protocol-contacts/normal/set-avatar-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_set_avatar_normal);

	/* Normal send typing tests. */
	g_test_add_func("/protocol-conversation/normal/send-typing",
	                test_purple_protocol_conversation_send_typing_normal);

	ret = g_test_run();

	g_main_loop_unref(loop);

	test_ui_purple_uninit();

	return ret;
}
