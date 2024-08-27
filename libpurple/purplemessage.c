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
#include "purplemessage.h"
#include "purpleprivate.h"
#include "util.h"

struct _PurpleMessage {
	GObject parent;

	char *id;
	PurpleContactInfo *author;

	char *author_name_color;

	char *contents;

	guint action : 1;
	guint event : 1;
	guint notice : 1;
	guint system : 1;

	GDateTime *timestamp;

	GError *error;

	GDateTime *delivered_at;
	GDateTime *edited_at;

	GHashTable *attachments;
};

enum {
	PROP_0,
	PROP_ACTION,
	PROP_AUTHOR,
	PROP_AUTHOR_NAME_COLOR,
	PROP_CONTENTS,
	PROP_DELIVERED,
	PROP_DELIVERED_AT,
	PROP_EDITED,
	PROP_EDITED_AT,
	PROP_ERROR,
	PROP_EVENT,
	PROP_ID,
	PROP_NOTICE,
	PROP_SYSTEM,
	PROP_TIMESTAMP,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_FINAL_TYPE(PurpleMessage, purple_message, G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_message_set_author(PurpleMessage *message, PurpleContactInfo *author) {
	if(g_set_object(&message->author, author)) {
		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_AUTHOR]);
	}
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
	case PROP_ACTION:
		g_value_set_boolean(value, purple_message_get_action(message));
		break;
	case PROP_AUTHOR:
		g_value_set_object(value, purple_message_get_author(message));
		break;
	case PROP_AUTHOR_NAME_COLOR:
		g_value_set_string(value,
		                   purple_message_get_author_name_color(message));
		break;
	case PROP_CONTENTS:
		g_value_set_string(value, purple_message_get_contents(message));
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
	case PROP_ERROR:
		g_value_set_boxed(value, purple_message_get_error(message));
		break;
	case PROP_EVENT:
		g_value_set_boolean(value, purple_message_get_event(message));
		break;
	case PROP_ID:
		g_value_set_string(value, purple_message_get_id(message));
		break;
	case PROP_NOTICE:
		g_value_set_boolean(value, purple_message_get_notice(message));
		break;
	case PROP_SYSTEM:
		g_value_set_boolean(value, purple_message_get_system(message));
		break;
	case PROP_TIMESTAMP:
		g_value_set_boxed(value, purple_message_get_timestamp(message));
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
	case PROP_ACTION:
		purple_message_set_action(message, g_value_get_boolean(value));
		break;
	case PROP_AUTHOR:
		purple_message_set_author(message, g_value_get_object(value));
		break;
	case PROP_AUTHOR_NAME_COLOR:
		purple_message_set_author_name_color(message,
		                                     g_value_get_string(value));
		break;
	case PROP_CONTENTS:
		purple_message_set_contents(message, g_value_get_string(value));
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
	case PROP_ERROR:
		purple_message_set_error(message, g_value_get_boxed(value));
		break;
	case PROP_EVENT:
		purple_message_set_event(message, g_value_get_boolean(value));
		break;
	case PROP_ID:
		purple_message_set_id(message, g_value_get_string(value));
		break;
	case PROP_NOTICE:
		purple_message_set_notice(message, g_value_get_boolean(value));
		break;
	case PROP_SYSTEM:
		purple_message_set_system(message, g_value_get_boolean(value));
		break;
	case PROP_TIMESTAMP:
		purple_message_set_timestamp(message, g_value_get_boxed(value));
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
	g_clear_object(&message->author);
	g_free(message->author_name_color);
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
		"action", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:author:
	 *
	 * The author of the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AUTHOR] = g_param_spec_object(
		"author", NULL, NULL,
		PURPLE_TYPE_CONTACT_INFO,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:author-name-color:
	 *
	 * The hex color for the author's name.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AUTHOR_NAME_COLOR] = g_param_spec_string(
		"author-name-color", NULL, NULL,
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
		"contents", NULL, NULL,
		NULL,
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
		"delivered", NULL, NULL,
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
		"delivered-at", NULL, NULL,
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
		"edited", NULL, NULL,
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
		"edited-at", NULL, NULL,
		G_TYPE_DATE_TIME,
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
		"error", NULL, NULL,
		G_TYPE_ERROR,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:event:
	 *
	 * Whether or not this message is for an event.
	 *
	 * Event messages can include things like a topic changing, a contact
	 * changing their display name, someone joining or leaving a channel, and
	 * so on.
	 *
	 * Since: 3.0
	 */
	properties[PROP_EVENT] = g_param_spec_boolean(
		"event", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:id:
	 *
	 * The protocol-specific identifier of the message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:notice:
	 *
	 * Whether or not this message is a notice.
	 *
	 * Notice typically means that a message should not be auto replied to, but
	 * this can vary across protocols. However, user interfaces may just want
	 * to notice that the message was a notice instead of a normal message.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NOTICE] = g_param_spec_boolean(
		"notice", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleMessage:system:
	 *
	 * Whether or not this is a system message.
	 *
	 * System messages are used to present errors and warnings to the user.
	 *
	 * Since: 3.0
	 */
	properties[PROP_SYSTEM] = g_param_spec_boolean(
		"system", NULL, NULL,
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
		"timestamp", NULL, NULL,
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleMessage *
purple_message_new(PurpleContactInfo *author, const char *contents) {
	g_return_val_if_fail(PURPLE_IS_CONTACT_INFO(author), NULL);
	g_return_val_if_fail(contents != NULL, NULL);

	return g_object_new(
		PURPLE_TYPE_MESSAGE,
		"author", author,
		"contents", contents,
		NULL);
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

PurpleContactInfo *
purple_message_get_author(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->author;
}

void
purple_message_set_author_name_color(PurpleMessage *message,
                                     const char *color)
{
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(g_set_str(&message->author_name_color, color)) {
		g_object_notify_by_pspec(G_OBJECT(message),
		                         properties[PROP_AUTHOR_NAME_COLOR]);
	}
}

const char *
purple_message_get_author_name_color(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->author_name_color;
}

void
purple_message_set_contents(PurpleMessage *message, const char *contents) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(g_set_str(&message->contents, contents)) {
		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_CONTENTS]);
	}
}

const char *
purple_message_get_contents(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->contents;
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

	if(birb_date_time_set(&message->delivered_at, datetime)) {
		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_DELIVERED]);
		g_object_notify_by_pspec(obj, properties[PROP_DELIVERED_AT]);
		g_object_thaw_notify(obj);
	}
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

	if(birb_date_time_set(&message->edited_at, datetime)) {
		g_object_freeze_notify(obj);
		g_object_notify_by_pspec(obj, properties[PROP_EDITED]);
		g_object_notify_by_pspec(obj, properties[PROP_EDITED_AT]);
		g_object_thaw_notify(obj);
	}
}

