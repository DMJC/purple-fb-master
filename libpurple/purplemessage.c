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

#include <glib/gi18n-lib.h>

#include "debug.h"
#include "purpleenums.h"
#include "purplemessage.h"
#include "purpleprivate.h"

struct _PurpleMessage {
	GObject parent;

	char *id;
	char *author;
	char *author_name_color;
	char *author_alias;

	char *contents;
	gboolean action;

	GDateTime *timestamp;
	PurpleMessageFlags flags;

	GError *error;

	GDateTime *delivered_at;
	GDateTime *edited_at;

	GHashTable *attachments;
};

enum {
	PROP_0,
	PROP_ID,
	PROP_AUTHOR,
	PROP_AUTHOR_ALIAS,
	PROP_AUTHOR_NAME_COLOR,
	PROP_CONTENTS,
	PROP_ACTION,
	PROP_TIMESTAMP,
	PROP_FLAGS,
	PROP_ERROR,
	PROP_DELIVERED,
	PROP_DELIVERED_AT,
	PROP_EDITED,
	PROP_EDITED_AT,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_FINAL_TYPE(PurpleMessage, purple_message, G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_message_set_author(PurpleMessage *message, const char *author) {
	g_free(message->author);
	message->author = g_strdup(author);

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_AUTHOR]);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_message_get_property(GObject *object, guint param_id, GValue *value,
                            GParamSpec *pspec)
{
	PurpleMessage *message = PURPLE_MESSAGE(object);

	switch(param_id) {
		case PROP_ID:
			g_value_set_string(value, purple_message_get_id(message));
			break;
		case PROP_AUTHOR:
			g_value_set_string(value, purple_message_get_author(message));
			break;
		case PROP_AUTHOR_ALIAS:
			g_value_set_string(value, purple_message_get_author_alias(message));
			break;
		case PROP_AUTHOR_NAME_COLOR:
			g_value_set_string(value,
			                   purple_message_get_author_name_color(message));
			break;
		case PROP_CONTENTS:
			g_value_set_string(value, purple_message_get_contents(message));
			break;
		case PROP_ACTION:
			g_value_set_boolean(value, purple_message_get_action(message));
			break;
		case PROP_TIMESTAMP:
			g_value_set_boxed(value, purple_message_get_timestamp(message));
			break;
		case PROP_FLAGS:
			g_value_set_flags(value, purple_message_get_flags(message));
			break;
		case PROP_ERROR:
			g_value_set_boxed(value, purple_message_get_error(message));
			break;
		case PROP_DELIVERED:
			g_value_set_boolean(value, purple_message_get_delivered(message));
			break;
		case PROP_DELIVERED_AT:
			g_value_set_boxed(value, purple_message_get_delivered_at(message));
			break;
		case PROP_EDITED:
			g_value_set_boolean(value, purple_message_get_edited(message));
			break;
		case PROP_EDITED_AT:
			g_value_set_boxed(value, purple_message_get_edited_at(message));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
			break;
	}
}

static void
purple_message_set_property(GObject *object, guint param_id,
                            const GValue *value, GParamSpec *pspec)
{
	PurpleMessage *message = PURPLE_MESSAGE(object);

	switch(param_id) {
		case PROP_ID:
			purple_message_set_id(message, g_value_get_string(value));
			break;
		case PROP_AUTHOR:
			purple_message_set_author(message, g_value_get_string(value));
			break;
		case PROP_AUTHOR_ALIAS:
			purple_message_set_author_alias(message, g_value_get_string(value));
			break;
		case PROP_AUTHOR_NAME_COLOR:
			purple_message_set_author_name_color(message,
			                                     g_value_get_string(value));
			break;
		case PROP_CONTENTS:
			purple_message_set_contents(message, g_value_get_string(value));
			break;
		case PROP_ACTION:
			purple_message_set_action(message, g_value_get_boolean(value));
			break;
		case PROP_TIMESTAMP:
			purple_message_set_timestamp(message, g_value_get_boxed(value));
			break;
		case PROP_FLAGS:
			purple_message_set_flags(message, g_value_get_flags(value));
			break;
		case PROP_ERROR:
			purple_message_set_error(message, g_value_get_boxed(value));
			break;
		case PROP_DELIVERED:
			purple_message_set_delivered(message, g_value_get_boolean(value));
			break;
		case PROP_DELIVERED_AT:
			purple_message_set_delivered_at(message, g_value_get_boxed(value));
			break;
		case PROP_EDITED:
			purple_message_set_edited(message, g_value_get_boolean(value));
			break;
		case PROP_EDITED_AT:
			purple_message_set_edited_at(message, g_value_get_boxed(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
			break;
	}
}

static void
purple_message_finalize(GObject *obj) {
	PurpleMessage *message = PURPLE_MESSAGE(obj);

	g_free(message->id);
	g_free(message->author);
	g_free(message->author_name_color);
	g_free(message->author_alias);
	g_free(message->contents);

	g_clear_error(&message->error);

	g_clear_pointer(&message->timestamp, g_date_time_unref);
	g_clear_pointer(&message->delivered_at, g_date_time_unref);
	g_clear_pointer(&message->edited_at, g_date_time_unref);

	g_hash_table_destroy(message->attachments);

	G_OBJECT_CLASS(purple_message_parent_class)->finalize(obj);
}

static void
purple_message_init(PurpleMessage *message) {
	message->attachments = g_hash_table_new_full(g_int64_hash, g_int64_equal,
	                                             NULL, g_object_unref);
}

static void
purple_message_class_init(PurpleMessageClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_message_get_property;
	obj_class->set_property = purple_message_set_property;
	obj_class->finalize = purple_message_finalize;

	/**
	 * PurpleMessage:id:
	 *
	 * The protocol-specific identifier of the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", "ID",
		"The protocol specific message id",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:author:
	 *
	 * The author of the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AUTHOR] = g_param_spec_string(
		"author", "Author",
		"The username of the person, who sent the message.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:author-name-color:
	 *
	 * The hex color for the author's name.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AUTHOR_NAME_COLOR] = g_param_spec_string(
		"author-name-color", "author-name-color",
		"The hex color to display the author's name with",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:author-alias:
	 *
	 * The alias of the author.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AUTHOR_ALIAS] = g_param_spec_string(
		"author-alias", "Author's alias",
		"The alias of the sender",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:contents:
	 *
	 * The contents of the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CONTENTS] = g_param_spec_string(
		"contents", "Contents",
		"The message text",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:action:
	 *
	 * Whether or not the message is an action.
	 *
	 * Typically a message is considered an action when the body starts with
	 * `/me`. Some protocols handle this differently but this is mostly a user
	 * interface hint that this message is different than a normal text
	 * message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACTION] = g_param_spec_boolean(
		"action", "action",
		"Whether or not the message is an action.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:timestamp:
	 *
	 * The timestamp of the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TIMESTAMP] = g_param_spec_boxed(
		"timestamp", "timestamp",
		"The timestamp of the message",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:flags:
	 *
	 * The #PurpleMessageFlags for the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_FLAGS] = g_param_spec_flags(
		"flags", "Flags",
		"Bitwise set of #PurpleMessageFlags flags",
		PURPLE_TYPE_MESSAGE_FLAGS, 0,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:error:
	 *
	 * An error that this message encountered. This could be something like a
	 * failed delivery, or failed redaction, or rate limited, etc.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ERROR] = g_param_spec_boxed(
		"error", "error",
		"An error that the message encountered",
		G_TYPE_ERROR,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:delivered:
	 *
	 * Whether or not the message was delivered. This is protocol dependent and
	 * possibly client dependent as well. So if this is %FALSE that doesn't
	 * necessarily mean the message was not delivered.
	 *
	 * Since: 3.0
	 */
	properties[PROP_DELIVERED] = g_param_spec_boolean(
		"delivered", "delivered",
		"Whether or not the message was delivered.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:delivered-at:
	 *
	 * The time that the message was delivered. This is protocol dependent and
	 * possibly client dependent as well. So if this is %NULL that doesn't
	 * necessarily mean the message was not delivered.
	 *
	 * Since: 3.0
	 */
	properties[PROP_DELIVERED_AT] = g_param_spec_boxed(
		"delivered-at", "delivered-at",
		"The time that the message was delivered.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:edited:
	 *
	 * Whether or not this message has been edited.
	 *
	 * This should typically only be set by a protocol plugin.
	 *
	 * Since: 3.0
	 */
	properties[PROP_EDITED] = g_param_spec_boolean(
		"edited", "edited",
		"Whether or not this message has been edited.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:edited-at:
	 *
	 * The time that the message was last edited at. This is protocol dependent
	 * and possibly client dependent as well. So if this is %NULL that doesn't
	 * necessarily mean the message was not edited.
	 *
	 * Since: 3.0
	 */
	properties[PROP_EDITED_AT] = g_param_spec_boxed(
		"edited-at", "edited-at",
		"The time that the message was last edited.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleMessage *
purple_message_new_outgoing(const char *author, const char *contents,
                            PurpleMessageFlags flags)
{
	PurpleMessage *message = NULL;
	GDateTime *dt = NULL;

	g_warn_if_fail(!(flags & PURPLE_MESSAGE_RECV));
	g_warn_if_fail(!(flags & PURPLE_MESSAGE_SYSTEM));

	flags |= PURPLE_MESSAGE_SEND;
	dt = g_date_time_new_now_local();

	/* who may be NULL for outgoing MUC messages */
	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", author,
		"contents", contents,
		"timestamp", dt,
		"flags", flags,
		NULL);

	g_date_time_unref(dt);

	return message;
}

PurpleMessage *
purple_message_new_incoming(const char *who, const char *contents,
                            PurpleMessageFlags flags, guint64 timestamp)
{
	PurpleMessage *message = NULL;
	GDateTime *dt = NULL;

	g_warn_if_fail(!(flags & PURPLE_MESSAGE_SEND));
	g_warn_if_fail(!(flags & PURPLE_MESSAGE_SYSTEM));

	flags |= PURPLE_MESSAGE_RECV;

	if(timestamp == 0) {
		dt = g_date_time_new_now_local();
	} else {
		dt = g_date_time_new_from_unix_local((gint64)timestamp);
	}

	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", who,
		"author-alias", who,
		"contents", contents,
		"timestamp", dt,
		"flags", flags,
		NULL);

	g_date_time_unref(dt);

	return message;
}

PurpleMessage *
purple_message_new_system(const char *contents, PurpleMessageFlags flags) {
	PurpleMessage *message = NULL;
	GDateTime *dt = NULL;

	g_warn_if_fail(!(flags & PURPLE_MESSAGE_SEND));
	g_warn_if_fail(!(flags & PURPLE_MESSAGE_RECV));

	flags |= PURPLE_MESSAGE_SYSTEM;
	dt = g_date_time_new_now_local();

	message = g_object_new(
		PURPLE_TYPE_MESSAGE,
		"contents", contents,
		"timestamp", dt,
		"flags", flags,
		NULL);

	g_date_time_unref(dt);

	return message;
}

const char *
purple_message_get_id(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), 0);

	return message->id;
}

void
purple_message_set_id(PurpleMessage *message, const char *id) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_free(message->id);
	message->id = g_strdup(id);

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_ID]);
}

const char *
purple_message_get_author(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->author;
}

void
purple_message_set_author_name_color(PurpleMessage *message,
                                     const char *color)
{
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_free(message->author_name_color);
	message->author_name_color = g_strdup(color);

	g_object_notify_by_pspec(G_OBJECT(message),
	                         properties[PROP_AUTHOR_NAME_COLOR]);
}

const char *
purple_message_get_author_name_color(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->author_name_color;
}

void
purple_message_set_author_alias(PurpleMessage *message,
                                const char *author_alias)
{
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_free(message->author_alias);
	message->author_alias = g_strdup(author_alias);

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_AUTHOR_ALIAS]);
}

