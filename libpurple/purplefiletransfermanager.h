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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_FILE_TRANSFER_MANAGER_H
#define PURPLE_FILE_TRANSFER_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "purplefiletransfer.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_FILE_TRANSFER_MANAGER (purple_file_transfer_manager_get_type())

/**
 * PurpleFileTransferManager:
 *
 * Keeps track of the [class@FileTransfer]'s that have been added to it and
 * propagates all of their signals.
 *
 * Since: 3.0
 */

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleFileTransferManager, purple_file_transfer_manager,
                     PURPLE, FILE_TRANSFER_MANAGER, GObject)

/**
 * purple_file_transfer_manager_get_default:
 *
 * Gets the default file transfer manager for libpurple.
 *
 * Returns: (transfer none): The default file transfer manager for libpurple.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleFileTransferManager *purple_file_transfer_manager_get_default(void);

/**
 * purple_file_transfer_manager_get_default_as_model:
 *
 * Gets the default file transfer manager for libpurple as a list model.
 *
 * Returns: (transfer none): The default file transfer manager for libpurple.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GListModel *purple_file_transfer_manager_get_default_as_model(void);

/**
 * purple_file_transfer_manager_add:
 * @manager: The instance.
 * @transfer: (transfer none): The file transfer to add.
 *
 * Adds @transfer to @manager.
 *
 * When a transfer is added to [class@FileTransferManager] the manager will
 * keep track of the transfer and propagate all of its signals.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_file_transfer_manager_add(PurpleFileTransferManager *manager, PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_manager_remove:
 * @manager: The instance.
 * @transfer: The transfer to remove.
 *
 * Removes @transfer from @manager.
 *
 * Returns: %TRUE if the item was removed, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_file_transfer_manager_remove(PurpleFileTransferManager *manager, PurpleFileTransfer *transfer);

G_END_DECLS

#endif /* PURPLE_FILE_TRANSFER_MANAGER_H */
