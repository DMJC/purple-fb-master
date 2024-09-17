/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#include <purple.h>

#include "pidgin/pidginnotificationaddcontact.h"

struct _PidginNotificationAddContact {
	PidginNotification parent;
};

G_DEFINE_FINAL_TYPE(PidginNotificationAddContact,
                    pidgin_notification_add_contact, PIDGIN_TYPE_NOTIFICATION)

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_notification_add_contact_create_dm_cb(GObject *self,
                                             GAsyncResult *result,
                                             G_GNUC_UNUSED gpointer data)
{
	PurpleContact *contact = PURPLE_CONTACT(self);
	PurpleConversation *conversation = NULL;
	GError *error = NULL;

	conversation = purple_contact_create_dm_finish(contact, result, &error);
	if(error != NULL) {
		g_warning("failed to create dm for %s: %s",
		          purple_contact_info_get_name_for_display(PURPLE_CONTACT_INFO(contact)),
		          error->message);

		g_clear_error(&error);
		g_clear_object(&conversation);

		return;
	}

	if(PURPLE_IS_CONVERSATION(conversation)) {
		purple_conversation_present(conversation);
	}

	g_clear_object(&conversation);
}

/******************************************************************************
 * Actions
 *****************************************************************************/
static void
pidgin_notification_add_contact_message(GtkWidget *self,
                                        G_GNUC_UNUSED const char *action_name,
                                        G_GNUC_UNUSED GVariant *parameter)
{
	PidginNotification *pidgin_notification = PIDGIN_NOTIFICATION(self);
	PurpleNotification *purple_notification = NULL;
	PurpleAddContactRequest *request = NULL;
	PurpleContact *contact = NULL;
	PurpleConversation *conversation = NULL;

	purple_notification = pidgin_notification_get_notification(pidgin_notification);
	request = purple_notification_add_contact_get_request(PURPLE_NOTIFICATION_ADD_CONTACT(purple_notification));
	contact = purple_add_contact_request_get_contact(request);

	conversation = purple_contact_find_dm(contact);
	if(PURPLE_IS_CONVERSATION(conversation)) {
		purple_conversation_present(conversation);
	} else {
		purple_contact_create_dm_async(contact, NULL,
		                               pidgin_notification_add_contact_create_dm_cb,
		                               NULL);
	}
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_notification_add_contact_init(PidginNotificationAddContact *notification)
{
	gtk_widget_init_template(GTK_WIDGET(notification));
}

static void
pidgin_notification_add_contact_class_init(PidginNotificationAddContactClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/notificationaddcontact.ui"
	);

#if 0
	/* This is disabled because it uses the request API and we really need to
	 * create a real dialog for this instead.
	 */
	gtk_widget_class_install_action(widget_class, "notification.add-contact",
	                                NULL,
	                                pidgin_notification_add_contact_add_contact);
#endif
	gtk_widget_class_install_action(widget_class, "notification.message",
	                                NULL,
	                                pidgin_notification_add_contact_message);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_notification_add_contact_new(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION_ADD_CONTACT(notification),
	                     NULL);

	return g_object_new(
		PIDGIN_TYPE_NOTIFICATION_ADD_CONTACT,
		"notification", notification,
		NULL);
}
