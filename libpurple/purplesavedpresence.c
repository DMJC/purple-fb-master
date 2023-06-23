/*
 * purple
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpleenums.h"
#include "purplesavedpresence.h"
#include "util.h"

struct _PurpleSavedPresence {
	GObject parent;

	GDateTime *last_used;
	guint use_count;

	char *name;

	PurplePresencePrimitive primitive;
	char *message;
	char *emoji;
};

enum {
	PROP_0,
	PROP_LAST_USED,
	PROP_USE_COUNT,
	PROP_NAME,
	PROP_PRIMITIVE,
	PROP_MESSAGE,
	PROP_EMOJI,
	N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE(PurpleSavedPresence, purple_saved_presence, G_TYPE_OBJECT)

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_saved_presence_set_property(GObject *obj, guint param_id,
                                   const GValue *value, GParamSpec *pspec)
{
	PurpleSavedPresence *presence = PURPLE_SAVED_PRESENCE(obj);

	switch(param_id) {
		case PROP_LAST_USED:
			purple_saved_presence_set_last_used(presence,
			                                    g_value_get_boxed(value));
			break;
		case PROP_USE_COUNT:
			purple_saved_presence_set_use_count(presence,
			                                    g_value_get_uint(value));
			break;
		case PROP_NAME:
			purple_saved_presence_set_name(presence,
			                               g_value_get_string(value));
			break;
		case PROP_PRIMITIVE:
			purple_saved_presence_set_primitive(presence,
			                                    g_value_get_enum(value));
			break;
		case PROP_MESSAGE:
			purple_saved_presence_set_message(presence,
			                                  g_value_get_string(value));
			break;
		case PROP_EMOJI:
			purple_saved_presence_set_emoji(presence,
			                                g_value_get_string(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_saved_presence_get_property(GObject *obj, guint param_id, GValue *value,
                                   GParamSpec *pspec)
{
	PurpleSavedPresence *presence = PURPLE_SAVED_PRESENCE(obj);

	switch(param_id) {
		case PROP_LAST_USED:
			g_value_set_boxed(value,
			                  purple_saved_presence_get_last_used(presence));
			break;
		case PROP_USE_COUNT:
			g_value_set_uint(value,
			                 purple_saved_presence_get_use_count(presence));
			break;
		case PROP_NAME:
			g_value_set_string(value,
			                   purple_saved_presence_get_name(presence));
			break;
		case PROP_PRIMITIVE:
			g_value_set_enum(value,
			                 purple_saved_presence_get_primitive(presence));
			break;
		case PROP_MESSAGE:
			g_value_set_string(value,
			                   purple_saved_presence_get_message(presence));
			break;
		case PROP_EMOJI:
			g_value_set_string(value,
			                   purple_saved_presence_get_emoji(presence));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_saved_presence_init(G_GNUC_UNUSED PurpleSavedPresence *presence) {
}

static void
purple_saved_presence_finalize(GObject *obj) {
	PurpleSavedPresence *presence = PURPLE_SAVED_PRESENCE(obj);

	g_clear_pointer(&presence->last_used, g_date_time_unref);

	g_clear_pointer(&presence->name, g_free);
	g_clear_pointer(&presence->message, g_free);
	g_clear_pointer(&presence->emoji, g_free);

	G_OBJECT_CLASS(purple_saved_presence_parent_class)->finalize(obj);
}

static void
purple_saved_presence_class_init(PurpleSavedPresenceClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_saved_presence_get_property;
	obj_class->set_property = purple_saved_presence_set_property;
	obj_class->finalize = purple_saved_presence_finalize;

	/**
	 * PurpleSavedPresence:last-used:
	 *
	 * The [struct@GLib.DateTime] when this saved presence was last used.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_LAST_USED] = g_param_spec_boxed(
		"last-used", "last-used",
		"The time this presence was last used.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleSavedPresence:use-count:
	 *
	 * The number of times this saved presence has been used.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_USE_COUNT] = g_param_spec_uint(
		"use-count", "use-count",
		"The number of times this saved presence has been used.",
		0, G_MAXUINT, 0,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleSavedPresence:name:
	 *
	 * The name of the saved presence.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_NAME] = g_param_spec_string(
		"name", "name",
		"The name of this saved presence.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleSavedPresence:primitive:
	 *
	 * The [enum@Purple.StatusPrimitive] for this saved presence.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_PRIMITIVE] = g_param_spec_enum(
		"primitive", "primitive",
		"The primitive for this saved presence.",
		PURPLE_TYPE_PRESENCE_PRIMITIVE,
		PURPLE_PRESENCE_PRIMITIVE_OFFLINE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleSavedPresence:message:
	 *
	 * The status message of this saved presence.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_MESSAGE] = g_param_spec_string(
		"message", "message",
		"The status message of this saved presence.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleSavedPresence:emoji:
	 *
	 * The emoji or mood of the presence.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_EMOJI] = g_param_spec_string(
		"emoji", "emoji",
		"The emoji for this saved presence.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleSavedPresence *
purple_saved_presence_new(void) {
	return g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
}

GDateTime *
purple_saved_presence_get_last_used(PurpleSavedPresence *presence) {
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence), NULL);

	return presence->last_used;
}

void
purple_saved_presence_set_last_used(PurpleSavedPresence *presence,
                                    GDateTime *last_used)
{
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	if(presence->last_used != last_used) {
		g_clear_pointer(&presence->last_used, g_date_time_unref);
		if(last_used != NULL) {
			presence->last_used = g_date_time_ref(last_used);
		}

		g_object_notify_by_pspec(G_OBJECT(presence),
		                         properties[PROP_LAST_USED]);
	}
}

guint
purple_saved_presence_get_use_count(PurpleSavedPresence *presence) {
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence), 0);

	return presence->use_count;
}

void
purple_saved_presence_set_use_count(PurpleSavedPresence *presence,
                                    guint use_count)
{
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	if(presence->use_count != use_count) {
		presence->use_count = use_count;

		g_object_notify_by_pspec(G_OBJECT(presence),
		                         properties[PROP_USE_COUNT]);
	}
}

const char *
purple_saved_presence_get_name(PurpleSavedPresence *presence) {
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence), NULL);

	return presence->name;
}

void
purple_saved_presence_set_name(PurpleSavedPresence *presence,
                               const char *name)
{
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	if(!purple_strequal(presence->name, name)) {
		g_free(presence->name);
		presence->name = g_strdup(name);

		g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_NAME]);
	}
}

PurplePresencePrimitive
purple_saved_presence_get_primitive(PurpleSavedPresence *presence) {
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence),
	                     PURPLE_PRESENCE_PRIMITIVE_OFFLINE);

	return presence->primitive;
}

void
purple_saved_presence_set_primitive(PurpleSavedPresence *presence,
                                    PurplePresencePrimitive primitive)
{
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	if(presence->primitive != primitive) {
		presence->primitive = primitive;

		g_object_notify_by_pspec(G_OBJECT(presence),
		                         properties[PROP_PRIMITIVE]);
	}
}

const char *
purple_saved_presence_get_message(PurpleSavedPresence *presence) {
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence), NULL);

	return presence->message;
}

void
purple_saved_presence_set_message(PurpleSavedPresence *presence,
                                  const char *message)
{
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	if(!purple_strequal(presence->message, message)) {
		g_free(presence->message);
		presence->message = g_strdup(message);

		g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_MESSAGE]);
	}
}

const char *
purple_saved_presence_get_emoji(PurpleSavedPresence *presence) {
	g_return_val_if_fail(PURPLE_IS_SAVED_PRESENCE(presence), NULL);

	return presence->emoji;
}

void
purple_saved_presence_set_emoji(PurpleSavedPresence *presence,
                                const char *emoji)
{
	g_return_if_fail(PURPLE_IS_SAVED_PRESENCE(presence));

	if(!purple_strequal(presence->emoji, emoji)) {
		g_free(presence->emoji);
		presence->emoji = g_strdup(emoji);

		g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_EMOJI]);
	}
}

gboolean
purple_saved_presence_equal(PurpleSavedPresence *a, PurpleSavedPresence *b) {
	/* Check if both objects are null. */
	if(a == NULL && b == NULL) {
		return TRUE;
	}

	/* Check if either object is null. */
	if(a == NULL || b == NULL) {
		return FALSE;
	}

	/* Check that both objects are a saved presence. */
	if(!PURPLE_IS_SAVED_PRESENCE(a) || !PURPLE_IS_SAVED_PRESENCE(b)) {
		return FALSE;
	}

	/* If last used is non-null on both compare them. */
	if(a->last_used != NULL && b->last_used != NULL) {
		if(!g_date_time_equal(a->last_used, b->last_used)) {
			return FALSE;
		}
	}

	if(a->last_used == NULL && b->last_used != NULL) {
		return FALSE;
	}

	if(a->last_used != NULL && b->last_used == NULL) {
		return FALSE;
	}

	/* Check the use counts. */
	if(a->use_count != b->use_count) {
		return FALSE;
	}

	/* Check the name. */
	if(!purple_strequal(a->name, b->name)) {
		return FALSE;
	}

	/* Check the primitive. */
	if(a->primitive != b->primitive) {
		return FALSE;
	}

	/* Check the message. */
	if(!purple_strequal(a->message, b->message)) {
		return FALSE;
	}

	/* Check the emoji. */
	if(!purple_strequal(a->emoji, b->emoji)) {
		return FALSE;
	}

	return TRUE;
}
