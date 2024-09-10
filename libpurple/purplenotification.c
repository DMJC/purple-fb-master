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

#include <birb.h>

#include "purplenotification.h"

#include "purpleenums.h"
#include "purplenotificationaddcontact.h"
#include "util.h"

typedef struct {
	char *id;
	PurpleNotificationType type;
	PurpleAccount *account;

	GDateTime *created_timestamp;
	char *title;
	char *subtitle;

	char *icon_name;
	gboolean read;
	gboolean interactive;
	gboolean persistent;

	gpointer data;
	GDestroyNotify data_destroy_func;

	gboolean deleted;
} PurpleNotificationPrivate;

enum {
	SIG_DELETED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

enum {
	PROP_0,
	PROP_ID,
	PROP_TYPE,
	PROP_ACCOUNT,
	PROP_CREATED_TIMESTAMP,
	PROP_TITLE,
	PROP_SUBTITLE,
	PROP_ICON_NAME,
	PROP_READ,
	PROP_INTERACTIVE,
	PROP_PERSISTENT,
	PROP_DATA,
	PROP_DATA_DESTROY_FUNC,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

G_DEFINE_TYPE_WITH_PRIVATE(PurpleNotification, purple_notification,
                           G_TYPE_OBJECT)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_notification_set_id(PurpleNotification *notification, const char *id) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(id == NULL) {
		priv->id = g_uuid_string_random();
	} else {
		priv->id = g_strdup(id);
	}

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_ID]);
}

static void
purple_notification_set_notification_type(PurpleNotification *notification,
                                          PurpleNotificationType type)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	priv->type = type;

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_TYPE]);
}

static void
purple_notification_set_data(PurpleNotification *notification, gpointer data) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	priv->data = data;

	g_object_notify_by_pspec(G_OBJECT(notification), properties[PROP_DATA]);
}