const char *
purple_message_get_author_alias(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	if (message->author_alias == NULL)
		return purple_message_get_author(message);

	return message->author_alias;
}

void
purple_message_set_contents(PurpleMessage *message, const char *contents) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_free(message->contents);
	message->contents = g_strdup(contents);

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_CONTENTS]);
}

const char *
purple_message_get_contents(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->contents;
}

gboolean
purple_message_is_empty(PurpleMessage *message) {
	return (message->contents == NULL || message->contents[0] == '\0');
}

void
purple_message_set_timestamp(PurpleMessage *message, GDateTime *timestamp) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_clear_pointer(&message->timestamp, g_date_time_unref);
	if(timestamp != NULL) {
		message->timestamp = g_date_time_ref(timestamp);
	}

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_TIMESTAMP]);
}

GDateTime *
purple_message_get_timestamp(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), 0);

	if(message->timestamp == NULL) {
		GDateTime *dt = g_date_time_new_now_local();
		purple_message_set_timestamp(message, dt);
		g_date_time_unref(dt);
	}

	return message->timestamp;
}

char *
purple_message_format_timestamp(PurpleMessage *message, const char *format) {
	GDateTime *dt = NULL;

	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);
	g_return_val_if_fail(format != NULL, NULL);

	dt = purple_message_get_timestamp(message);

	return g_date_time_format(dt, format);
}

