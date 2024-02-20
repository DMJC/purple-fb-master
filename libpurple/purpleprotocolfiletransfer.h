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

#ifndef PURPLE_PROTOCOL_FILE_TRANSFER_H
#define PURPLE_PROTOCOL_FILE_TRANSFER_H

#include <glib.h>
#include <glib-object.h>

#include "purplefiletransfer.h"
#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_FILE_TRANSFER (purple_protocol_file_transfer_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolFileTransfer,
                    purple_protocol_file_transfer, PURPLE,
                    PROTOCOL_FILE_TRANSFER, PurpleProtocol)

/**
 * PurpleProtocolFileTransfer:
 *
 * #PurpleProtocolFileTransfer provides methods for sending and receiving
 * files.
 *
 * Since: 3.0.0
 */

/**
 * PurpleProtocolFileTransferInterface:
 *
 * This interface defines the behavior for sending and receiving files.
 *
 * Since: 3.0.0
 */
struct _PurpleProtocolFileTransferInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	void (*send_async)(PurpleProtocolFileTransfer *protocol, PurpleFileTransfer *transfer, GAsyncReadyCallback callback, gpointer data);
	gboolean (*send_finish)(PurpleProtocolFileTransfer *protocol, GAsyncResult *result, GError **error);

	void (*receive_async)(PurpleProtocolFileTransfer *protocol, PurpleFileTransfer *transfer, GAsyncReadyCallback callback, gpointer data);
	gboolean (*receive_finish)(PurpleProtocolFileTransfer *protocol, GAsyncResult *result, GError **error);

	/*< private >*/
	gpointer reserved[4];
};

G_BEGIN_DECLS

/**
 * purple_protocol_file_transfer_send_async:
 * @protocol: The instance.
 * @transfer: The file transfer to send.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is
 *            satisfied.
 * @data: User data to pass to @callback.
 *
 * Starts the process of sending @transfer.
 *
 * > Note: This function does not take a cancellable as the protocol should be
 * using [property@FileTransfer:cancellable] from @transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_file_transfer_send_async(PurpleProtocolFileTransfer *protocol, PurpleFileTransfer *transfer, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_file_transfer_send_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolFileTransfer.send_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@ProtocolFileTransfer.send_async] and
 * gets the result.
 *
 * Returns: %TRUE if the transfer was started successfully, otherwise %FALSE
 *          with @error possibly set.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_file_transfer_send_finish(PurpleProtocolFileTransfer *protocol, GAsyncResult *result, GError **error);

/**
 * purple_protocol_file_transfer_receive_async:
 * @protocol: The instance.
 * @transfer: The file transfer to send.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is
 *            satisfied.
 * @data: User data to pass to @callback.
 *
 * Starts the process of receiving @transfer.
 *
 * > Note: This function does not take a cancellable as the protocol should be
 * using [property@FileTransfer:cancellable] from @transfer.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_file_transfer_receive_async(PurpleProtocolFileTransfer *protocol, PurpleFileTransfer *transfer, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_file_transfer_receive_finish:
 * @protocol: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolFileTransfer.receive_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@ProtocolFileTransfer.receive_async] and
 * gets the result.
 *
 * Returns: %TRUE if the transfer was started successfully, otherwise %FALSE
 *          with @error possibly set.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_file_transfer_receive_finish(PurpleProtocolFileTransfer *protocol, GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_FILE_TRANSFER_H */
