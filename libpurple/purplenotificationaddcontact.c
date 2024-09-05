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

#include "purplenotificationaddcontact.h"

struct _PurpleNotificationAddContact {
	PurpleNotification parent;

	PurpleAddContactRequest *request;
};

enum {
	PROP_0,
	PROP_REQUEST,
	N_PROPERTIES,
};
static GParamSpec *properties[N_PROPERTIES] = {NULL, };

static void
purple_notification_add_contact_contact_notify_cb(GObject *obj,
                                                  GParamSpec *pspec,
                                                  gpointer data);
static void
purple_notification_add_contact_request_notify_cb(GObject *obj,
                                                  GParamSpec *pspec,
                                                  gpointer data);

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_notification_add_contact_update(PurpleNotificationAddContact *notification)
{
	PurpleAccount *account = NULL;
	PurpleContact *remote = NULL;
	PurpleContactInfo *info = NULL;
	char *title = NULL;

	g_return_if_fail(PURPLE_IS_NOTIFICATION_ADD_CONTACT(notification));

	remote = purple_add_contact_request_get_contact(notification->request);
	account = purple_contact_get_account(remote);
	info = purple_account_get_contact_info(account);

	title = g_strdup_printf(_("%s added %s to their contact_list"),
	                        purple_contact_info_get_name_for_display(PURPLE_CONTACT_INFO(remote)),
	                        purple_contact_info_get_name_for_display(info));
	purple_notification_set_title(PURPLE_NOTIFICATION(notification), title);
	g_free(title);
}

static void
purple_notification_add_contact_set_request(PurpleNotificationAddContact *notification,
                                            PurpleAddContactRequest *request)
{
	g_return_if_fail(PURPLE_IS_NOTIFICATION_ADD_CONTACT(notification));

	if(g_set_object(&notification->request, request)) {
		if(PURPLE_IS_ADD_CONTACT_REQUEST(request)) {
			PurpleContact *contact = NULL;

			contact = purple_add_contact_request_get_contact(request);
			g_signal_connect_object(contact, "notify",
			                        G_CALLBACK(purple_notification_add_contact_contact_notify_cb),
			                        notification, G_CONNECT_DEFAULT);

			/* We need to know when the contact or the message changes. */
			g_signal_connect_object(request, "notify",
			                        G_CALLBACK(purple_notification_add_contact_request_notify_cb),
			                        notification, G_CONNECT_DEFAULT);
		}

		g_object_notify_by_pspec(G_OBJECT(notification),
		                         properties[PROP_REQUEST]);

		purple_notification_add_contact_update(notification);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
purple_notification_add_contact_contact_notify_cb(G_GNUC_UNUSED GObject *obj,
                                                  G_GNUC_UNUSED GParamSpec *pspec,
                                                  gpointer data)
{
	purple_notification_add_contact_update(data);
}

static void
purple_notification_add_contact_request_notify_cb(G_GNUC_UNUSED GObject *obj,
                                                  G_GNUC_UNUSED GParamSpec *pspec,
                                                  gpointer data)
{
	purple_notification_add_contact_update(data);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_FINAL_TYPE(PurpleNotificationAddContact,
                    purple_notification_add_contact, PURPLE_TYPE_NOTIFICATION)

static void
purple_notification_add_contact_finalize(GObject *obj) {
	PurpleNotificationAddContact *notification = NULL;

	notification = PURPLE_NOTIFICATION_ADD_CONTACT(obj);

	g_clear_object(&notification->request);

	G_OBJECT_CLASS(purple_notification_add_contact_parent_class)->finalize(obj);
}

static void
purple_notification_add_contact_get_property(GObject *obj, guint param_id,
                                             GValue *value, GParamSpec *pspec)
{
	PurpleNotificationAddContact *notification = NULL;

	notification = PURPLE_NOTIFICATION_ADD_CONTACT(obj);

	switch(param_id) {
	case PROP_REQUEST:
		g_value_set_object(value,
		                   purple_notification_add_contact_get_request(notification));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_notification_add_contact_set_property(GObject *obj, guint param_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
	PurpleNotificationAddContact *notification = NULL;

	notification = PURPLE_NOTIFICATION_ADD_CONTACT(obj);

	switch(param_id) {
	case PROP_REQUEST:
		purple_notification_add_contact_set_request(notification,
		                                            g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, param_id, pspec);
		break;
	}
}

static void
purple_notification_add_contact_init(PurpleNotificationAddContact *notification)
{
	purple_notification_set_icon_name(PURPLE_NOTIFICATION(notification),
	                                  "contact-new-symbolic");
}

static void
purple_notification_add_contact_class_init(PurpleNotificationAddContactClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = purple_notification_add_contact_finalize;
	obj_class->get_property = purple_notification_add_contact_get_property;
	obj_class->set_property = purple_notification_add_contact_set_property;

	/**
	 * PurpleNotificationAddContact:request:
	 *
	 * The [class@AddContactRequest] that this notification is for.
	 *
	 * Since: 3.0
	 */
	properties[PROP_REQUEST] = g_param_spec_object(
		"request", NULL, NULL,
		PURPLE_TYPE_ADD_CONTACT_REQUEST,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(obj_class, N_PROPERTIES, properties);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
PurpleNotification *
purple_notification_add_contact_new(PurpleAddContactRequest *request) {
	g_return_val_if_fail(PURPLE_IS_ADD_CONTACT_REQUEST(request), NULL);

	return g_object_new(
		PURPLE_TYPE_NOTIFICATION_ADD_CONTACT,
		"request", request,
		NULL);
}

PurpleAddContactRequest *
purple_notification_add_contact_get_request(PurpleNotificationAddContact *notification)
{
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION_ADD_CONTACT(notification),
	                     NULL);

	return notification->request;
}
