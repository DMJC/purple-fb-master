/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include "purplefiletransfermanager.h"
#include "purplefiletransfermanagerprivate.h"

enum {
	SIG_ADDED,
	SIG_REMOVED,
	SIG_TRANSFER_CHANGED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

struct _PurpleFileTransferManager {
	GObject parent;

	GPtrArray *transfers;
};

static PurpleFileTransferManager *default_manager = NULL;

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_file_transfer_manager_transfer_changed_cb(PurpleFileTransfer *transfer,
                                                 GParamSpec *pspec,
                                                 gpointer data)
{
	g_signal_emit(data, signals[SIG_TRANSFER_CHANGED],
	              g_param_spec_get_name_quark(pspec),
	              transfer, pspec);
}

/******************************************************************************
 * GListModel Implementation
 *****************************************************************************/
static GType
purple_file_transfer_manager_get_item_type(G_GNUC_UNUSED GListModel *list) {
	return PURPLE_TYPE_FILE_TRANSFER;
}

static guint
purple_file_transfer_manager_get_n_items(GListModel *list) {
	PurpleFileTransferManager *manager = PURPLE_FILE_TRANSFER_MANAGER(list);

	return manager->transfers->len;
}

static gpointer
purple_file_transfer_manager_get_item(GListModel *list, guint position) {
	PurpleFileTransferManager *manager = PURPLE_FILE_TRANSFER_MANAGER(list);
	PurpleFileTransfer *transfer = NULL;

	if(position < manager->transfers->len) {
		transfer = g_ptr_array_index(manager->transfers, position);
		g_object_ref(transfer);
	}

	return transfer;
}

static void
pidgin_file_transfer_manager_list_model_iface_init(GListModelInterface *iface) {
	iface->get_item_type = purple_file_transfer_manager_get_item_type;
	iface->get_n_items = purple_file_transfer_manager_get_n_items;
	iface->get_item = purple_file_transfer_manager_get_item;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE_WITH_CODE(PurpleFileTransferManager,
                              purple_file_transfer_manager,
                              G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                                    pidgin_file_transfer_manager_list_model_iface_init))

static void
purple_file_transfer_manager_finalize(GObject *obj) {
	PurpleFileTransferManager *manager= PURPLE_FILE_TRANSFER_MANAGER(obj);

	g_clear_pointer(&manager->transfers, g_ptr_array_unref);

	G_OBJECT_CLASS(purple_file_transfer_manager_parent_class)->finalize(obj);
}

static void
purple_file_transfer_manager_init(PurpleFileTransferManager *manager) {
	manager->transfers = g_ptr_array_new_with_free_func(g_object_unref);
}

static void
purple_file_transfer_manager_class_init(PurpleFileTransferManagerClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_file_transfer_manager_finalize;

	/**
	 * PurpleFileTransferManager::added:
	 * @manager: The instance.
	 * @transfer: The [class@FileTransfer] that was added.
	 * @data: User data.
	 *
	 * Emitted when a file transfer is added to @manager.
	 *
	 * Since: 3.0.0
	 */
	signals[SIG_ADDED] = g_signal_new_class_handler(
		"added",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_FILE_TRANSFER);

	/**
	 * PurpleFileTransferManager::removed:
	 * @manager: The instance.
	 * @transfer: The [class@FileTransfer] that was removed.
	 * @data: User data.
	 *
	 * Emitted when a file transfer is removed from @manager.
	 *
	 * Since: 3.0.0
	 */
	signals[SIG_REMOVED] = g_signal_new_class_handler(
		"removed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_FILE_TRANSFER);

	/**
	 * PurpleFileTransferManager::transfer-changed:
	 * @manager: The manager instance.
	 * @transfer: The transfer that changed.
	 * @pspec: The [class@GObject.ParamSpec] for the property that changed.
	 *
	 * This is a propagation of the notify signal from @transfer. This means
	 * that your callback will be called for any file transfer that @manager
	 * knows about.
	 *
	 * This also supports details, so you can specify the signal name as
	 * something like `transfer-changed::state` and your callback will only
	 * be called when the state property of @transfer has been changed.
	 *
	 * Since: 3.0.0
	 */
	signals[SIG_TRANSFER_CHANGED] = g_signal_new_class_handler(
		"transfer-changed",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		PURPLE_TYPE_FILE_TRANSFER,
		G_TYPE_PARAM);
}

/******************************************************************************
 * Private API
 *****************************************************************************/
void
purple_file_transfer_manager_startup(void) {
	if(!PURPLE_IS_FILE_TRANSFER_MANAGER(default_manager)) {
		default_manager = g_object_new(PURPLE_TYPE_FILE_TRANSFER_MANAGER,
		                               NULL);

		g_object_add_weak_pointer(G_OBJECT(default_manager),
		                          (gpointer *)&default_manager);
	}
}

void
purple_file_transfer_manager_shutdown(void) {
	g_clear_object(&default_manager);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleFileTransferManager *
purple_file_transfer_manager_get_default(void) {
	return default_manager;
}

GListModel *
purple_file_transfer_manager_get_default_as_model(void) {
	if(PURPLE_IS_FILE_TRANSFER_MANAGER(default_manager)) {
		return G_LIST_MODEL(default_manager);
	}

	return NULL;
}

void
purple_file_transfer_manager_add(PurpleFileTransferManager *manager,
                                 PurpleFileTransfer *transfer)
{
	guint position = 0;

	g_return_if_fail(PURPLE_IS_FILE_TRANSFER_MANAGER(manager));
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));

	/* Store the len as position before appending to avoid math. */
	position = manager->transfers->len;
	g_ptr_array_add(manager->transfers, g_object_ref(transfer));

	g_signal_connect_object(transfer, "notify",
	                        G_CALLBACK(purple_file_transfer_manager_transfer_changed_cb),
	                        manager, 0);

	g_list_model_items_changed(G_LIST_MODEL(manager), position, 0, 1);

	g_signal_emit(manager, signals[SIG_ADDED], 0, transfer);
}

gboolean
purple_file_transfer_manager_remove(PurpleFileTransferManager *manager,
                                    PurpleFileTransfer *transfer)
{
	gboolean found = FALSE;
	guint position = 0;

	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER_MANAGER(manager), FALSE);
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), FALSE);

	found = g_ptr_array_find(manager->transfers, transfer, &position);
	if(!found) {
		return FALSE;
	}

	/* Temporarily add a ref to the transfer so we can emit the removed
	 * signal without issue.
	 */
	g_object_ref(transfer);

	g_ptr_array_remove_index(manager->transfers, position);

	g_list_model_items_changed(G_LIST_MODEL(manager), position, 1, 0);
	g_signal_emit(manager, signals[SIG_REMOVED], 0, transfer);

	g_object_unref(transfer);

	return TRUE;
}
