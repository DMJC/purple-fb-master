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

#ifndef PIDGIN_NOTIFICATION_LINK_H
#define PIDGIN_NOTIFICATION_LINK_H

#include <glib.h>

#include <gtk/gtk.h>

#include <purple.h>

#include "pidginnotification.h"
#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginNotificationLink:
 *
 * #PidginNotificationLink is a widget that displays
 * notifications from [class@Purple.NotificationManager] for
 * [class@Purple.NotificationLink].
 *
 * Since: 3.0
 */

#define PIDGIN_TYPE_NOTIFICATION_LINK (pidgin_notification_link_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginNotificationLink, pidgin_notification_link,
                     PIDGIN, NOTIFICATION_LINK, PidginNotification)

/**
 * pidgin_notification_link_new:
 * @notification: A [class@Purple.Notification] to display.
 *
 * Creates a new #PidginNotificationLink instance that will display
 * @notification.
 *
 * Returns: (transfer full): The new #PidginNotificationLink instance.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_notification_link_new(PurpleNotification *notification);

G_END_DECLS

#endif /* PIDGIN_NOTIFICATION_LINK_H */
