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

#include "purpleprotocolfiletransfer.h"

/******************************************************************************
 * GInterface Implementation
 *****************************************************************************/
G_DEFINE_INTERFACE(PurpleProtocolFileTransfer, purple_protocol_file_transfer,
                   PURPLE_TYPE_PROTOCOL)

static void
purple_protocol_file_transfer_default_init(G_GNUC_UNUSED PurpleProtocolFileTransferInterface *iface)
{
}

/******************************************************************************
 * Public API
 *****************************************************************************/
void
purple_protocol_file_transfer_send_async(PurpleProtocolFileTransfer *protocol,
                                         PurpleFileTransfer *transfer,
                                         GAsyncReadyCallback callback,
                                         gpointer data)
{
	PurpleProtocolFileTransferInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_FILE_TRANSFER(protocol));

	iface = PURPLE_PROTOCOL_FILE_TRANSFER_GET_IFACE(protocol);
	if(iface != NULL && iface->send_async != NULL) {
		iface->send_async(protocol, transfer, callback, data);
	} else {
		g_task_report_new_error(G_OBJECT(protocol), callback, data,
		                        purple_protocol_file_transfer_send_async,
		                        PURPLE_PROTOCOL_FILE_TRANSFER_ERROR, 0,
		                        "%s does not implement send_async",
		                        G_OBJECT_TYPE_NAME(protocol));
	}
}

gboolean
purple_protocol_file_transfer_send_finish(PurpleProtocolFileTransfer *protocol,
                                          GAsyncResult *result,
                                          GError **error)
{
	PurpleProtocolFileTransferInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_FILE_TRANSFER(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	if(g_async_result_is_tagged(result,
	                            purple_protocol_file_transfer_send_async))
	{
		return g_task_propagate_boolean(G_TASK(result), error);
	}

	iface = PURPLE_PROTOCOL_FILE_TRANSFER_GET_IFACE(protocol);
	if(iface != NULL && iface->send_finish != NULL) {
		return iface->send_finish(protocol, result, error);
	}

	g_warning("purple_protocol_file_transfer_send_finish called without "
	          "calling purple_protocol_file_transfer_send_async");

	return FALSE;
}

void
purple_protocol_file_transfer_receive_async(PurpleProtocolFileTransfer *protocol,
                                            PurpleFileTransfer *transfer,
                                            GAsyncReadyCallback callback,
                                            gpointer data)
{
	PurpleProtocolFileTransferInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_FILE_TRANSFER(protocol));

	iface = PURPLE_PROTOCOL_FILE_TRANSFER_GET_IFACE(protocol);
	if(iface != NULL && iface->receive_async != NULL) {
		iface->receive_async(protocol, transfer, callback, data);
	} else {
		g_task_report_new_error(G_OBJECT(protocol), callback, data,
		                        purple_protocol_file_transfer_receive_async,
		                        PURPLE_PROTOCOL_FILE_TRANSFER_ERROR, 0,
		                        "%s does not implement receive_async",
		                        G_OBJECT_TYPE_NAME(protocol));
	}
}

gboolean
purple_protocol_file_transfer_receive_finish(PurpleProtocolFileTransfer *protocol,
                                             GAsyncResult *result,
                                             GError **error)
{
	PurpleProtocolFileTransferInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_FILE_TRANSFER(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	if(g_async_result_is_tagged(result,
	                            purple_protocol_file_transfer_receive_async))
	{
		return g_task_propagate_boolean(G_TASK(result), error);
	}

	iface = PURPLE_PROTOCOL_FILE_TRANSFER_GET_IFACE(protocol);
	if(iface != NULL && iface->receive_finish != NULL) {
		return iface->receive_finish(protocol, result, error);
	}

	g_warning("purple_protocol_file_transfer_receive_finish called without "
	          "calling purple_protocol_file_transfer_receive_async");

	return FALSE;
}