static void
purple_notification_set_data_destroy_func(PurpleNotification *notification,
                                          GDestroyNotify data_destroy_func)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	priv->data_destroy_func = data_destroy_func;

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
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		g_value_set_enum(value,
		                 purple_notification_get_notification_type(notification));
		G_GNUC_END_IGNORE_DEPRECATIONS
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
	case PROP_SUBTITLE:
		g_value_set_string(value,
		                   purple_notification_get_subtitle(notification));
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
	case PROP_PERSISTENT:
		g_value_set_boolean(value,
		                    purple_notification_get_persistent(notification));
		break;
	case PROP_DATA:
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		g_value_set_pointer(value,
		                    purple_notification_get_data(notification));
		G_GNUC_END_IGNORE_DEPRECATIONS
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
	case PROP_SUBTITLE:
		purple_notification_set_subtitle(notification,
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
	case PROP_PERSISTENT:
		purple_notification_set_persistent(notification,
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
	PurpleNotificationPrivate *priv = NULL;

	priv = purple_notification_get_instance_private(notification);

	g_clear_pointer(&priv->id, g_free);
	g_clear_object(&priv->account);

	birb_date_time_clear(&priv->created_timestamp);
	g_clear_pointer(&priv->title, g_free);
	g_clear_pointer(&priv->subtitle, g_free);
	g_clear_pointer(&priv->icon_name, g_free);

	if(priv->data_destroy_func != NULL) {
		priv->data_destroy_func(priv->data);
	}

	G_OBJECT_CLASS(purple_notification_parent_class)->finalize(obj);
}

static void
purple_notification_init(G_GNUC_UNUSED PurpleNotification *notification) {
}

static void
purple_notification_class_init(PurpleNotificationClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->get_property = purple_notification_get_property;
	obj_class->set_property = purple_notification_set_property;
	obj_class->finalize = purple_notification_finalize;

	/**
	 * PurpleNotification:id:
	 *
	 * The ID of the notification.
	 *
	 * This used for things that need to address it.
	 *
	 * If not set, this will be auto populated at creation time.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ID] = g_param_spec_string(
		"id", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:type:
	 *
	 * The [enum@NotificationType] of this notification.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TYPE] = g_param_spec_enum(
		"type", NULL, NULL,
		PURPLE_TYPE_NOTIFICATION_TYPE,
		PURPLE_NOTIFICATION_TYPE_UNKNOWN,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_DEPRECATED |
		G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:account:
	 *
	 * An optional [class@Account] that this notification is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ACCOUNT] = g_param_spec_object(
		"account", NULL, NULL,
		PURPLE_TYPE_ACCOUNT,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:created-timestamp:
	 *
	 * The creation time of this notification. This always represented as UTC
	 * internally, and will be set to UTC now by default.
	 *
	 * Since: 3.0
	 */
	properties[PROP_CREATED_TIMESTAMP] = g_param_spec_boxed(
		"created-timestamp", NULL, NULL,
		G_TYPE_DATE_TIME,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:title:
	 *
	 * An optional title for this notification. A user interface may or may not
	 * choose to use this when displaying the notification. Regardless, this
	 * should be a translated string.
	 *
	 * Since: 3.0
	 */
	properties[PROP_TITLE] = g_param_spec_string(
		"title", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:subtitle:
	 *
	 * An optional subtitle for this notification. A user interface may or may
	 * not choose to use this when displaying the notification. Regardless,
	 * this should be a translated string.
	 *
	 * Since: 3.0
	 */
	properties[PROP_SUBTITLE] = g_param_spec_string(
		"subtitle", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:icon-name:
	 *
	 * The icon-name in the icon theme to use for the notification. A user
	 * interface may or may not choose to use this when display the
	 * notification.
	 *
	 * Since: 3.0
	 */
	properties[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name", NULL, NULL,
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:read:
	 *
	 * Whether or not the notification has been read.
	 *
	 * Since: 3.0
	 */
	properties[PROP_READ] = g_param_spec_boolean(
		"read", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:interactive:
	 *
	 * Whether or not the notification can be interacted with.
	 *
	 * Since: 3.0
	 */
	properties[PROP_INTERACTIVE] = g_param_spec_boolean(
		"interactive", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:persistent:
	 *
	 * Whether or not the notification can be dismissed by users.
	 *
	 * Since: 3.0
	 */
	properties[PROP_PERSISTENT] = g_param_spec_boolean(
		"persistent", NULL, NULL,
		FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:data:
	 *
	 * Data specific to the [enum@NotificationType] for the notification.
	 *
	 * Since: 3.0
	 */
	properties[PROP_DATA] = g_param_spec_pointer(
		"data", NULL, NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_DEPRECATED |
		G_PARAM_STATIC_STRINGS);

	/**
	 * PurpleNotification:data-destroy-func:
	 *
	 * A function to call to free [property@PurpleNotification:data].
	 *
	 * Since: 3.0
	 */
	properties[PROP_DATA_DESTROY_FUNC] = g_param_spec_pointer(
		"data-destroy-func", NULL, NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_DEPRECATED |
		G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);

	/**
	 * PurpleNotification::deleted:
	 * @notification: The instance.
	 *
	 * Emitted when the notification is deleted. This is typically done by a
	 * user interface calling [method@PurpleNotification.delete].
	 *
	 * Since: 3.0
	 */
	signals[SIG_DELETED] = g_signal_new_class_handler(
		"deleted",
		G_OBJECT_CLASS_TYPE(klass),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleNotification *
purple_notification_new(PurpleNotificationType type, PurpleAccount *account,
                        gpointer data, GDestroyNotify data_destroy_func)
{
	PurpleNotification *notification = NULL;
	char *envvar = NULL;

	/* This is temporary until this function gets replaced with the generic one
	 * below.
	 */
	envvar = g_strdup(g_getenv("G_ENABLE_DIAGNOSTIC"));
	g_unsetenv("G_ENABLE_DIAGNOSTIC");

	notification = g_object_new(PURPLE_TYPE_NOTIFICATION,
	                    "type", type,
	                    "account", account,
	                    "data", data,
	                    "data-destroy-func", data_destroy_func,
	                    NULL);

	g_setenv("G_ENABLE_DIAGNOSTIC", envvar, FALSE);
	g_free(envvar);

	return notification;
}

PurpleNotification *
purple_notification_new_generic(const char *id, const char *title) {
	return g_object_new(PURPLE_TYPE_NOTIFICATION,
	                    "id", id,
	                    "title", title,
	                    NULL);
}

PurpleNotification *
purple_notification_new_from_add_contact_request(PurpleAddContactRequest *request)
{
	g_return_val_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request), NULL);

	return purple_notification_add_contact_new(NULL, request);
}

PurpleNotification *
purple_notification_new_from_authorization_request(PurpleAuthorizationRequest *authorization_request)
{
	PurpleAccount *account = NULL;
	PurpleNotification *notification = NULL;
	char *title = NULL;
	const char *alias = NULL;
	const char *username = NULL;

	g_return_val_if_fail(PURPLE_IS_AUTHORIZATION_REQUEST(authorization_request),
	                     NULL);

	account = purple_authorization_request_get_account(authorization_request);
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	notification = purple_notification_new(PURPLE_NOTIFICATION_TYPE_AUTHORIZATION_REQUEST,
	                                       account, authorization_request,
	                                       g_object_unref);
	G_GNUC_END_IGNORE_DEPRECATIONS

	username = purple_authorization_request_get_username(authorization_request);
	alias = purple_authorization_request_get_alias(authorization_request);

	if(alias != NULL && *alias != '\0') {
		title = g_strdup_printf(_("%s (%s) would like to add %s to their"
		                          " contact list"),
		                        alias, username,
		                        purple_account_get_username(account));
	} else {
		title = g_strdup_printf(_("%s would like to add %s to their contact"
		                          " list"),
		                        username,
		                        purple_account_get_username(account));
	}

	purple_notification_set_title(notification, title);
	g_free(title);

	return notification;
}

PurpleNotification *
purple_notification_new_from_connection_error(PurpleAccount *account,
                                              PurpleConnectionErrorInfo *info)
{
	PurpleConnectionErrorInfo *copy = NULL;
	PurpleNotification *notification = NULL;
	PurpleProtocol *protocol = NULL;
	char *title = NULL;
	const char *username = NULL;

	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);
	g_return_val_if_fail(info != NULL, NULL);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	copy = purple_connection_error_info_copy(info);
	notification = purple_notification_new(PURPLE_NOTIFICATION_TYPE_CONNECTION_ERROR,
	                                       account, copy,
	                                       (GDestroyNotify)purple_connection_error_info_free);
	G_GNUC_END_IGNORE_DEPRECATIONS

	/* Set the title of the notification. */
	username = purple_account_get_username(account);
	if(purple_account_get_enabled(account)) {
		title = g_strdup_printf(_("%s disconnected"), username);
	} else {
		title = g_strdup_printf(_("%s disabled"), username);
	}
	purple_notification_set_title(notification, title);
	g_free(title);

	/* Add the protocol's icon as the notification's icon. */
	protocol = purple_account_get_protocol(account);
	if(PURPLE_IS_PROTOCOL(protocol)) {
		const char *icon_name = purple_protocol_get_icon_name(protocol);

		if(icon_name != NULL) {
			purple_notification_set_icon_name(notification, icon_name);
		}
	}

	return notification;
}

const char *
purple_notification_get_id(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->id;
}

PurpleNotificationType
purple_notification_get_notification_type(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification),
	                     PURPLE_NOTIFICATION_TYPE_UNKNOWN);

	priv = purple_notification_get_instance_private(notification);

	return priv->type;
}

PurpleAccount *
purple_notification_get_account(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->account;
}

void
purple_notification_set_account(PurpleNotification *notification,
                                PurpleAccount *account)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(g_set_object(&priv->account, account)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_ACCOUNT]);
	}
}

GDateTime *
purple_notification_get_created_timestamp(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->created_timestamp;
}

void
purple_notification_set_created_timestamp(PurpleNotification *notification,
                                          GDateTime *timestamp)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(birb_date_time_set(&priv->created_timestamp, timestamp)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_CREATED_TIMESTAMP]);
	}
}

