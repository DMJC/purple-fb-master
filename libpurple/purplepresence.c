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

#include "purplepresence.h"

#include "debug.h"
#include "purpleenums.h"
#include "purpleprivate.h"
#include "util.h"

typedef struct {
	gboolean idle;
	GDateTime *idle_time;
	GDateTime *login_time;

	PurplePresencePrimitive primitive;
	char *message;
	char *emoji;
	gboolean mobile;
	gboolean notifications_disabled;
} PurplePresencePrivate;

enum {
	PROP_0,
	PROP_IDLE,
	PROP_IDLE_TIME,
	PROP_LOGIN_TIME,
	PROP_PRIMITIVE,
	PROP_MESSAGE,
	PROP_EMOJI,
	PROP_MOBILE,
	PROP_NOTIFICATIONS_DISABLED,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(PurplePresence, purple_presence, G_TYPE_OBJECT)

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_presence_set_property(GObject *obj, guint param_id, const GValue *value,
                             GParamSpec *pspec)
{
	PurplePresence *presence = PURPLE_PRESENCE(obj);

	switch (param_id) {
		case PROP_IDLE:
			purple_presence_set_idle(presence, g_value_get_boolean(value),
			                         NULL);
			break;
		case PROP_IDLE_TIME:
			purple_presence_set_idle(presence, TRUE, g_value_get_boxed(value));
			break;
		case PROP_LOGIN_TIME:
			purple_presence_set_login_time(presence, g_value_get_boxed(value));
			break;
		case PROP_PRIMITIVE:
			purple_presence_set_primitive(presence, g_value_get_enum(value));
			break;
		case PROP_MESSAGE:
			purple_presence_set_message(presence, g_value_get_string(value));
			break;
		case PROP_EMOJI:
			purple_presence_set_emoji(presence, g_value_get_string(value));
			break;
		case PROP_MOBILE:
			purple_presence_set_mobile(presence, g_value_get_boolean(value));
			break;
		case PROP_NOTIFICATIONS_DISABLED:
			purple_presence_set_notifications_disabled(presence,
			                                           g_value_get_boolean(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_presence_get_property(GObject *obj, guint param_id, GValue *value,
                             GParamSpec *pspec)
{
	PurplePresence *presence = PURPLE_PRESENCE(obj);

	switch (param_id) {
		case PROP_IDLE:
			g_value_set_boolean(value, purple_presence_is_idle(presence));
			break;
		case PROP_IDLE_TIME:
			g_value_set_boxed(value, purple_presence_get_idle_time(presence));
			break;
		case PROP_LOGIN_TIME:
			g_value_set_boxed(value, purple_presence_get_login_time(presence));
			break;
		case PROP_PRIMITIVE:
			g_value_set_enum(value, purple_presence_get_primitive(presence));
			break;
		case PROP_MESSAGE:
			g_value_set_string(value, purple_presence_get_message(presence));
			break;
		case PROP_EMOJI:
			g_value_set_string(value, purple_presence_get_emoji(presence));
			break;
		case PROP_MOBILE:
			g_value_set_boolean(value, purple_presence_get_mobile(presence));
			break;
		case PROP_NOTIFICATIONS_DISABLED:
			g_value_set_boolean(value,
			                    purple_presence_get_notifications_disabled(presence));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_presence_init(G_GNUC_UNUSED PurplePresence *presence) {
}

static void
purple_presence_finalize(GObject *obj) {
	PurplePresencePrivate *priv = NULL;

	priv = purple_presence_get_instance_private(PURPLE_PRESENCE(obj));

	g_clear_pointer(&priv->idle_time, g_date_time_unref);
	g_clear_pointer(&priv->login_time, g_date_time_unref);

	g_clear_pointer(&priv->message, g_free);
	g_clear_pointer(&priv->emoji, g_free);

	G_OBJECT_CLASS(purple_presence_parent_class)->finalize(obj);
}

static void
purple_presence_class_init(PurplePresenceClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_presence_get_property;
	obj_class->set_property = purple_presence_set_property;
	obj_class->finalize = purple_presence_finalize;

	/**
	 * PurplePresence:idle:
	 *
	 * Whether or not the presence is in an idle state.
	 *
	 * Since: 3.0
	 */
	properties[PROP_IDLE] = g_param_spec_boolean("idle", "Idle",
				"Whether the presence is in idle state.", FALSE,
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:idle-time:
	 *
	 * The time when the presence went idle.
	 *
	 * Since: 3.0
	 */
	properties[PROP_IDLE_TIME] = g_param_spec_boxed(
				"idle-time", "Idle time",
				"The idle time of the presence",
				G_TYPE_DATE_TIME,
				G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:login-time:
	 *
	 * The login-time of the presence.
	 *
	 * Since: 3.0
	 */
	properties[PROP_LOGIN_TIME] = g_param_spec_boxed(
		"login-time", "Login time",
		"The login time of the presence.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:primitive:
	 *
	 * The [enum@Purple.PresencePrimitive] for this presence.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PRIMITIVE] = g_param_spec_enum(
		"primitive", "primitive",
		"The primitive for the presence",
		PURPLE_TYPE_PRESENCE_PRIMITIVE,
		PURPLE_PRESENCE_PRIMITIVE_OFFLINE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:message:
	 *
	 * The status message of the presence.
	 *
	 * Since: 3.0
	 */
	properties[PROP_MESSAGE] = g_param_spec_string(
		"message", "message",
		"The status message",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:emoji:
	 *
	 * The emoji or mood of the presence.
	 *
	 * Since: 3.0
	 */
	properties[PROP_EMOJI] = g_param_spec_string(
		"emoji", "emoji",
		"The emoji for the presence.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:mobile:
	 *
	 * Whether or not the presence is on a mobile device.
	 *
	 * Since: 3.0
	 */
	properties[PROP_MOBILE] = g_param_spec_boolean(
		"mobile", "mobile",
		"Whether or not the presence is on a mobile device.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurplePresence:notifications-disabled:
	 *
	 * Whether or not the presence has notifications disabled.
	 *
	 * Some protocols, like Slack, allow users to set an available schedule. By
	 * default it displays that the user has notifications turned off outside
	 * of that schedule.
	 *
	 * Since: 3.0
	 */
	properties[PROP_NOTIFICATIONS_DISABLED] = g_param_spec_boolean(
		"notifications-disabled", "notifications-disabled",
		"Whether or not this presence has notifications disabled.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurplePresence *
purple_presence_new(void) {
	return g_object_new(PURPLE_TYPE_PRESENCE, NULL);
}

void
purple_presence_set_idle(PurplePresence *presence, gboolean idle,
                         GDateTime *idle_time)
{
	PurplePresencePrivate *priv = NULL;
	PurplePresenceClass *klass = NULL;
	gboolean old_idle;
	GObject *obj = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);
	klass = PURPLE_PRESENCE_GET_CLASS(presence);

	if (priv->idle == idle && priv->idle_time == idle_time) {
		return;
	}

	old_idle = priv->idle;
	priv->idle = idle;

	g_clear_pointer(&priv->idle_time, g_date_time_unref);
	if(idle && idle_time != NULL) {
		priv->idle_time = g_date_time_ref(idle_time);
	}

	obj = G_OBJECT(presence);
	g_object_freeze_notify(obj);
	g_object_notify_by_pspec(obj, properties[PROP_IDLE]);
	g_object_notify_by_pspec(obj, properties[PROP_IDLE_TIME]);
	g_object_thaw_notify(obj);

	if(klass->update_idle) {
		klass->update_idle(presence, old_idle);
	}
}

void
purple_presence_set_login_time(PurplePresence *presence, GDateTime *login_time)
{
	PurplePresencePrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);

	if(priv->login_time != NULL && login_time != NULL) {
		if(g_date_time_equal(priv->login_time, login_time)) {
			return;
		}
	}

	if(priv->login_time != NULL) {
		g_date_time_unref(priv->login_time);
	}

	if(login_time != NULL) {
		priv->login_time = g_date_time_ref(login_time);
	} else {
		priv->login_time = NULL;
	}

	g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_LOGIN_TIME]);
}

gboolean
purple_presence_is_available(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), FALSE);

	priv = purple_presence_get_instance_private(presence);

	return priv->primitive == PURPLE_PRESENCE_PRIMITIVE_AVAILABLE;
}

gboolean
purple_presence_is_online(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), FALSE);

	priv = purple_presence_get_instance_private(presence);

	switch(priv->primitive) {
	case PURPLE_PRESENCE_PRIMITIVE_AVAILABLE:
	case PURPLE_PRESENCE_PRIMITIVE_IDLE:
	case PURPLE_PRESENCE_PRIMITIVE_INVISIBLE:
	case PURPLE_PRESENCE_PRIMITIVE_AWAY:
	case PURPLE_PRESENCE_PRIMITIVE_DO_NOT_DISTURB:
	case PURPLE_PRESENCE_PRIMITIVE_STREAMING:
		return TRUE;
	case PURPLE_PRESENCE_PRIMITIVE_OFFLINE:
	default:
		return FALSE;
	}
}

gboolean
purple_presence_is_idle(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), FALSE);

	if(!purple_presence_is_online(presence)) {
		return FALSE;
	}

	priv = purple_presence_get_instance_private(presence);

	return priv->idle;
}

GDateTime *
purple_presence_get_idle_time(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), NULL);

	priv = purple_presence_get_instance_private(presence);

	return priv->idle_time;
}

GDateTime *
purple_presence_get_login_time(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), 0);

	priv = purple_presence_get_instance_private(presence);

	return priv->login_time;
}

gint
purple_presence_compare(PurplePresence *presence1, PurplePresence *presence2) {
	PurplePresencePrimitive primitive1 = PURPLE_PRESENCE_PRIMITIVE_OFFLINE;
	PurplePresencePrimitive primitive2 = PURPLE_PRESENCE_PRIMITIVE_OFFLINE;
	GDateTime *idle1 = NULL;
	GDateTime *idle2 = NULL;
	GDateTime *now = NULL;
	GTimeSpan diff1 = 0;
	GTimeSpan diff2 = 0;

	if(presence1 == presence2) {
		return 0;
	} else if (presence1 == NULL) {
		return 1;
	} else if (presence2 == NULL) {
		return -1;
	}

	primitive1 = purple_presence_get_primitive(presence1);
	primitive2 = purple_presence_get_primitive(presence2);

	if(primitive1 != PURPLE_PRESENCE_PRIMITIVE_OFFLINE &&
	   primitive2 == PURPLE_PRESENCE_PRIMITIVE_OFFLINE)
	{
		return -1;
	} else if(primitive1 == PURPLE_PRESENCE_PRIMITIVE_OFFLINE &&
	          primitive2 != PURPLE_PRESENCE_PRIMITIVE_OFFLINE)
	{
		return 1;
	}

	idle1 = purple_presence_get_idle_time(presence1);
	idle2 = purple_presence_get_idle_time(presence2);

	if(idle1 == NULL && idle2 == NULL) {
		return 0;
	} else if(idle1 == NULL && idle2 != NULL) {
		return -1;
	} else if(idle1 != NULL && idle2 == NULL) {
		return 1;
	}

	now = g_date_time_new_now_local();
	diff1 = g_date_time_difference(now, idle1);
	diff2 = g_date_time_difference(now, idle2);
	g_date_time_unref(now);

	if(diff1 > diff2) {
		return 1;
	} else if (diff1 < diff2) {
		return -1;
	}

	return 0;
}

PurplePresencePrimitive
purple_presence_get_primitive(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence),
	                     PURPLE_PRESENCE_PRIMITIVE_OFFLINE);

	priv = purple_presence_get_instance_private(presence);

	return priv->primitive;
}