void
purple_message_set_flags(PurpleMessage *message, PurpleMessageFlags flags) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	message->flags = flags;

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_FLAGS]);
}

PurpleMessageFlags
purple_message_get_flags(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), 0);

	return message->flags;
}

void
purple_message_set_error(PurpleMessage *message, GError *error) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_clear_error(&message->error);
	if(error != NULL) {
		message->error = g_error_copy(error);
	}

	g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_ERROR]);
}

GError *
purple_message_get_error(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->error;
}

gboolean
purple_message_add_attachment(PurpleMessage *message,
                              PurpleAttachment *attachment)
{
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);
	g_return_val_if_fail(PURPLE_IS_ATTACHMENT(attachment), FALSE);

	return g_hash_table_insert(message->attachments,
	                           purple_attachment_get_hash_key(attachment),
	                           g_object_ref(attachment));
}

gboolean
purple_message_remove_attachment(PurpleMessage *message, guint64 id) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return g_hash_table_remove(message->attachments, &id);
}

PurpleAttachment *
purple_message_get_attachment(PurpleMessage *message, guint64 id) {
	PurpleAttachment *attachment = NULL;

	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	attachment = g_hash_table_lookup(message->attachments, &id);
	if(PURPLE_IS_ATTACHMENT(attachment)) {
		return g_object_ref(attachment);
	}

	return NULL;
}