void
purple_notification_set_created_timestamp_now(PurpleNotification *notification)
{
	GDateTime *timestamp = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	timestamp = g_date_time_new_now_local();
	purple_notification_set_created_timestamp(notification, timestamp);
	g_date_time_unref(timestamp);
}

const char *
purple_notification_get_title(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->title;
}

void
purple_notification_set_title(PurpleNotification *notification,
                              const char *title)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(g_set_str(&priv->title, title)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_TITLE]);
	}
}

const char *
purple_notification_get_subtitle(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->subtitle;
}

void
purple_notification_set_subtitle(PurpleNotification *notification,
                                 const char *subtitle)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(g_set_str(&priv->subtitle, subtitle)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_SUBTITLE]);
	}
}

const char *
purple_notification_get_icon_name(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->icon_name;
}

void
purple_notification_set_icon_name(PurpleNotification *notification,
                                  const char *icon_name)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(g_set_str(&priv->icon_name, icon_name)) {
		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_ICON_NAME]);
	}
}

gboolean
purple_notification_get_read(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), FALSE);

	priv = purple_notification_get_instance_private(notification);

	return priv->read;
}

void
purple_notification_set_read(PurpleNotification *notification, gboolean read) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(priv->read != read) {
		priv->read = read;

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_READ]);
	}
}