void
purple_presence_set_primitive(PurplePresence *presence,
                              PurplePresencePrimitive primitive)
{
	PurplePresencePrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);

	if(priv->primitive != primitive) {
		priv->primitive = primitive;

		g_object_notify_by_pspec(G_OBJECT(presence),
		                         properties[PROP_PRIMITIVE]);
	}
}

const char *
purple_presence_get_message(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), NULL);

	priv = purple_presence_get_instance_private(presence);

	return priv->message;
}

void
purple_presence_set_message(PurplePresence *presence, const char *message) {
	PurplePresencePrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);

	if(!purple_strequal(priv->message, message)) {
		g_free(priv->message);
		priv->message = g_strdup(message);

		g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_MESSAGE]);
	}
}

const char *
purple_presence_get_emoji(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), NULL);

	priv = purple_presence_get_instance_private(presence);

	return priv->emoji;
}

void
purple_presence_set_emoji(PurplePresence *presence, const char *emoji) {
	PurplePresencePrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);

	if(!purple_strequal(priv->emoji, emoji)) {
		g_free(priv->emoji);
		priv->emoji = g_strdup(emoji);

		g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_EMOJI]);
	}
}

gboolean
purple_presence_get_mobile(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), FALSE);

	priv = purple_presence_get_instance_private(presence);

	return priv->mobile;
}