void
purple_message_foreach_attachment(PurpleMessage *message,
                                  PurpleAttachmentForeachFunc func,
                                  gpointer data)
{
	GHashTableIter iter;
	gpointer value;

	g_return_if_fail(PURPLE_IS_MESSAGE(message));
	g_return_if_fail(func != NULL);

	g_hash_table_iter_init(&iter, message->attachments);
	while(g_hash_table_iter_next(&iter, NULL, &value)) {
		func(PURPLE_ATTACHMENT(value), data);
	}
}

void
purple_message_clear_attachments(PurpleMessage *message) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	g_hash_table_remove_all(message->attachments);
}

gboolean
purple_message_get_action(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return message->action;
}

void
purple_message_set_action(PurpleMessage *message, gboolean action) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(action != message->action) {
		message->action = action;

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_ACTION]);
	}
}

gboolean
purple_message_get_delivered(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return (message->delivered_at != NULL);
}

void
purple_message_set_delivered(PurpleMessage *message, gboolean delivered) {
	GDateTime *datetime = NULL;

	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(delivered) {
		datetime = g_date_time_new_now_utc();
	}

	purple_message_set_delivered_at(message, datetime);
	g_clear_pointer(&datetime, g_date_time_unref);
}

GDateTime *
purple_message_get_delivered_at(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->delivered_at;
}

void
purple_message_set_delivered_at(PurpleMessage *message, GDateTime *datetime) {
	GObject *obj = NULL;

	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	obj = G_OBJECT(message);

	if(datetime == NULL) {
		if(message->delivered_at == NULL) {
			return;
		}

		g_clear_pointer(&message->delivered_at, g_date_time_unref);
	} else {
		if(message->delivered_at != NULL) {
			if(g_date_time_equal(message->delivered_at, datetime)) {
				return;
			}

			g_clear_pointer(&message->delivered_at, g_date_time_unref);
			message->delivered_at = g_date_time_ref(datetime);
		} else {
			message->delivered_at = g_date_time_ref(datetime);
		}
	}

	g_object_freeze_notify(obj);
	g_object_notify_by_pspec(obj, properties[PROP_DELIVERED]);
	g_object_notify_by_pspec(obj, properties[PROP_DELIVERED_AT]);
	g_object_thaw_notify(obj);
}

gboolean
purple_message_get_edited(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return (message->edited_at != NULL);
}

void
purple_message_set_edited(PurpleMessage *message, gboolean edited) {
	GDateTime *datetime = NULL;

	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(edited) {
		datetime = g_date_time_new_now_utc();
	}

	purple_message_set_edited_at(message, datetime);
	g_clear_pointer(&datetime, g_date_time_unref);
}

GDateTime *
purple_message_get_edited_at(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->edited_at;
}

void
purple_message_set_edited_at(PurpleMessage *message, GDateTime *datetime) {
	GObject *obj = NULL;

	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	obj = G_OBJECT(message);

	if(datetime == NULL) {
		if(message->edited_at == NULL) {
			return;
		}

		g_clear_pointer(&message->edited_at, g_date_time_unref);
	} else {
		if(message->edited_at != NULL) {
			if(g_date_time_equal(message->edited_at, datetime)) {
				return;
			}

			g_clear_pointer(&message->edited_at, g_date_time_unref);
			message->edited_at = g_date_time_ref(datetime);
		} else {
			message->edited_at = g_date_time_ref(datetime);
		}
	}

	g_object_freeze_notify(obj);
	g_object_notify_by_pspec(obj, properties[PROP_EDITED]);
	g_object_notify_by_pspec(obj, properties[PROP_EDITED_AT]);
	g_object_thaw_notify(obj);
}
