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
#include <glib/gstdio.h>

#include <purple.h>

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_file_transfer_new_send(gconstpointer data) {
	PurpleAccount *account = NULL;
	PurpleFileTransfer *transfer = NULL;
	PurpleContactInfo *initiator = NULL;
	PurpleContactInfo *remote = NULL;
	GFile *local_file = NULL;
	GStatBuf st;
	char *basename = NULL;
	const char *executable = data;
	const char *filename = NULL;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);
	local_file = g_file_new_for_path(executable);

	transfer = purple_file_transfer_new_send(account, remote, local_file);
	g_assert_true(PURPLE_IS_FILE_TRANSFER(transfer));

	initiator = purple_file_transfer_get_initiator(transfer);
	g_assert_true(initiator == purple_account_get_contact_info(account));

	filename = purple_file_transfer_get_filename(transfer);
	basename = g_path_get_basename(executable);
	g_assert_cmpstr(filename, ==, basename);
	g_clear_pointer(&basename, g_free);

	g_stat(executable, &st);
	g_assert_cmpuint(purple_file_transfer_get_file_size(transfer), ==,
	                 st.st_size);

	g_clear_object(&local_file);
	g_clear_object(&account);
	g_clear_object(&remote);
	g_clear_object(&transfer);
}

static void
test_purple_file_transfer_new_receive(void) {
	PurpleAccount *account = NULL;
	PurpleFileTransfer *transfer = NULL;
	PurpleContactInfo *initiator = NULL;
	PurpleContactInfo *remote = NULL;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);

	transfer = purple_file_transfer_new_receive(account, remote, "foo", 0);
	g_assert_true(PURPLE_IS_FILE_TRANSFER(transfer));

	initiator = purple_file_transfer_get_initiator(transfer);
	g_assert_true(initiator == remote);

	g_clear_object(&account);
	g_clear_object(&remote);
	g_clear_object(&transfer);
}

static void
test_file_transfer_properties(gconstpointer data) {
	PurpleAccount *account = NULL;
	PurpleAccount *account1 = NULL;
	PurpleFileTransfer *transfer = NULL;
	PurpleContactInfo *initiator = NULL;
	PurpleContactInfo *initiator1 = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleContactInfo *remote1 = NULL;
	GCancellable *cancellable = NULL;
	GFile *local_file = NULL;
	GFile *local_file1 = NULL;
	char *content_type = NULL;
	char *filename = NULL;
	char *filename1 = NULL;
	char *message = NULL;
	const char *executable = data;
	guint64 file_size = 1337;
	guint64 file_size1 = 0;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);
	initiator = purple_contact_info_new(NULL);
	local_file = g_file_new_for_path(executable);
	filename = g_path_get_basename(executable);

	/* Create the instance. */
	transfer = g_object_new(
		PURPLE_TYPE_FILE_TRANSFER,
		"account", account,
		"remote", remote,
		"initiator", initiator,
		"local-file", local_file,
		"filename", filename,
		"file-size", file_size,
		"content-type", "application/octet-stream",
		"message", "have you heard the word?",
		NULL);

	/* Get all the property values. */
	g_object_get(
		G_OBJECT(transfer),
		"account", &account1,
		"remote", &remote1,
		"initiator", &initiator1,
		"local-file", &local_file1,
		"filename", &filename1,
		"file-size", &file_size1,
		"content-type", &content_type,
		"message", &message,
		"cancellable", &cancellable,
		NULL);

	/* Validate and clean up all of the properties. */
	g_assert_true(account == account1);
	g_clear_object(&account1);

	g_assert_true(remote == remote1);
	g_clear_object(&remote1);

	g_assert_true(initiator == initiator1);
	g_clear_object(&initiator1);

	g_assert_true(local_file == local_file1);
	g_clear_object(&local_file1);

	g_assert_cmpstr(filename, ==, filename1);
	g_clear_pointer(&filename1, g_free);

	g_assert_cmpuint(file_size, ==, file_size1);

	g_assert_cmpstr(content_type, ==, "application/octet-stream");
	g_clear_pointer(&content_type, g_free);

	g_assert_cmpstr(message, ==, "have you heard the word?");
	g_clear_pointer(&message, g_free);

	g_assert_true(G_IS_CANCELLABLE(cancellable));
	g_clear_object(&cancellable);

	/* Clean up or initial objects. */
	g_clear_pointer(&filename, g_free);
	g_clear_object(&local_file);
	g_clear_object(&initiator);
	g_clear_object(&account);
	g_clear_object(&remote);
	g_clear_object(&transfer);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_data_func("/file-transfer/new/send", argv[0],
	                     test_purple_file_transfer_new_send);
	g_test_add_func("/file-transfer/new/receive",
	                test_purple_file_transfer_new_receive);
	g_test_add_data_func("/file-transfer/properties", argv[0],
	                     test_file_transfer_properties);

	return g_test_run();
}
