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

#include "purpleauthorizationrequestnotification.h"

#include "util.h"

enum {
	PROP_0,
	PROP_AUTHORIZATION_REQUEST,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, 0};

struct _PurpleAuthorizationRequestNotification {
	PurpleNotification parent;

	PurpleAuthorizationRequest *authorization_request;
};

static void
purple_authorization_request_notification_request_changed_cb(GObject *obj,
                                                             GParamSpec *pspec,
                                                             gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_authorization_request_notification_update(PurpleAuthorizationRequestNotification *auth_notification)
{
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleNotification *notification = PURPLE_NOTIFICATION(auth_notification);
	char *title = NULL;
	const char *alias = NULL;
	const char *username = NULL;

	request = auth_notification->authorization_request;
	account = purple_authorization_request_get_account(request);
	username = purple_authorization_request_get_username(request);
	alias = purple_authorization_request_get_alias(request);

	if(!purple_strempty(alias)) {
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
}

static void
purple_authorization_request_notification_set_request(PurpleAuthorizationRequestNotification *notification,
                                                      PurpleAuthorizationRequest *request)
{
	g_return_if_fail(PURPLE_IS_AUTHORIZATION_REQUEST_NOTIFICATION(notification));

	if(g_set_object(&notification->authorization_request, request)) {
		if(PURPLE_IS_AUTHORIZATION_REQUEST(request)) {
			g_signal_connect_object(request, "notify",
			                        G_CALLBACK(purple_authorization_request_notification_request_changed_cb),
			                        notification, 0);

			purple_authorization_request_notification_update(notification);
		}

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_AUTHORIZATION_REQUEST]);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_authorization_request_notification_request_changed_cb(G_GNUC_UNUSED GObject *obj,
                                                             G_GNUC_UNUSED GParamSpec *pspec,
                                                             gpointer data)
{
	purple_authorization_request_notification_update(data);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleAuthorizationRequestNotification,
                    purple_authorization_request_notification,
                    PURPLE_TYPE_NOTIFICATION)

static void
purple_authorization_request_notification_finalize(GObject *object) {
	PurpleAuthorizationRequestNotification *notification = NULL;

	notification = PURPLE_AUTHORIZATION_REQUEST_NOTIFICATION(object);

	g_clear_object(&notification->authorization_request);

	G_OBJECT_CLASS(purple_authorization_request_notification_parent_class)->finalize(object);
}

static void
purple_authorization_request_notification_get_property(GObject *obj,
                                                       guint param_id,
                                                       GValue *value,
                                                       GParamSpec *pspec)
{
	PurpleAuthorizationRequestNotification *notification = NULL;

	notification = PURPLE_AUTHORIZATION_REQUEST_NOTIFICATION(obj);

	switch(param_id) {
	case PROP_AUTHORIZATION_REQUEST:
		g_value_set_object(value,
		                   purple_authorization_request_notification_get_request(notification));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_authorization_request_notification_set_property(GObject *obj,
                                                       guint param_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
	PurpleAuthorizationRequestNotification *notification = NULL;

	notification = PURPLE_AUTHORIZATION_REQUEST_NOTIFICATION(obj);

	switch(param_id) {
	case PROP_AUTHORIZATION_REQUEST:
		purple_authorization_request_notification_set_request(notification,
		                                                      g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_authorization_request_notification_init(G_GNUC_UNUSED PurpleAuthorizationRequestNotification *notification)
{
}

static void
purple_authorization_request_notification_class_init(PurpleAuthorizationRequestNotificationClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_authorization_request_notification_finalize;
	obj_class->get_property = purple_authorization_request_notification_get_property;
	obj_class->set_property = purple_authorization_request_notification_set_property;

	/**
	 * PurpleAuthorizationRequestNotification:authorization-request:
	 *
	 * The [class@AuthorizationRequest] that this notification was created for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_AUTHORIZATION_REQUEST] = g_param_spec_object(
	    "authorization-request", "authorization-request",
	    "The authorization request this notification is for.",
	    PURPLE_TYPE_AUTHORIZATION_REQUEST,
	    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleNotification *
purple_authorization_request_notification_new(PurpleAuthorizationRequest *request)
{
	PurpleAccount *account = NULL;

	g_return_val_if_fail(PURPLE_IS_AUTHORIZATION_REQUEST(request), NULL);

	account = purple_authorization_request_get_account(request);

	return g_object_new(
	    PURPLE_TYPE_AUTHORIZATION_REQUEST_NOTIFICATION,
	    "account", account,
	    "authorization-request", request,
	    NULL);
}

PurpleAuthorizationRequest *
purple_authorization_request_notification_get_request(PurpleAuthorizationRequestNotification *notification)
{
	g_return_val_if_fail(PURPLE_IS_AUTHORIZATION_REQUEST_NOTIFICATION(notification),
	                     NULL);

	return notification->authorization_request;
}
