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

#define TEST_PURPLE_PROTOCOL_FILE_TRANSFER_DOMAIN \
	(g_quark_from_static_string("test-protocol-file-transfer"))

/******************************************************************************
 * TestProtocolFileTransferEmpty implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolFileTransferEmpty,
                     test_purple_protocol_file_transfer_empty,
                     TEST_PURPLE, PROTOCOL_FILE_TRANSFER_EMPTY, PurpleProtocol)

struct _TestPurpleProtocolFileTransferEmpty {
	PurpleProtocol parent;
};

static void
test_purple_protocol_file_transfer_empty_iface_init(G_GNUC_UNUSED PurpleProtocolFileTransferInterface *iface)
{
}

G_DEFINE_FINAL_TYPE_WITH_CODE(TestPurpleProtocolFileTransferEmpty,
                              test_purple_protocol_file_transfer_empty,
                              PURPLE_TYPE_PROTOCOL,
                              G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_FILE_TRANSFER,
                                                    test_purple_protocol_file_transfer_empty_iface_init))

static void
test_purple_protocol_file_transfer_empty_init(G_GNUC_UNUSED TestPurpleProtocolFileTransferEmpty *empty)
{
}

static void
test_purple_protocol_file_transfer_empty_class_init(G_GNUC_UNUSED TestPurpleProtocolFileTransferEmptyClass *klass)
{
}

/******************************************************************************
 * TestProtocolFileTransferEmpty Tests
 *****************************************************************************/
static void
test_purple_protocol_file_transfer_empty_send_cb(GObject *source,
                                                 GAsyncResult *result,
                                                 G_GNUC_UNUSED gpointer data)
{
	PurpleProtocolFileTransfer *protocol = NULL;
	GError *error = NULL;
	gboolean success = FALSE;

	g_assert_true(PURPLE_IS_PROTOCOL_FILE_TRANSFER(source));
	protocol = PURPLE_PROTOCOL_FILE_TRANSFER(source);

	success = purple_protocol_file_transfer_send_finish(protocol, result,
	                                                    &error);
	g_assert_error(error, PURPLE_PROTOCOL_FILE_TRANSFER_ERROR, 0);
	g_clear_error(&error);
	g_assert_false(success);
}

static void
test_purple_protocol_file_transfer_empty_send(gconstpointer data) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;
	PurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	GFile *local_file = NULL;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);
	local_file = g_file_new_for_path(data);

	transfer = purple_file_transfer_new_send(account, remote, local_file);
	g_clear_object(&account);
	g_clear_object(&remote);
	g_clear_object(&local_file);

	protocol_file_transfer = g_object_new(test_purple_protocol_file_transfer_empty_get_type(),
	                                      NULL);

	purple_protocol_file_transfer_send_async(protocol_file_transfer,
	                                         transfer,
	                                         test_purple_protocol_file_transfer_empty_send_cb,
	                                         NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(transfer);
	g_assert_finalize_object(protocol_file_transfer);
}

static void
test_purple_protocol_file_transfer_empty_receive_cb(GObject *source,
                                                    GAsyncResult *result,
                                                    G_GNUC_UNUSED gpointer data)
{
	PurpleProtocolFileTransfer *protocol = NULL;
	GError *error = NULL;
	gboolean success = FALSE;

	g_assert_true(PURPLE_IS_PROTOCOL_FILE_TRANSFER(source));
	protocol = PURPLE_PROTOCOL_FILE_TRANSFER(source);

	success = purple_protocol_file_transfer_receive_finish(protocol, result,
	                                                       &error);
	g_assert_error(error, PURPLE_PROTOCOL_FILE_TRANSFER_ERROR, 0);
	g_clear_error(&error);
	g_assert_false(success);
}

static void
test_purple_protocol_file_transfer_empty_receive(void) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;
	PurpleProtocolFileTransfer *protocol_file_transfer = NULL;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);

	transfer = purple_file_transfer_new_receive(account, remote,
	                                            "file.png", 0);
	g_clear_object(&account);
	g_clear_object(&remote);

	protocol_file_transfer = g_object_new(test_purple_protocol_file_transfer_empty_get_type(),
	                                      NULL);

	purple_protocol_file_transfer_receive_async(protocol_file_transfer,
	                                            transfer,
	                                            test_purple_protocol_file_transfer_empty_receive_cb,
	                                            NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_finalize_object(transfer);
	g_assert_finalize_object(protocol_file_transfer);
}

/******************************************************************************
 * TestProtocolFileTransfer implementation
 *****************************************************************************/
G_DECLARE_FINAL_TYPE(TestPurpleProtocolFileTransfer,
                     test_purple_protocol_file_transfer, TEST_PURPLE,
                     PROTOCOL_FILE_TRANSFER, PurpleProtocol)

struct _TestPurpleProtocolFileTransfer {
	PurpleProtocol parent;

	gboolean should_error;

	guint send_async;
	guint send_finish;
	guint receive_async;
	guint receive_finish;
};

