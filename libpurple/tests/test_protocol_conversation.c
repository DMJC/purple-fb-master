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

G_DEFINE_TYPE_WITH_CODE(TestPurpleProtocolConversationEmpty,
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
			"name", "this is required at the moment",
			"type", PurpleConversationTypeDM,
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

/******************************************************************************
 * TestProtocolConversation Implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolConversation,
                     test_purple_protocol_conversation, TEST_PURPLE,
                     PROTOCOL_CONVERSATION, PurpleProtocol)

struct _TestPurpleProtocolConversation {
	PurpleProtocol parent;

	gboolean should_error;

	guint send_message_async;
	guint send_message_finish;
};

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

static void
test_purple_protocol_conversation_iface_init(PurpleProtocolConversationInterface *iface) {
	iface->send_message_async = test_purple_protocol_conversation_send_message_async;
	iface->send_message_finish = test_purple_protocol_conversation_send_message_finish;
}

G_DEFINE_TYPE_WITH_CODE(TestPurpleProtocolConversation, test_purple_protocol_conversation,
                        PURPLE_TYPE_PROTOCOL,
                        G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_CONVERSATION,
                                              test_purple_protocol_conversation_iface_init))

static void
test_purple_protocol_conversation_init(TestPurpleProtocolConversation *protocol)
{
	protocol->send_message_async = 0;
	protocol->send_message_finish = 0;
}

static void
test_purple_protocol_conversation_class_init(G_GNUC_UNUSED TestPurpleProtocolConversationClass *klass)
{
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
		"type", PurpleConversationTypeDM,
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
 * Main
 *****************************************************************************/
gint
main(int argc, char **argv) {
	int ret = 0;

	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	loop = g_main_loop_new(NULL, FALSE);

	g_test_add_func("/protocol-conversation/empty/send-message-async",
	                test_purple_protocol_conversation_empty_send_message_async);
	g_test_add_func("/protocol-conversation/empty/send-message-finish",
	                test_purple_protocol_conversation_empty_send_message_finish);

	g_test_add_data_func("/protocol-contacts/normal/send-message-normal",
	                     GINT_TO_POINTER(FALSE),
	                     test_purple_protocol_conversation_send_message_normal);
	g_test_add_data_func("/protocol-contacts/normal/send-message-error",
	                     GINT_TO_POINTER(TRUE),
	                     test_purple_protocol_conversation_send_message_normal);

	ret = g_test_run();

	g_main_loop_unref(loop);

	test_ui_purple_uninit();

	return ret;
}
