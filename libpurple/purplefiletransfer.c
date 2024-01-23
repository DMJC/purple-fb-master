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

#include "purpleaccount.h"
#include "purplecontactinfo.h"
#include "purpleenums.h"
#include "purplefiletransfer.h"
#include "util.h"

enum {
	PROP_0,
	PROP_ACCOUNT,
	PROP_REMOTE,
	PROP_INITIATOR,
	PROP_CANCELLABLE,
	PROP_STATE,
	PROP_ERROR,
	PROP_LOCAL_FILE,
	PROP_FILENAME,
	PROP_FILE_SIZE,
	PROP_CONTENT_TYPE,
	PROP_MESSAGE,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleFileTransfer {
	GObject parent;

	PurpleAccount *account;
	PurpleContactInfo *remote;
	PurpleContactInfo *initiator;

	GCancellable *cancellable;

	PurpleFileTransferState state;
	GError *error;

	GFile *local_file;
	char *filename;
	guint64 file_size;
	char *content_type;

	char *message;
};

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_file_transfer_set_account(PurpleFileTransfer *transfer,
                                 PurpleAccount *account)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));

	if(g_set_object(&transfer->account, account)) {
		g_object_notify_by_pspec(G_OBJECT(transfer), properties[PROP_ACCOUNT]);
	}
}

static void
purple_file_transfer_set_remote(PurpleFileTransfer *transfer,
                                PurpleContactInfo *remote)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));
	g_return_if_fail(PURPLE_IS_CONTACT_INFO(remote));

	if(g_set_object(&transfer->remote, remote)) {
		g_object_notify_by_pspec(G_OBJECT(transfer), properties[PROP_REMOTE]);
	}
}

static void
purple_file_transfer_set_initiator(PurpleFileTransfer *transfer,
                                   PurpleContactInfo *initiator)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));
	g_return_if_fail(PURPLE_IS_CONTACT_INFO(initiator));

	if(g_set_object(&transfer->initiator, initiator)) {
		g_object_notify_by_pspec(G_OBJECT(transfer),
		                         properties[PROP_INITIATOR]);
	}
}

static void
purple_file_transfer_set_filename(PurpleFileTransfer *transfer,
                                  const char *filename)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));
	g_return_if_fail(!purple_strempty(filename));

	if(!purple_strequal(transfer->filename, filename)) {
		g_free(transfer->filename);
		transfer->filename = g_strdup(filename);

		g_object_notify_by_pspec(G_OBJECT(transfer),
		                         properties[PROP_FILENAME]);
	}
}

static void
purple_file_transfer_set_file_size(PurpleFileTransfer *transfer, guint64 size)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));

	if(transfer->file_size != size) {
		transfer->file_size = size;

		g_object_notify_by_pspec(G_OBJECT(transfer),
		                         properties[PROP_FILE_SIZE]);
	}
}