void
purple_presence_set_mobile(PurplePresence *presence, gboolean mobile) {
	PurplePresencePrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);

	if(priv->mobile != mobile) {
		priv->mobile = mobile;

		g_object_notify_by_pspec(G_OBJECT(presence), properties[PROP_MOBILE]);
	}
}

gboolean
purple_presence_get_notifications_disabled(PurplePresence *presence) {
	PurplePresencePrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_PRESENCE(presence), FALSE);

	priv = purple_presence_get_instance_private(presence);

	return priv->notifications_disabled;
}

void
purple_presence_set_notifications_disabled(PurplePresence *presence,
                                           gboolean notifications_disabled)
{
	PurplePresencePrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_PRESENCE(presence));

	priv = purple_presence_get_instance_private(presence);

	if(priv->notifications_disabled != notifications_disabled) {
		priv->notifications_disabled = notifications_disabled;

		g_object_notify_by_pspec(G_OBJECT(presence),
		                         properties[PROP_NOTIFICATIONS_DISABLED]);
	}
}

const char *
purple_presence_primitive_to_string(PurplePresencePrimitive primitive) {
	switch(primitive) {
		case PURPLE_PRESENCE_PRIMITIVE_OFFLINE:
			return _("Offline");
			break;
		case PURPLE_PRESENCE_PRIMITIVE_AVAILABLE:
			return _("Available");
			break;
		case PURPLE_PRESENCE_PRIMITIVE_IDLE:
			return _("Idle");
			break;
		case PURPLE_PRESENCE_PRIMITIVE_INVISIBLE:
			return _("Invisible");
			break;
		case PURPLE_PRESENCE_PRIMITIVE_AWAY:
			return _("Away");
			break;
		case PURPLE_PRESENCE_PRIMITIVE_DO_NOT_DISTURB:
			return _("Do not disturb");
			break;
		case PURPLE_PRESENCE_PRIMITIVE_STREAMING:
			return _("Streaming");
			break;
		default:
			return _("Unknown");
			break;
	}
}