static void
test_purple_protocol_file_transfer_send_async(PurpleProtocolFileTransfer *r,
                                              PurpleFileTransfer *transfer,
                                              GAsyncReadyCallback callback,
                                              gpointer data)
{
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	GCancellable *cancellable = NULL;
	GTask *task = NULL;

	protocol_file_transfer = TEST_PURPLE_PROTOCOL_FILE_TRANSFER(r);
	protocol_file_transfer->send_async += 1;

	cancellable = purple_file_transfer_get_cancellable(transfer);

	task = g_task_new(r, cancellable, callback, data);
	if(protocol_file_transfer->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_FILE_TRANSFER_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_file_transfer_send_finish(PurpleProtocolFileTransfer *r,
                                               GAsyncResult *result,
                                               GError **error)
{
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;

	protocol_file_transfer = TEST_PURPLE_PROTOCOL_FILE_TRANSFER(r);
	protocol_file_transfer->send_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_file_transfer_receive_async(PurpleProtocolFileTransfer *r,
                                                 PurpleFileTransfer *transfer,
                                                 GAsyncReadyCallback callback,
                                                 gpointer data)
{
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	GCancellable *cancellable = NULL;
	GTask *task = NULL;

	protocol_file_transfer = TEST_PURPLE_PROTOCOL_FILE_TRANSFER(r);
	protocol_file_transfer->receive_async += 1;

	cancellable = purple_file_transfer_get_cancellable(transfer);

	task = g_task_new(r, cancellable, callback, data);
	if(protocol_file_transfer->should_error) {
		GError *error = g_error_new_literal(TEST_PURPLE_PROTOCOL_FILE_TRANSFER_DOMAIN,
		                                    0, "error");
		g_task_return_error(task, error);
	} else {
		g_task_return_boolean(task, TRUE);
	}

	g_clear_object(&task);
}

static gboolean
test_purple_protocol_file_transfer_receive_finish(PurpleProtocolFileTransfer *r,
                                                  GAsyncResult *result,
                                                  GError **error)
{
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;

	protocol_file_transfer = TEST_PURPLE_PROTOCOL_FILE_TRANSFER(r);
	protocol_file_transfer->receive_finish += 1;

	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
test_purple_protocol_file_transfer_iface_init(PurpleProtocolFileTransferInterface *iface) {
	iface->send_async = test_purple_protocol_file_transfer_send_async;
	iface->send_finish = test_purple_protocol_file_transfer_send_finish;
	iface->receive_async = test_purple_protocol_file_transfer_receive_async;
	iface->receive_finish = test_purple_protocol_file_transfer_receive_finish;
}

G_DEFINE_FINAL_TYPE_WITH_CODE(TestPurpleProtocolFileTransfer,
                              test_purple_protocol_file_transfer,
                              PURPLE_TYPE_PROTOCOL,
                              G_IMPLEMENT_INTERFACE(PURPLE_TYPE_PROTOCOL_FILE_TRANSFER,
                                                    test_purple_protocol_file_transfer_iface_init))

static void
test_purple_protocol_file_transfer_init(TestPurpleProtocolFileTransfer *protocol_file_transfer)
{
	protocol_file_transfer->send_async = 0;
	protocol_file_transfer->send_finish = 0;
	protocol_file_transfer->receive_async = 0;
	protocol_file_transfer->receive_finish = 0;
}

static void
test_purple_protocol_file_transfer_class_init(G_GNUC_UNUSED TestPurpleProtocolFileTransferClass *klass)
{
}

/******************************************************************************
 * TestProtocolFileTransfer send tests
 *****************************************************************************/
static void
test_purple_protocol_file_transfer_send_cb(GObject *obj, GAsyncResult *res,
                                           G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolFileTransfer *test_protocol_file_transfer = NULL;
	PurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	protocol_file_transfer = PURPLE_PROTOCOL_FILE_TRANSFER(obj);
	test_protocol_file_transfer = TEST_PURPLE_PROTOCOL_FILE_TRANSFER(obj);

	result = purple_protocol_file_transfer_send_finish(protocol_file_transfer,
	                                                   res, &error);

	if(test_protocol_file_transfer->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_FILE_TRANSFER_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}
}

static void
test_purple_protocol_file_transfer_send_normal(gconstpointer data) {
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;
	GFile *local_file = NULL;
	const char *filename = data;

	protocol_file_transfer = g_object_new(test_purple_protocol_file_transfer_get_type(),
	                                      NULL);

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);
	local_file = g_file_new_for_path(filename);

	transfer = purple_file_transfer_new_send(account, remote, local_file);

	purple_protocol_file_transfer_send_async(PURPLE_PROTOCOL_FILE_TRANSFER(protocol_file_transfer),
	                                         transfer,
	                                         test_purple_protocol_file_transfer_send_cb,
	                                         NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_cmpuint(protocol_file_transfer->send_async, ==, 1);
	g_assert_cmpuint(protocol_file_transfer->send_finish, ==, 1);

	g_clear_object(&account);
	g_assert_finalize_object(transfer);
	g_assert_finalize_object(remote);
	g_assert_finalize_object(local_file);
	g_assert_finalize_object(protocol_file_transfer);
}

static void
test_purple_protocol_file_transfer_send_error_normal(gconstpointer data) {
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;
	GFile *local_file = NULL;
	const char *filename = data;

	protocol_file_transfer = g_object_new(test_purple_protocol_file_transfer_get_type(),
	                                      NULL);
	g_object_set_data(G_OBJECT(protocol_file_transfer), "filename",
	                  (gpointer)filename);
	protocol_file_transfer->should_error = TRUE;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);
	local_file = g_file_new_for_path(filename);

	transfer = purple_file_transfer_new_send(account, remote, local_file);

	purple_protocol_file_transfer_send_async(PURPLE_PROTOCOL_FILE_TRANSFER(protocol_file_transfer),
	                                         transfer,
	                                         test_purple_protocol_file_transfer_send_cb,
	                                         NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_cmpuint(protocol_file_transfer->send_async, ==, 1);
	g_assert_cmpuint(protocol_file_transfer->send_finish, ==, 1);

	g_clear_object(&account);
	g_assert_finalize_object(transfer);
	g_assert_finalize_object(remote);
	g_assert_finalize_object(local_file);
	g_assert_finalize_object(protocol_file_transfer);
}

/******************************************************************************
 * TestProtocolFileTransfer receive tests
 *****************************************************************************/
static void
test_purple_protocol_file_transfer_receive_cb(GObject *obj, GAsyncResult *res,
                                              G_GNUC_UNUSED gpointer data)
{
	TestPurpleProtocolFileTransfer *test_protocol_file_transfer = NULL;
	PurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	GError *error = NULL;
	gboolean result = FALSE;

	protocol_file_transfer = PURPLE_PROTOCOL_FILE_TRANSFER(obj);
	test_protocol_file_transfer = TEST_PURPLE_PROTOCOL_FILE_TRANSFER(obj);

	result = purple_protocol_file_transfer_receive_finish(protocol_file_transfer,
	                                                      res, &error);

	if(test_protocol_file_transfer->should_error) {
		g_assert_error(error, TEST_PURPLE_PROTOCOL_FILE_TRANSFER_DOMAIN, 0);
		g_clear_error(&error);
		g_assert_false(result);
	} else {
		g_assert_no_error(error);
		g_assert_true(result);
	}
}

static void
test_purple_protocol_file_transfer_receive_normal(void) {
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;

	protocol_file_transfer = g_object_new(test_purple_protocol_file_transfer_get_type(),
	                                      NULL);

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);

	transfer = purple_file_transfer_new_receive(account, remote, "file.png",
	                                            0);

	purple_protocol_file_transfer_receive_async(PURPLE_PROTOCOL_FILE_TRANSFER(protocol_file_transfer),
	                                            transfer,
	                                            test_purple_protocol_file_transfer_receive_cb,
	                                            NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_cmpuint(protocol_file_transfer->receive_async, ==, 1);
	g_assert_cmpuint(protocol_file_transfer->receive_finish, ==, 1);

	g_clear_object(&account);
	g_assert_finalize_object(transfer);
	g_assert_finalize_object(remote);
	g_assert_finalize_object(protocol_file_transfer);
}

static void
test_purple_protocol_file_transfer_receive_error_normal(void) {
	TestPurpleProtocolFileTransfer *protocol_file_transfer = NULL;
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;

	protocol_file_transfer = g_object_new(test_purple_protocol_file_transfer_get_type(),
	                                      NULL);
	protocol_file_transfer->should_error = TRUE;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);

	transfer = purple_file_transfer_new_receive(account, remote, "file.png",
	                                            0);

	purple_protocol_file_transfer_receive_async(PURPLE_PROTOCOL_FILE_TRANSFER(protocol_file_transfer),
	                                            transfer,
	                                            test_purple_protocol_file_transfer_receive_cb,
	                                            NULL);

	g_main_context_iteration(NULL, FALSE);

	g_assert_cmpuint(protocol_file_transfer->receive_async, ==, 1);
	g_assert_cmpuint(protocol_file_transfer->receive_finish, ==, 1);

	g_clear_object(&account);
	g_assert_finalize_object(transfer);
	g_assert_finalize_object(remote);
	g_assert_finalize_object(protocol_file_transfer);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_data_func("/protocol-contacts/empty/send", argv[0],
	                     test_purple_protocol_file_transfer_empty_send);
	g_test_add_func("/protocol-contacts/empty/receive",
	                test_purple_protocol_file_transfer_empty_receive);

	g_test_add_data_func("/protocol-contacts/normal/send-normal", argv[0],
	                     test_purple_protocol_file_transfer_send_normal);
	g_test_add_data_func("/protocol-contacts/normal/send-error", argv[0],
	                     test_purple_protocol_file_transfer_send_error_normal);
	g_test_add_func("/protocol-contacts/normal/receive-normal",
	                test_purple_protocol_file_transfer_receive_normal);
	g_test_add_func("/protocol-contacts/normal/receive-error",
	                test_purple_protocol_file_transfer_receive_error_normal);

	return g_test_run();
}