GError *
purple_message_get_error(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);

	return message->error;
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

gboolean
purple_message_get_event(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return message->event;
}

void
purple_message_set_event(PurpleMessage *message, gboolean event) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(message->event != event) {
		message->event = event;

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_EVENT]);
	}
}

const char *
purple_message_get_id(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), 0);

	return message->id;
}

void
purple_message_set_id(PurpleMessage *message, const char *id) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(g_set_str(&message->id, id)) {
		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_ID]);
	}
}

gboolean
purple_message_is_empty(PurpleMessage *message) {
	return (message->contents == NULL || message->contents[0] == '\0');
}

gboolean
purple_message_get_notice(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return message->notice;
}

void
purple_message_set_notice(PurpleMessage *message, gboolean notice) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(message->notice != notice) {
		message->notice = notice;

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_NOTICE]);
	}
}

gboolean
purple_message_get_system(PurpleMessage *message) {
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	return message->system;
}

void
purple_message_set_system(PurpleMessage *message, gboolean system) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(message->system != system) {
		message->system = system;

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_SYSTEM]);
	}
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

void
purple_message_set_timestamp(PurpleMessage *message, GDateTime *timestamp) {
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	if(birb_date_time_set(&message->timestamp, timestamp)) {
		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_TIMESTAMP]);
	}
}

void
purple_message_set_timestamp_now(PurpleMessage *message) {
	GDateTime *dt = NULL;

	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	dt = g_date_time_new_now_utc();
	purple_message_set_timestamp(message, dt);
	g_date_time_unref(dt);
}

char *
purple_message_format_timestamp(PurpleMessage *message, const char *format) {
	GDateTime *dt = NULL;

	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), NULL);
	g_return_val_if_fail(format != NULL, NULL);

	dt = purple_message_get_timestamp(message);

	return g_date_time_format(dt, format);
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

int
purple_message_compare_timestamp(PurpleMessage *message1,
                                 PurpleMessage *message2)
{
	if(message1 == message2) {
		return 0;
	}

	if(message1 == NULL) {
		return -1;
	}

	if(message2 == NULL) {
		return 1;
	}

	return birb_date_time_compare(message1->timestamp, message2->timestamp);
}
