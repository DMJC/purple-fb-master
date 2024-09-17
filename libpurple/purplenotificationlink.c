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

#include "purplenotificationlink.h"

#include "util.h"

struct _PurpleNotificationLink {
	PurpleNotification parent;

	char *link_text;
	char *link_uri;
};

enum {
	PROP_0,
	PROP_LINK_TEXT,
	PROP_LINK_URI,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_notification_link_set_link_text(PurpleNotificationLink *notification,
                                       const char *link_text)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION_LINK(notification));

	if(g_set_str(&notification->link_text, link_text)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_LINK_TEXT]);
	}
}

static void
purple_notification_link_set_link_uri(PurpleNotificationLink *notification,
                                      const char *link_uri)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION_LINK(notification));
	g_return_if_fail(!purple_strempty(link_uri));

	if(g_set_str(&notification->link_uri, link_uri)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_LINK_URI]);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleNotificationLink, purple_notification_link,
                    PURPLE_TYPE_NOTIFICATION)

static void
purple_notification_link_finalize(GObject *obj) {
	PurpleNotificationLink *notification = PURPLE_NOTIFICATION_LINK(obj);

	g_clear_pointer(&notification->link_text, g_free);
	g_clear_pointer(&notification->link_uri, g_free);

	G_OBJECT_CLASS(purple_notification_link_parent_class)->finalize(obj);
}

static void
purple_notification_link_get_property(GObject *obj, guint param_id,
                                      GValue *value, GParamSpec *pspec)
{
	PurpleNotificationLink *notification = PURPLE_NOTIFICATION_LINK(obj);

	switch(param_id) {
	case PROP_LINK_TEXT:
		g_value_set_string(value,
		                   purple_notification_link_get_link_text(notification));
		break;
	case PROP_LINK_URI:
		g_value_set_string(value,
		                   purple_notification_link_get_link_uri(notification));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_notification_link_set_property(GObject *obj, guint param_id,
                                      const GValue *value, GParamSpec *pspec)
{
	PurpleNotificationLink *notification = PURPLE_NOTIFICATION_LINK(obj);

	switch(param_id) {
	case PROP_LINK_TEXT:
		purple_notification_link_set_link_text(notification,
		                                       g_value_get_string(value));
		break;
	case PROP_LINK_URI:
		purple_notification_link_set_link_uri(notification,
		                                      g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_notification_link_init(G_GNUC_UNUSED PurpleNotificationLink *notification)
{
}

static void
purple_notification_link_class_init(PurpleNotificationLinkClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_notification_link_finalize;
	obj_class->get_property = purple_notification_link_get_property;
	obj_class->set_property = purple_notification_link_set_property;

	/**
	 * PurpleNotificationLink:link-text:
	 *
	 * The text to display instead of the URI.
	 *
	 * If this is not set, the value of [property@NotificationLink:link-uri]
	 * will be returned.
	 *
	 * Since: 3.0
	 */
	properties[PROP_LINK_TEXT] = g_param_spec_string(
		"link-text", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotificationLink:link-uri:
	 *
	 * The URI for this notification.
	 *
	 * Since: 3.0
	 */
	properties[PROP_LINK_URI] = g_param_spec_string(
		"link-uri", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleNotification *
purple_notification_link_new(const char *id, const char *title,
                             const char *link_text, const char *link_uri)
{
	g_return_val_if_fail(!purple_strempty(title), NULL);
	g_return_val_if_fail(!purple_strempty(link_uri), NULL);

	return g_object_new(
		PURPLE_TYPE_NOTIFICATION_LINK,
		"id", id,
		"title", title,
		"link-text", link_text,
		"link-uri", link_uri,
		NULL);
}

const char *
purple_notification_link_get_link_text(PurpleNotificationLink *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION_LINK(notification), NULL);

	if(!purple_strempty(notification->link_text)) {
		return notification->link_text;
	}

	return notification->link_uri;
}

const char *
purple_notification_link_get_link_uri(PurpleNotificationLink *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION_LINK(notification), NULL);

	return notification->link_uri;
}