gboolean
purple_notification_get_interactive(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), FALSE);

	priv = purple_notification_get_instance_private(notification);

	return priv->interactive;
}

void
purple_notification_set_interactive(PurpleNotification *notification,
                                    gboolean interactive)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(priv->interactive != interactive) {
		priv->interactive = interactive;

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_INTERACTIVE]);
	}
}

gboolean
purple_notification_get_persistent(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), FALSE);

	priv = purple_notification_get_instance_private(notification);

	return priv->persistent;
}

void
purple_notification_set_persistent(PurpleNotification *notification,
                                   gboolean persistent)
{
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	if(priv->persistent != persistent) {
		priv->persistent = persistent;

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_PERSISTENT]);
	}
}

gpointer
purple_notification_get_data(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_val_if_fail(PURPLE_IS_NOTIFICATION(notification), NULL);

	priv = purple_notification_get_instance_private(notification);

	return priv->data;
}

int
purple_notification_compare(gconstpointer a, gconstpointer b) {
	PurpleNotification *notification_a = NULL;
	PurpleNotification *notification_b = NULL;
	PurpleNotificationPrivate *priv_a = NULL;
	PurpleNotificationPrivate *priv_b = NULL;

	if(a == NULL && b == NULL) {
		return 0;
	}

	if(a == NULL) {
		return -1;
	}

	if(b == NULL) {
		return 1;
	}

	notification_a = (PurpleNotification *)a;
	notification_b = (PurpleNotification *)b;

	priv_a = purple_notification_get_instance_private(notification_a);
	priv_b = purple_notification_get_instance_private(notification_b);

	return g_date_time_compare(priv_a->created_timestamp,
	                           priv_b->created_timestamp);
}

void
purple_notification_delete(PurpleNotification *notification) {
	PurpleNotificationPrivate *priv = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION(notification));

	priv = purple_notification_get_instance_private(notification);

	/* Calling this multiple times is a programming error. */
	g_return_if_fail(priv->deleted == FALSE);

	priv->deleted = TRUE;

	g_signal_emit(notification, signals[SIG_DELETED], 0);
}
