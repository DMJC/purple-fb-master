/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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

#include "purplenotification.h"

#include "purpleenums.h"

struct _PurpleNotification {
	GObject parent;

	gchar *id;
	PurpleNotificationType type;
	PurpleAccount *account;

	GDateTime *created_timestamp;
	gchar *title;
	gchar *icon_name;
	gboolean read;
	gboolean interactive;

	gpointer data;
	GDestroyNotify data_destroy_func;
};

enum {
	PROP_0,
	PROP_ID,
	PROP_TYPE,
	PROP_ACCOUNT,
	PROP_CREATED_TIMESTAMP,
	PROP_TITLE,
	PROP_ICON_NAME,
	PROP_READ,
	PROP_INTERACTIVE,
	PROP_DATA,
	PROP_DATA_DESTROY_FUNC,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE(PurpleNotification, purple_notification, G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_notification_set_id(PurpleNotification *notification, const gchar *id) {
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	if(id == NULL) {
		notification->id = g_uuid_string_random();
	} else {
		notification->id = g_strdup(id);
	}

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_ID]);
}

static void
purple_notification_set_notification_type(PurpleNotification *notification,
                                          PurpleNotificationType type)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	notification->type = type;

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_TYPE]);
}

static void
purple_notification_set_account(PurpleNotification *notification,
                                PurpleAccount *account)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	if(g_set_object(&notification->account, account)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_ACCOUNT]);
	}
}

static void
purple_notification_set_data(PurpleNotification *notification, gpointer data) {
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	notification->data = data;

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_DATA]);
}

