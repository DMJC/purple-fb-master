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

#if !defined(PIDGIN_GLOBAL_HEADER_INSIDE) && !defined(PIDGIN_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PIDGIN_NOTIFICATION_H
#define PIDGIN_NOTIFICATION_H

#include <glib.h>

#include <gtk/gtk.h>

#include <adwaita.h>

#include <purple.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginNotification:
 *
 * #PidginNotification is a widget that displays [class@Purple.Notification]'s
 * from [class@Purple.NotificationManager].
 *
 * Depending on the [class@Purple.Notification] that this is created for,
 * different actions will be displayed.
 *
 * Since: 3.0
 */

#define PIDGIN_TYPE_NOTIFICATION (pidgin_notification_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_DERIVABLE_TYPE(PidginNotification, pidgin_notification, PIDGIN,
                         NOTIFICATION, GtkBox)

struct _PidginNotificationClass {
    /*< private >*/
    GtkBoxClass parent;

    /*< private >*/
    gpointer reserved[4];
};

/**
 * pidgin_notification_new:
 * @notification: A [class@Purple.Notification] to display.
 *
 * Creates a new widget that will display @notification.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_notification_new(PurpleNotification *notification);

/**
 * pidgin_notification_get_notification:
 * @notification: The instance.
 *
 * Gets the [class@Purple.Notification] that @notification is displaying.
 *
 * Returns: (transfer none): The notification.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
PurpleNotification *pidgin_notification_get_notification(PidginNotification *notification);

/**
 * pidgin_notification_get_child:
 * @notification: The instance.
 *
 * Gets the child widget from @notification.
 *
 * This child is meant to hold actions that can be performed for this
 * notification.
 *
 * Returns: (transfer none) (nullable): The child widget if set, otherwise
 *          %NULL.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_notification_get_child(PidginNotification *notification);

/**
 * pidgin_notification_set_child:
 * @notification: The instance.
 * @child: (nullable): The new child widget.
 *
 * Sets the child of @notification to @child.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_notification_set_child(PidginNotification *notification, GtkWidget *child);

G_END_DECLS

#endif /* PIDGIN_NOTIFICATION_H */
