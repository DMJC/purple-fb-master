/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_FILE_TRANSFER_H
#define PURPLE_FILE_TRANSFER_H

#include <glib-object.h>

#include "purpleaccount.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleFileTransferState:
 * @PURPLE_FILE_TRANSFER_STATE_UNKNOWN: The transfer is in an unknown state.
 * @PURPLE_FILE_TRANSFER_STATE_NEGOTIATING: The transfer is still being
 *                                          negotiated. This includes
 *                                          determining who's actually sending
 *                                          as well as waiting for the
 *                                          receiving user to select a
 *                                          destination file.
 * @PURPLE_FILE_TRANSFER_STATE_STARTED: The transfer is in progress.
 * @PURPLE_FILE_TRANSFER_STATE_FINISHED: The transfer has completed
 *                                       successfully.
 * @PURPLE_FILE_TRANSFER_STATE_FAILED: The transfer failed.
 */
typedef enum {
	PURPLE_FILE_TRANSFER_STATE_UNKNOWN,
	PURPLE_FILE_TRANSFER_STATE_NEGOTIATING,
	PURPLE_FILE_TRANSFER_STATE_STARTED,
	PURPLE_FILE_TRANSFER_STATE_FINISHED,
	PURPLE_FILE_TRANSFER_STATE_FAILED,
} PurpleFileTransferState;

/**
 * PurpleFileTransfer:
 *
 * A peer to peer file transfer object.
 *
 * Since: 3.0.0
 */

#define PURPLE_TYPE_FILE_TRANSFER purple_file_transfer_get_type()

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleFileTransfer, purple_file_transfer, PURPLE,
                     FILE_TRANSFER, GObject)

/**
 * purple_file_transfer_new_send:
 * @account: The account this file transfer is from.
 * @remote: The [class@ContactInfo] of the remote user.
 * @local_file: The [iface@Gio.File] of the file to send.
 *
 * Creates a new [class@FileTransfer] which will be used to send @local_file to
 * @remote.
 *
 * This will also set [property@FileTransfer:filename] and
 * [property@FileTransfer:file-size] to the proper values
 * for @local_file.
 *
 * Returns: (transfer full): The new file transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleFileTransfer *purple_file_transfer_new_send(PurpleAccount *account, PurpleContactInfo *remote, GFile *local_file);

/**
 * purple_file_transfer_new_receive:
 * @account: The account this file transfer is to.
 * @remote: The [class@ContactInfo] of the user sending the file.
 * @filename: (not nullable): The base filename.
 * @file_size: The size of the file.
 *
 * Creates a new [class@FileTransfer] which will be used to negotiate a file
 * transfer from @remote to @account with the given @filename and @file_size.
 *
 * > Note: This function should only be called by a protocol plugin
 * implementation.
 *
 * Returns: (transfer full): The new file transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleFileTransfer *purple_file_transfer_new_receive(PurpleAccount *account, PurpleContactInfo *remote, const char *filename, guint64 file_size);

/**
 * purple_file_transfer_get_account:
 * @transfer: The instance.
 *
 * Gets the [class@Account] that @transfer was created with to transfer a file.
 *
 * Returns: (transfer none): The account.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAccount *purple_file_transfer_get_account(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_get_remote:
 * @transfer: The instance.
 *
 * Gets the [class@ContactInfo] that @transfer was created with to transfer a
 * file with.
 *
 * Returns: (transfer none): The remote contact info.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleContactInfo *purple_file_transfer_get_remote(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_get_initiator:
 * @transfer: The instance.
 *
 * Gets the [class@ContactInfo] of the user that initiated @transfer.
 *
 * Returns: (transfer none): The contact info who initiated the file transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleContactInfo *purple_file_transfer_get_initiator(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_get_cancellable:
 * @transfer: The instance.
 *
 * The [class@Gio.Cancellable] for @transfer that can be used to cancel the
 * transfer.
 *
 * Returns: (transfer none): The cancellable.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GCancellable *purple_file_transfer_get_cancellable(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_get_state:
 * @transfer: The instance.
 *
 * Gets the [enum@FileTransferState] of the transfer.
 *
 * Returns: The state of @transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleFileTransferState purple_file_transfer_get_state(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_set_state:
 * @transfer: The instance.
 * @state: The new state.
 *
 * Sets the state of @transfer to @state.
 *
 * This method should only be called by protocol plugins to match what it is
 * doing.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_file_transfer_set_state(PurpleFileTransfer *transfer, PurpleFileTransferState state);

/**
 * purple_file_transfer_get_error:
 * @transfer: The instance.
 *
 * Gets the #GError from @transfer.
 *
 * Returns: (transfer none) (nullable): The error for the transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GError *purple_file_transfer_get_error(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_set_error:
 * @transfer: The instance.
 * @error: (transfer full) (nullable): The new error.
 *
 * Sets the error of @transfer to @error.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_file_transfer_set_error(PurpleFileTransfer *transfer, GError *error);

/**
 * purple_file_transfer_get_local_file:
 * @transfer: The instance.
 *
 * Gets the local file from @transfer.
 *
 * Returns: (transfer none): The local file.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GFile *purple_file_transfer_get_local_file(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_set_local_file:
 * @transfer: The instance.
 * @local_file: The new local file.
 *
 * Sets the local file of @transfer to @local_file.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_file_transfer_set_local_file(PurpleFileTransfer *transfer, GFile *local_file);

/**
 * purple_file_transfer_get_filename:
 * @transfer: The instance.
 *
 * Gets the base filename from @transfer.
 *
 * Returns: (transfer none): The base filename.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_file_transfer_get_filename(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_get_file_size:
 * @transfer: The instance.
 *
 * Gets the size of the file that is being transferred.
 *
 * Returns: The size of the file in bytes or %0 if the size is unknown.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
guint64 purple_file_transfer_get_file_size(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_get_content_type:
 * @transfer: The instance.
 *
 * Gets the content type of the file being transferred.
 *
 * Returns: (nullable): The content type.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_file_transfer_get_content_type(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_set_content_type:
 * @transfer: The instance.
 * @content_type: (nullable): The new content type.
 *
 * Sets the content type of @transfer to @content_type.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_file_transfer_set_content_type(PurpleFileTransfer *transfer, const char *content_type);

/**
 * purple_file_transfer_get_message:
 * @transfer: The instance.
 *
 * The message that was sent with the file transfer if the protocol supports
 * sending one.
 *
 * Returns: The message sent with the transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_file_transfer_get_message(PurpleFileTransfer *transfer);

/**
 * purple_file_transfer_set_message:
 * @transfer: The instance.
 * @message: (nullable): The new message.
 *
 * Sets the message to send with @transfer to @message.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_file_transfer_set_message(PurpleFileTransfer *transfer, const char *message);

G_END_DECLS

#endif /* PURPLE_FILE_TRANSFER_H */