static void
purple_notification_set_data_destroy_func(PurpleNotification *notification,
                                          GDestroyNotify data_destroy_func)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	notification->data_destroy_func = data_destroy_func;

	g_object_notify_by_pspec(G_OBJECT(notification),
	                         properties[PROP_DATA_DESTROY_FUNC]);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_notification_get_property(GObject *obj, guint param_id, GValue *value,
                                 GParamSpec *pspec)
{
	PurpleNotification *notification = PURPLE_NOTIFICATION(obj);

	switch(param_id) {
		case PROP_ID:
			g_value_set_string(value,
			                   purple_notification_get_id(notification));
			break;
		case PROP_TYPE:
			g_value_set_enum(value,
			                 purple_notification_get_notification_type(notification));
			break;
		case PROP_ACCOUNT:
			g_value_set_object(value,
			                   purple_notification_get_account(notification));
			break;
		case PROP_CREATED_TIMESTAMP:
			g_value_set_boxed(value,
			                  purple_notification_get_created_timestamp(notification));
			break;
		case PROP_TITLE:
			g_value_set_string(value,
			                   purple_notification_get_title(notification));
			break;
		case PROP_ICON_NAME:
			g_value_set_string(value,
			                   purple_notification_get_icon_name(notification));
			break;
		case PROP_READ:
			g_value_set_boolean(value,
			                    purple_notification_get_read(notification));
			break;
		case PROP_INTERACTIVE:
			g_value_set_boolean(value,
			                    purple_notification_get_interactive(notification));
			break;
		case PROP_DATA:
			g_value_set_pointer(value,
			                    purple_notification_get_data(notification));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_notification_set_property(GObject *obj, guint param_id,
                                 const GValue *value, GParamSpec *pspec)
{
	PurpleNotification *notification = PURPLE_NOTIFICATION(obj);

	switch(param_id) {
		case PROP_ID:
			purple_notification_set_id(notification,
			                           g_value_get_string(value));
			break;
		case PROP_TYPE:
			purple_notification_set_notification_type(notification,
			                                          g_value_get_enum(value));
			break;
		case PROP_ACCOUNT:
			purple_notification_set_account(notification,
			                                g_value_get_object(value));
			break;
		case PROP_CREATED_TIMESTAMP:
			purple_notification_set_created_timestamp(notification,
			                                          g_value_get_boxed(value));
			break;
		case PROP_TITLE:
			purple_notification_set_title(notification,
			                              g_value_get_string(value));
			break;
		case PROP_ICON_NAME:
			purple_notification_set_icon_name(notification,
			                                  g_value_get_string(value));
			break;
		case PROP_READ:
			purple_notification_set_read(notification,
			                             g_value_get_boolean(value));
			break;
		case PROP_INTERACTIVE:
			purple_notification_set_interactive(notification,
			                                    g_value_get_boolean(value));
			break;
		case PROP_DATA:
			purple_notification_set_data(notification,
			                             g_value_get_pointer(value));
			break;
		case PROP_DATA_DESTROY_FUNC:
			purple_notification_set_data_destroy_func(notification,
			                                          g_value_get_pointer(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
			break;
	}
}

static void
purple_notification_finalize(GObject *obj) {
	PurpleNotification *notification = PURPLE_NOTIFICATION(obj);

	g_clear_pointer(&notification->id, g_free);
	g_clear_object(&notification->account);

	g_clear_pointer(&notification->created_timestamp, g_date_time_unref);
	g_clear_pointer(&notification->title, g_free);
	g_clear_pointer(&notification->icon_name, g_free);

	if(notification->data_destroy_func != NULL) {
		notification->data_destroy_func(notification->data);
	}

	G_OBJECT_CLASS(purple_notification_parent_class)->finalize(obj);
}

static void
purple_notification_init(PurpleNotification *notification) {
	purple_notification_set_id(notification, NULL);

	if(notification->created_timestamp == NULL) {
		purple_notification_set_created_timestamp(notification, NULL);
	}
}

static void
purple_notification_class_init(PurpleNotificationClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_notification_get_property;
	obj_class->set_property = purple_notification_set_property;
	obj_class->finalize = purple_notification_finalize;

	/**
	 * PurpleNotification::id:
	 *
	 * The ID of the notification. Used for things that need to address it.
	 * This is auto populated at creation time.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", "id",
		"The identifier of the notification.",
		NULL,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS
	);

	/**
	 * PurpleNotification::type:
	 *
	 * The [enum@NotificationType] of this notification.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TYPE] = g_param_spec_enum(
		"type", "type",
		"The type of notification.",
		PURPLE_TYPE_NOTIFICATION_TYPE,
		PURPLE_NOTIFICATION_TYPE_UNKNOWN,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::account:
	 *
	 * An optional [class@Account] that this notification is for.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", "account",
		"The optional account that this notification is for.",
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::created-timestamp:
	 *
	 * The creation time of this notification. This always represented as UTC
	 * internally, and will be set to UTC now by default.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_CREATED_TIMESTAMP] = g_param_spec_boxed(
		"created-timestamp", "created-timestamp",
		"The timestamp when this notification was created.",
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::title:
	 *
	 * An optional title for this notification. A user interface may or may not
	 * choose to use this when displaying the notification. Regardless, this
	 * should be a translated string.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_TITLE] = g_param_spec_string(
		"title", "title",
		"The title for the notification.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::icon-name:
	 *
	 * The icon-name in the icon theme to use for the notification. A user
	 * interface may or may not choose to use this when display the
	 * notification.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name", "icon-name",
		"The icon name for the notification.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::read:
	 *
	 * Whether or not the notification has been read.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_READ] = g_param_spec_boolean(
		"read", "read",
		"Whether or not the notification has been read.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::interactive:
	 *
	 * Whether or not the notification can be interacted with.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_INTERACTIVE] = g_param_spec_boolean(
		"interactive", "interactive",
		"Whether or not the notification can be interacted with.",
		FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::data:
	 *
	 * Data specific to the [enum@NotificationType] for the notification.
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_DATA] = g_param_spec_pointer(
		"data", "data",
		"The type specific data for the notification.",
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification::data-destroy-func:
	 *
	 * A [func@GLib.DestroyFunc] to call to free
	 * [property@PurpleNotification:data].
	 *
	 * Since: 3.0.0
	 */
	properties[PROP_DATA_DESTROY_FUNC] = g_param_spec_pointer(
		"data-destroy-func", "data-destroy-func",
		"The destroy function to clean up the data property.",
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleNotification *
purple_notification_new(PurpleNotificationType type, PurpleAccount *account,
                        gpointer data, GDestroyNotify data_destroy_func)
{
	return g_object_new(PURPLE_TYPE_NOTIFICATION,
	                    "type", type,
	                    "account", account,
	                    "data", data,
	                    "data-destroy-func", data_destroy_func,
	                    NULL);
}

const gchar *
purple_notification_get_id(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	return notification->id;
}

PurpleNotificationType
purple_notification_get_notification_type(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification),
	                     PURPLE_NOTIFICATION_TYPE_UNKNOWN);

	return notification->type;
}

PurpleAccount *
purple_notification_get_account(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	return notification->account;
}

GDateTime *
purple_notification_get_created_timestamp(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	return notification->created_timestamp;
}

void
purple_notification_set_created_timestamp(PurpleNotification *notification,
                                          GDateTime *timestamp)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	g_clear_pointer(&notification->created_timestamp, g_date_time_unref);

	if(timestamp == NULL) {
		notification->created_timestamp = g_date_time_new_now_utc();
	} else {
		notification->created_timestamp = g_date_time_to_utc(timestamp);
	}

	g_object_notify_by_pspec(G_OBJECT(notification),
	                         properties[PROP_CREATED_TIMESTAMP]);
}

const gchar *
purple_notification_get_title(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	return notification->title;
}

void
purple_notification_set_title(PurpleNotification *notification,
                              const gchar *title)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	g_free(notification->title);
	notification->title = g_strdup(title);

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_TITLE]);
}

const gchar *
purple_notification_get_icon_name(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	return notification->icon_name;
}

void
purple_notification_set_icon_name(PurpleNotification *notification,
                                  const gchar *icon_name)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	g_free(notification->icon_name);
	notification->icon_name = g_strdup(icon_name);

	g_object_notify_by_pspec(G_OBJECT(notification),
	                         properties[PROP_ICON_NAME]);
}

gboolean
purple_notification_get_read(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), FALSE);

	return notification->read;
}

void
purple_notification_set_read(PurpleNotification *notification, gboolean read) {
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	if(notification->read != read) {
		notification->read = read;

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_READ]);
	}
}

gboolean
purple_notification_get_interactive(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), FALSE);

	return notification->interactive;
}

void
purple_notification_set_interactive(PurpleNotification *notification,
                                    gboolean interactive)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	if(notification->interactive != interactive) {
		notification->interactive = interactive;

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_INTERACTIVE]);
	}
}

gpointer
purple_notification_get_data(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	return notification->data;
}
