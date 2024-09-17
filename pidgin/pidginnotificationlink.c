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

#include "pidgin/pidginnotificationlink.h"

struct _PidginNotificationLink {
	PidginNotification parent;
};

G_DEFINE_FINAL_TYPE(PidginNotificationLink, pidgin_notification_link,
                    PIDGIN_TYPE_NOTIFICATION)

/******************************************************************************
 * Actions
 *****************************************************************************/
static void
pidgin_notification_link_open_uri(GtkWidget *self,
                                  G_GNUC_UNUSED const char *action_name,
                                  G_GNUC_UNUSED GVariant *parameter)
{
	PidginNotification *pidgin_notification = PIDGIN_NOTIFICATION(self);
	PurpleNotification *purple_notification = NULL;
	PurpleNotificationLink *link = NULL;
	PurpleUi *ui = NULL;
	const char *uri = NULL;

	purple_notification = pidgin_notification_get_notification(pidgin_notification);
	purple_notification_set_read(purple_notification, TRUE);

	link = PURPLE_NOTIFICATION_LINK(purple_notification);
	uri = purple_notification_link_get_link_uri(link);

	ui = purple_core_get_ui();
	purple_ui_open_uri(ui, uri, NULL, NULL, NULL);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_notification_link_init(PidginNotificationLink *notification) {
	gtk_widget_init_template(GTK_WIDGET(notification));
}

static void
pidgin_notification_link_class_init(PidginNotificationLinkClass *klass) {
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/notificationlink.ui"
	);

	gtk_widget_class_install_action(widget_class, "uri.open",
	                                NULL,
	                                pidgin_notification_link_open_uri);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_notification_link_new(PurpleNotification *notification) {
	g_return_val_if_fail(PURPLE_IS_NOTIFICATION_LINK(notification),
	                     NULL);

	return g_object_new(
		PIDGIN_TYPE_NOTIFICATION_LINK,
		"notification", notification,
		NULL);
}
