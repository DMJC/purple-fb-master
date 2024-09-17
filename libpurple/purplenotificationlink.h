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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_NOTIFICATION_LINK_H
#define PURPLE_NOTIFICATION_LINK_H

#include <glib.h>
#include <glib-object.h>

#include "purplenotification.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleNotificationLink:
 *
 * A [class@Notification] that contains a link for more information.
 *
 * Since: 3.0
 */

#define PURPLE_TYPE_NOTIFICATION_LINK (purple_notification_link_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleNotificationLink, purple_notification_link,
                     PURPLE, NOTIFICATION_LINK, PurpleNotification)

/**
 * purple_notification_link_new:
 * @id: (nullable): An identifier for this notification.
 * @title: The title for the notification.
 * @link_text: (nullable): The text to display for the link.
 * @link_uri: The URI of the link.
 *
 * Creates a new [class@Notification] with a link to @link_uri.
 *
 * Returns: The new notification.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleNotification *purple_notification_link_new(const char *id, const char *title, const char *link_text, const char *link_uri);

/**
 * purple_notification_link_get_link_text:
 * @notification: The instance.
 *
 * The link text if set otherwise, the value of
 * [property@NotificationLink:link-uri].
 *
 * Returns: The link text if set, otherwise the link URI.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_notification_link_get_link_text(PurpleNotificationLink *notification);

/**
 * purple_notification_link_get_link_uri:
 * @notification: The instance.
 *
 * The link URI.
 *
 * Returns: The link URI.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_notification_link_get_link_uri(PurpleNotificationLink *notification);

G_END_DECLS

#endif /* PURPLE_NOTIFICATION_LINK_H */
