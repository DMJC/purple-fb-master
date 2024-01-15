/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpleircv3message.h"

enum {
	PROP_0,
	PROP_COMMAND,
	PROP_SOURCE,
	PROP_PARAMS,
	PROP_TAGS,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

struct _PurpleIRCv3Message {
	GObject parent;

	char *command;
	GStrv params;
	char *source;
	GHashTable *tags;
};

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE(PurpleIRCv3Message, purple_ircv3_message, G_TYPE_OBJECT)

static void
purple_ircv3_message_finalize(GObject *obj) {
	PurpleIRCv3Message *message = PURPLE_IRCV3_MESSAGE(obj);

	g_clear_pointer(&message->command, g_free);
	g_clear_pointer(&message->params, g_strfreev);
	g_clear_pointer(&message->source, g_free);
	g_clear_pointer(&message->tags, g_hash_table_unref);

	G_OBJECT_CLASS(purple_ircv3_message_parent_class)->finalize(obj);
}

static void
purple_ircv3_message_get_property(GObject *obj, guint param_id, GValue *value,
                                  GParamSpec *pspec)
{
	PurpleIRCv3Message *message = PURPLE_IRCV3_MESSAGE(obj);

	switch(param_id) {
	case PROP_COMMAND:
		g_value_set_string(value, purple_ircv3_message_get_command(message));
		break;
	case PROP_PARAMS:
		g_value_set_boxed(value, purple_ircv3_message_get_params(message));
		break;
	case PROP_SOURCE:
		g_value_set_string(value, purple_ircv3_message_get_source(message));
		break;
	case PROP_TAGS:
		g_value_set_boxed(value, purple_ircv3_message_get_tags(message));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_ircv3_message_set_property(GObject *obj, guint param_id,
                                  const GValue *value, GParamSpec *pspec)
{
	PurpleIRCv3Message *message = PURPLE_IRCV3_MESSAGE(obj);

	switch(param_id) {
	case PROP_COMMAND:
		purple_ircv3_message_set_command(message, g_value_get_string(value));
		break;
	case PROP_PARAMS:
		purple_ircv3_message_set_params(message, g_value_get_boxed(value));
		break;
	case PROP_SOURCE:
		purple_ircv3_message_set_source(message, g_value_get_string(value));
		break;
	case PROP_TAGS:
		purple_ircv3_message_set_tags(message, g_value_get_boxed(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_ircv3_message_init(G_GNUC_UNUSED PurpleIRCv3Message *message) {
}

static void
purple_ircv3_message_class_init(PurpleIRCv3MessageClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_ircv3_message_finalize;
	obj_class->get_property = purple_ircv3_message_get_property;
	obj_class->set_property = purple_ircv3_message_set_property;

	/**
	 * PurpleIRCv3Message:command:
	 *
	 * The command of this message.
	 *
	 * This could be something like JOIN or a server reply numeric like 005.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_COMMAND] = g_param_spec_string(
		"command", "command",
		"The command of the message.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleIRCv3Message:params:
	 *
	 * The parameters of the message.
	 *
	 * When serialized, the last item will be prefixed with a :.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PARAMS] = g_param_spec_boxed(
		"params", "params",
		"The parameters of this message.",
		G_TYPE_STRV,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleIRCv3Message:source:
	 *
	 * The source of the message.
	 *
	 * This could be a nickname, a full nick!ident@server, a server name, or
	 * %NULL.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_SOURCE] = g_param_spec_string(
		"source", "source",
		"The source of the message.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleIRCv3Message:tags:
	 *
	 * The [ircv3 message tags](https://ircv3.net/specs/extensions/message-tags)
	 * for the message.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TAGS] = g_param_spec_boxed(
		"tags", "tags",
		"The ircv3 message tags from the message.",
		G_TYPE_HASH_TABLE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleIRCv3Message *
purple_ircv3_message_new(const char *command) {
	return g_object_new(
		PURPLE_IRCV3_TYPE_MESSAGE,
		"command", command,
		NULL);
}

const char *
purple_ircv3_message_get_command(PurpleIRCv3Message *message) {
	g_return_val_if_fail(PURPLE_IRCV3_IS_MESSAGE(message), NULL);

	return message->command;
}

void
purple_ircv3_message_set_command(PurpleIRCv3Message *message,
                                 const char *command)
{
	g_return_if_fail(PURPLE_IRCV3_IS_MESSAGE(message));
	g_return_if_fail(!purple_strempty(command));

	if(!purple_strequal(message->command, command)) {
		g_free(message->command);
		message->command = g_strdup(command);

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_COMMAND]);
	}
}

const char *
purple_ircv3_message_get_source(PurpleIRCv3Message *message) {
	g_return_val_if_fail(PURPLE_IRCV3_IS_MESSAGE(message), NULL);

	return message->source;
}

void
purple_ircv3_message_set_source(PurpleIRCv3Message *message,
                                const char *source)
{
	g_return_if_fail(PURPLE_IRCV3_IS_MESSAGE(message));

	if(!purple_strequal(message->source, source)) {
		g_free(message->source);
		message->source = g_strdup(source);

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_SOURCE]);
	}
}

GHashTable *
purple_ircv3_message_get_tags(PurpleIRCv3Message *message) {
	g_return_val_if_fail(PURPLE_IRCV3_IS_MESSAGE(message), NULL);

	return message->tags;
}

void
purple_ircv3_message_set_tags(PurpleIRCv3Message *message, GHashTable *tags) {
	g_return_if_fail(PURPLE_IRCV3_IS_MESSAGE(message));

	if(message->tags != tags) {
		g_clear_pointer(&message->tags, g_hash_table_unref);

		if(tags != NULL) {
			message->tags = g_hash_table_ref(tags);
		}

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_TAGS]);
	}
}

GStrv
purple_ircv3_message_get_params(PurpleIRCv3Message *message) {
	g_return_val_if_fail(PURPLE_IRCV3_IS_MESSAGE(message), NULL);

	return message->params;
}

void
purple_ircv3_message_set_params(PurpleIRCv3Message *message, GStrv params) {
	g_return_if_fail(PURPLE_IRCV3_IS_MESSAGE(message));

	if(message->params != params) {
		g_clear_pointer(&message->params, g_strfreev);

		if(params != NULL) {
			message->params = g_strdupv(params);
		}

		g_object_notify_by_pspec(G_OBJECT(message), properties[PROP_PARAMS]);
	}
}
