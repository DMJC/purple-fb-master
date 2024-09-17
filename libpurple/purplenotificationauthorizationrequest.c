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

#include "purplenotificationauthorizationrequest.h"

#include "util.h"

enum {
	PROP_0,
	PROP_AUTHORIZATION_REQUEST,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, 0};

struct _PurpleNotificationAuthorizationRequest {
	PurpleNotification parent;

	PurpleAuthorizationRequest *authorization_request;
};

static void
purple_notification_authorization_request_notify_cb(GObject *obj,
                                                    GParamSpec *pspec,
                                                    gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_notification_authorization_request_update(PurpleNotificationAuthorizationRequest *auth_notification)
{
	PurpleAccount *account = NULL;
	PurpleAuthorizationRequest *request = NULL;
	PurpleContact *contact = NULL;
	PurpleContactInfo *account_info = NULL;
	PurpleContactInfo *contact_info = NULL;
	PurpleNotification *notification = PURPLE_NOTIFICATION(auth_notification);
	char *title = NULL;

	request = auth_notification->authorization_request;
	contact = purple_authorization_request_get_contact(request);
	contact_info = PURPLE_CONTACT_INFO(contact);

	account = purple_contact_get_account(contact);
	account_info = purple_account_get_contact_info(account);

	title = g_strdup_printf(_("%s would like to add %s to their contact list"),
	                        purple_contact_info_get_name_for_display(contact_info),
	                        purple_contact_info_get_name_for_display(account_info));
	purple_notification_set_title(notification, title);
	g_free(title);
}

static void
purple_notification_authorization_request_set_request(PurpleNotificationAuthorizationRequest *notification,
                                                      PurpleAuthorizationRequest *request)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION_AUTHORIZATION_REQUEST(notification));

	if(g_set_object(&notification->authorization_request, request)) {
		PurpleAccount *account = NULL;
		GObject *obj = G_OBJECT(notification);

		g_object_freeze_notify(obj);

		if(PURPLE_IS_AUTHORIZATION_REQUEST(request)) {
			PurpleContact *contact = NULL;

			contact = purple_authorization_request_get_contact(request);
			account = purple_contact_get_account(contact);

			g_signal_connect_object(request, "notify",
			                        G_CALLBACK(purple_notification_authorization_request_notify_cb),
			                        notification, G_CONNECT_DEFAULT);

			g_signal_connect_object(contact, "notify::name-for-display",
			                        G_CALLBACK(purple_notification_authorization_request_notify_cb),
			                        notification, G_CONNECT_DEFAULT);
		}

		purple_notification_set_account(PURPLE_NOTIFICATION(notification),
		                                account);

		g_object_notify_by_pspec(obj, properties[PROP_AUTHORIZATION_REQUEST]);
		g_object_thaw_notify(obj);

		purple_notification_authorization_request_update(notification);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_notification_authorization_request_notify_cb(G_GNUC_UNUSED GObject *obj,
                                                    G_GNUC_UNUSED GParamSpec *pspec,
                                                    gpointer data)
{
	purple_notification_authorization_request_update(data);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleNotificationAuthorizationRequest,
                    purple_notification_authorization_request,
                    PURPLE_TYPE_NOTIFICATION)

static void
purple_notification_authorization_request_constructed(GObject *obj) {
	PurpleAccount *account = NULL;
	PurpleNotification *notification = PURPLE_NOTIFICATION(obj);

	G_OBJECT_CLASS(purple_notification_authorization_request_parent_class)->constructed(obj);

	account = purple_notification_get_account(notification);
	if(!PURPLE_IS_ACCOUNT(account)) {
		PurpleContact *contact = NULL;
		PurpleNotificationAuthorizationRequest *auth_notification = NULL;

		auth_notification = PURPLE_NOTIFICATION_AUTHORIZATION_REQUEST(obj);
		contact = purple_authorization_request_get_contact(auth_notification->authorization_request);
		account = purple_contact_get_account(contact);

		purple_notification_set_account(notification, account);
	}

	purple_notification_set_icon_name(notification,
	                                  "address-book-new-symbolic");
}

static void
purple_notification_authorization_request_finalize(GObject *object) {
	PurpleNotificationAuthorizationRequest *notification = NULL;

	notification = PURPLE_NOTIFICATION_AUTHORIZATION_REQUEST(object);

	g_clear_object(&notification->authorization_request);

	G_OBJECT_CLASS(purple_notification_authorization_request_parent_class)->finalize(object);
}

static void
purple_notification_authorization_request_get_property(GObject *obj,
                                                       guint param_id,
                                                       GValue *value,
                                                       GParamSpec *pspec)
{
	PurpleNotificationAuthorizationRequest *notification = NULL;

	notification = PURPLE_NOTIFICATION_AUTHORIZATION_REQUEST(obj);

	switch(param_id) {
	case PROP_AUTHORIZATION_REQUEST:
		g_value_set_object(value,
		                   purple_notification_authorization_request_get_request(notification));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_notification_authorization_request_set_property(GObject *obj,
                                                       guint param_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
	PurpleNotificationAuthorizationRequest *notification = NULL;

	notification = PURPLE_NOTIFICATION_AUTHORIZATION_REQUEST(obj);

	switch(param_id) {
	case PROP_AUTHORIZATION_REQUEST:
		purple_notification_authorization_request_set_request(notification,
		                                                      g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_notification_authorization_request_init(G_GNUC_UNUSED PurpleNotificationAuthorizationRequest *notification)
{
}

static void
purple_notification_authorization_request_class_init(PurpleNotificationAuthorizationRequestClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->constructed = purple_notification_authorization_request_constructed;
	obj_class->finalize = purple_notification_authorization_request_finalize;
	obj_class->get_property = purple_notification_authorization_request_get_property;
	obj_class->set_property = purple_notification_authorization_request_set_property;

	/**
	 * PurpleNotificationAuthorizationRequest:authorization-request:
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
purple_notification_authorization_request_new(const char *id,
                                              PurpleAuthorizationRequest *request)
{
	g_return_val_if_fail(PURPLE_IS_AUTHORIZATION_REQUEST(request), NULL);

	return g_object_new(
	    PURPLE_TYPE_NOTIFICATION_AUTHORIZATION_REQUEST,
	    "authorization-request", request,
	    "id", id,
	    NULL);
}

PurpleAuthorizationRequest *
purple_notification_authorization_request_get_request(PurpleNotificationAuthorizationRequest *notification)
{
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION_AUTHORIZATION_REQUEST(notification),
	                     NULL);

	return notification->authorization_request;
}
