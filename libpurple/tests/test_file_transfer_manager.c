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
 * Add/Remove Test
 *****************************************************************************/
static void
test_purple_file_transfer_manager_add_remove_cb(PurpleFileTransferManager *manager,
                                                PurpleFileTransfer *transfer,
                                                gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_FILE_TRANSFER_MANAGER(manager));
	g_assert_true(PURPLE_IS_FILE_TRANSFER(transfer));

	*counter = *counter + 1;
}

static void
test_purple_file_transfer_manager_add_remove(void) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransfer *transfer = NULL;
	PurpleFileTransferManager *manager = NULL;
	guint added = 0;
	guint removed = 0;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);

	manager = g_object_new(PURPLE_TYPE_FILE_TRANSFER_MANAGER, NULL);
	g_signal_connect(manager, "added",
	                 G_CALLBACK(test_purple_file_transfer_manager_add_remove_cb),
	                 &added);
	g_signal_connect(manager, "removed",
	                 G_CALLBACK(test_purple_file_transfer_manager_add_remove_cb),
	                 &removed);

	g_assert_cmpuint(g_list_model_get_n_items(G_LIST_MODEL(manager)), ==, 0);

	/* Add the transfer, verify that it was added, and that the signal was
	 * emitted.
	 */
	transfer = purple_file_transfer_new_receive(account, remote, "foo.bar", 0);
	purple_file_transfer_manager_add(manager, transfer);
	g_assert_cmpuint(g_list_model_get_n_items(G_LIST_MODEL(manager)), ==, 1);
	g_assert_cmpuint(added, ==, 1);
	g_assert_cmpuint(removed, ==, 0);

	/* Remove the transfer, verify that it was removed, and that the signal
	 * was emitted.
	 */
	purple_file_transfer_manager_remove(manager, transfer);
	g_assert_cmpuint(g_list_model_get_n_items(G_LIST_MODEL(manager)), ==, 0);
	g_assert_cmpuint(added, ==, 1);
	g_assert_cmpuint(removed, ==, 1);

	/* Cleanup */
	g_clear_object(&transfer);
	g_clear_object(&manager);
	g_clear_object(&remote);
	g_clear_object(&account);
}

/******************************************************************************
 * Signal Propagation Tests
 *****************************************************************************/
static void
test_purple_file_transfer_manager_transfer_changed_cb(PurpleFileTransferManager *manager,
                                                      PurpleFileTransfer *transfer,
                                                      GParamSpec *pspec,
                                                      gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_FILE_TRANSFER_MANAGER(manager));
	g_assert_true(PURPLE_IS_FILE_TRANSFER(transfer));
	g_assert_true(G_IS_PARAM_SPEC(pspec));

	*counter = *counter + 1;
}

static void
test_purple_file_transfer_manager_propagates_notify(void) {
	PurpleAccount *account = NULL;
	PurpleContactInfo *remote = NULL;
	PurpleFileTransferManager *manager = NULL;
	PurpleFileTransfer *transfer = NULL;
	guint generic_counter = 0;
	guint specific_counter = 0;

	account = purple_account_new("test", "test");
	remote = purple_contact_info_new(NULL);

	manager = g_object_new(PURPLE_TYPE_FILE_TRANSFER_MANAGER, NULL);
	g_signal_connect(manager, "transfer-changed",
	                 G_CALLBACK(test_purple_file_transfer_manager_transfer_changed_cb),
	                 &generic_counter);
	g_signal_connect(manager, "transfer-changed::state",
	                 G_CALLBACK(test_purple_file_transfer_manager_transfer_changed_cb),
	                 &specific_counter);

	transfer = purple_file_transfer_new_receive(account, remote, "foo.bar", 0);
	purple_file_transfer_manager_add(manager, transfer);

	/* Verify our counters are still 0. */
	g_assert_cmpuint(generic_counter, ==, 0);
	g_assert_cmpuint(specific_counter, ==, 0);

	/* Change the state property and verify that each counter was incremented
	 * by 1.
	 */
	purple_file_transfer_set_state(transfer,
	                               PURPLE_FILE_TRANSFER_STATE_NEGOTIATING);
	g_assert_cmpuint(generic_counter, ==, 1);
	g_assert_cmpuint(specific_counter, ==, 1);

	/* Now set another property and verify that only the generic counter was
	 * incremented.
	 */
	purple_file_transfer_set_message(transfer, "heyo");
	g_assert_cmpuint(generic_counter, ==, 2);
	g_assert_cmpuint(specific_counter, ==, 1);

	g_clear_object(&transfer);
	g_clear_object(&manager);
	g_clear_object(&remote);
	g_clear_object(&account);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	gint ret = 0;

	g_test_init(&argc, &argv, NULL);

	g_test_set_nonfatal_assertions();

	test_ui_purple_init();

	g_test_add_func("/file-transfer-manager/add-remove",
	                test_purple_file_transfer_manager_add_remove);

	g_test_add_func("/file-transfer-manager/propagates-notify",
	                test_purple_file_transfer_manager_propagates_notify);

	ret = g_test_run();

	test_ui_purple_uninit();

	return ret;
}