static char *
purple_file_transfer_get_local_file_name_and_size(GFile *local_file,
                                                  guint64 *file_size)
{
	GError *error = NULL;
	GFileInfo *info = NULL;
	const char *display_name = NULL;
	char *ret = NULL;

	info = g_file_query_info(local_file,
	                         G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
	                         G_FILE_ATTRIBUTE_STANDARD_SIZE,
	                         G_FILE_QUERY_INFO_NONE,
	                         NULL,
	                         &error);

	if(error != NULL) {
		char *path = g_file_get_path(local_file);

		g_warning("failed to query %s: %s", path, error->message);

		g_clear_pointer(&path, g_free);
		g_clear_error(&error);
		g_clear_object(&info);

		return NULL;
	}

	display_name = g_file_info_get_attribute_string(info,
	                                                G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
	/* display_name is only valid as long as the GFileInfo object is around,
	 * but it is freed before this function returns, so we need to dupe the
	 * display name attribute to return it.
	 */
	ret = g_strdup(display_name);

	if(file_size != NULL) {
		*file_size = g_file_info_get_attribute_uint64(info,
		                                              G_FILE_ATTRIBUTE_STANDARD_SIZE);
	}

	g_clear_object(&info);

	return ret;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleFileTransfer, purple_file_transfer, G_TYPE_OBJECT)

static void
purple_file_transfer_finalize(GObject *obj) {
	PurpleFileTransfer *transfer = PURPLE_FILE_TRANSFER(obj);

	g_clear_object(&transfer->account);
	g_clear_object(&transfer->remote);
	g_clear_object(&transfer->initiator);

	g_clear_object(&transfer->cancellable);
	g_clear_error(&transfer->error);

	g_clear_object(&transfer->local_file);
	g_clear_pointer(&transfer->filename, g_free);
	g_clear_pointer(&transfer->content_type, g_free);

	g_clear_pointer(&transfer->message, g_free);

	G_OBJECT_CLASS(purple_file_transfer_parent_class)->finalize(obj);
}

static void
purple_file_transfer_get_property(GObject *obj, guint param_id, GValue *value,
                                  GParamSpec *pspec)
{
	PurpleFileTransfer *transfer = PURPLE_FILE_TRANSFER(obj);

	switch(param_id) {
	case PROP_ACCOUNT:
		g_value_set_object(value, purple_file_transfer_get_account(transfer));
		break;
	case PROP_REMOTE:
		g_value_set_object(value, purple_file_transfer_get_remote(transfer));
		break;
	case PROP_INITIATOR:
		g_value_set_object(value,
		                   purple_file_transfer_get_initiator(transfer));
		break;
	case PROP_CANCELLABLE:
		g_value_set_object(value,
		                   purple_file_transfer_get_cancellable(transfer));
		break;
	case PROP_STATE:
		g_value_set_enum(value, purple_file_transfer_get_state(transfer));
		break;
	case PROP_ERROR:
		g_value_set_boxed(value, purple_file_transfer_get_error(transfer));
		break;
	case PROP_LOCAL_FILE:
		g_value_set_object(value,
		                   purple_file_transfer_get_local_file(transfer));
		break;
	case PROP_FILENAME:
		g_value_set_string(value,
		                   purple_file_transfer_get_filename(transfer));
		break;
	case PROP_FILE_SIZE:
		g_value_set_uint64(value,
		                   purple_file_transfer_get_file_size(transfer));
		break;
	case PROP_CONTENT_TYPE:
		g_value_set_string(value,
		                   purple_file_transfer_get_content_type(transfer));
		break;
	case PROP_MESSAGE:
		g_value_set_string(value, purple_file_transfer_get_message(transfer));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_file_transfer_set_property(GObject *obj, guint param_id,
                                  const GValue *value, GParamSpec *pspec)
{
	PurpleFileTransfer *transfer = PURPLE_FILE_TRANSFER(obj);

	switch(param_id) {
	case PROP_ACCOUNT:
		purple_file_transfer_set_account(transfer, g_value_get_object(value));
		break;
	case PROP_REMOTE:
		purple_file_transfer_set_remote(transfer, g_value_get_object(value));
		break;
	case PROP_INITIATOR:
		purple_file_transfer_set_initiator(transfer,
		                                   g_value_get_object(value));
		break;
	case PROP_STATE:
		purple_file_transfer_set_state(transfer, g_value_get_enum(value));
		break;
	case PROP_ERROR:
		purple_file_transfer_set_error(transfer, g_value_get_boxed(value));
		break;
	case PROP_LOCAL_FILE:
		purple_file_transfer_set_local_file(transfer,
		                                    g_value_get_object(value));
		break;
	case PROP_FILENAME:
		purple_file_transfer_set_filename(transfer,
		                                  g_value_get_string(value));
		break;
	case PROP_FILE_SIZE:
		purple_file_transfer_set_file_size(transfer,
		                                   g_value_get_uint64(value));
		break;
	case PROP_CONTENT_TYPE:
		purple_file_transfer_set_content_type(transfer,
		                                      g_value_get_string(value));
		break;
	case PROP_MESSAGE:
		purple_file_transfer_set_message(transfer, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_file_transfer_init(PurpleFileTransfer *transfer) {
	transfer->cancellable = g_cancellable_new();
}

static void
purple_file_transfer_class_init(PurpleFileTransferClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_file_transfer_finalize;
	obj_class->get_property = purple_file_transfer_get_property;
	obj_class->set_property = purple_file_transfer_set_property;

	/**
	 * PurpleFileTransfer:account:
	 *
	 * The account that this file transfer is for.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "account",
		"The account that this file transfer is for.",
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:remote:
	 *
	 * The [class@ContactInfo] for the remote user of this file transfer.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_REMOTE] = g_param_spec_object(
		"remote", "remote",
		"The contact info for the remote user.",
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:initiator:
	 *
	 * The [class@ContactInfo] that initiated this file transfer.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_INITIATOR] = g_param_spec_object(
		"initiator", "initiator",
		"The contact info of the user that initiated the file transfer.",
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:cancellable:
	 *
	 * The [class@Gio.Cancellable] for this transfer. It may be used to cancel
	 * the file transfer at any time.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_CANCELLABLE] = g_param_spec_object(
		"cancellable", "cancellable",
		"The cancellable for this transfer.",
		G_TYPE_CANCELLABLE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:state:
	 *
	 * The state of the transfer.
	 *
	 * This is typically only set by the protocol plugin that is performing the
	 * file transfer.
	 *
	 * If the state is set to error then [property@FileTransfer:error]
	 * should be set.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_STATE] = g_param_spec_enum(
		"state", "state",
		"The state of the transfer.",
		PURPLE_TYPE_FILE_TRANSFER_STATE,
		PURPLE_FILE_TRANSFER_STATE_UNKNOWN,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:error:
	 *
	 * A #GError representing the failure of the transfer. This should only be
	 * set if [property@FileTransfer:state] is set to error.
	 *
	 * This should be used to tell the user about network issues or if the
	 * transfer was cancelled and so on.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ERROR] = g_param_spec_boxed(
		"error", "error",
		"The error that caused the transfer to fail.",
		G_TYPE_ERROR,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:local-file:
	 *
	 * The local file that is being sent or received.
	 *
	 * When sending a file, this is the file that's being sent. When receiving
	 * a file, this is the file where the transfer is being written.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_LOCAL_FILE] = g_param_spec_object(
		"local-file", "local-file",
		"The local file for the transfer.",
		G_TYPE_FILE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:filename:
	 *
	 * The base filename for the transfer. This is used as the default filename
	 * for the receiving side.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_FILENAME] = g_param_spec_string(
		"filename", "filename",
		"The base filename for the transfer.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:file-size:
	 *
	 * The size of the file in bytes. A value of %0 typically means the size is
	 * unknown, but it is possible to transfer empty files as well.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_FILE_SIZE] = g_param_spec_uint64(
		"file-size", "file-size",
		"The size of the file in bytes.",
		0, G_MAXUINT64, 0,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:content-type:
	 *
	 * The content or media type of the file that's being transferred. This is
	 * meant to be used as a hint to user interfaces so they can provide
	 * previews or appropriate actions for the files.
	 *
	 * See the [Media Types page](https://www.iana.org/assignments/media-types/media-types.xhtml)
	 * for more information.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_CONTENT_TYPE] = g_param_spec_string(
		"content-type", "content-type",
		"The content type of the file being transferred.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleFileTransfer:message:
	 *
	 * Some protocols support sending a message with the file transfer. This
	 * field is to hold that message.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_MESSAGE] = g_param_spec_string(
		"message", "message",
		"The message to send with the transfer.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleFileTransfer *
purple_file_transfer_new_send(PurpleAccount *account,
                              PurpleContactInfo *remote,
                              GFile *local_file)
{
	PurpleFileTransfer *transfer = NULL;
	gchar *filename = NULL;
	guint64 file_size = 0;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(remote), NULL);

	filename = purple_file_transfer_get_local_file_name_and_size(local_file,
	                                                             &file_size);

	transfer = g_object_new(
		PURPLE_TYPE_FILE_TRANSFER,
		"account", account,
		"remote", remote,
		"initiator", PURPLE_CONTACT_INFO(account),
		"local-file", local_file,
		"filename", filename,
		"file-size", file_size,
		NULL);

	g_clear_pointer(&filename, g_free);

	return transfer;
}

PurpleFileTransfer *
purple_file_transfer_new_receive(PurpleAccount *account,
                                 PurpleContactInfo *remote,
                                 const char *filename,
                                 guint64 file_size)
{
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(remote), NULL);
	g_return_val_if_fail(!purple_strempty(filename), NULL);

	return g_object_new(
		PURPLE_TYPE_FILE_TRANSFER,
		"account", account,
		"remote", remote,
		"initiator", remote,
		"filename", filename,
		"file-size", file_size,
		NULL);
}

PurpleAccount *
purple_file_transfer_get_account(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->account;
}

PurpleContactInfo *
purple_file_transfer_get_remote(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->remote;
}

PurpleContactInfo *
purple_file_transfer_get_initiator(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->initiator;
}

GCancellable *
purple_file_transfer_get_cancellable(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->cancellable;
}

PurpleFileTransferState
purple_file_transfer_get_state(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer),
	                     PURPLE_FILE_TRANSFER_STATE_UNKNOWN);

	return transfer->state;
}

void
purple_file_transfer_set_state(PurpleFileTransfer *transfer,
                               PurpleFileTransferState state)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));

	if(transfer->state != state) {
		transfer->state = state;

		g_object_notify_by_pspec(G_OBJECT(transfer), properties[PROP_STATE]);
	}
}

GError *
purple_file_transfer_get_error(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->error;
}

void
purple_file_transfer_set_error(PurpleFileTransfer *transfer, GError *error) {
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));

	if(transfer->error != error) {
		g_clear_error(&transfer->error);
		transfer->error = error;

		g_object_notify_by_pspec(G_OBJECT(transfer), properties[PROP_ERROR]);
	}
}

GFile *
purple_file_transfer_get_local_file(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->local_file;
}

void
purple_file_transfer_set_local_file(PurpleFileTransfer *transfer,
                                    GFile *local_file)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));
	g_return_if_fail(G_IS_FILE(local_file));

	if(g_set_object(&transfer->local_file, local_file)) {
		g_object_notify_by_pspec(G_OBJECT(transfer),
		                         properties[PROP_LOCAL_FILE]);
	}
}

const char *
purple_file_transfer_get_filename(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->filename;
}

guint64
purple_file_transfer_get_file_size(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), 0);

	return transfer->file_size;
}

const char *
purple_file_transfer_get_content_type(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->content_type;
}

void
purple_file_transfer_set_content_type(PurpleFileTransfer *transfer,
                                      const char *content_type)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));

	if(!purple_strequal(transfer->content_type, content_type)) {
		g_free(transfer->content_type);
		transfer->content_type = g_strdup(content_type);

		g_object_notify_by_pspec(G_OBJECT(transfer),
		                         properties[PROP_CONTENT_TYPE]);
	}
}

const char *
purple_file_transfer_get_message(PurpleFileTransfer *transfer) {
	g_return_val_if_fail(PURPLE_IS_FILE_TRANSFER(transfer), NULL);

	return transfer->message;
}

void
purple_file_transfer_set_message(PurpleFileTransfer *transfer,
                                 const char *message)
{
	g_return_if_fail(PURPLE_IS_FILE_TRANSFER(transfer));

	if(!purple_strequal(transfer->message, message)) {
		g_free(transfer->message);
		transfer->message = g_strdup(message);

		g_object_notify_by_pspec(G_OBJECT(transfer), properties[PROP_MESSAGE]);
	}
}
