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

#ifndef PURPLE_NOTIFICATION_ADD_CONTACT_H
#define PURPLE_NOTIFICATION_ADD_CONTACT_H

#include <glib.h>
#include <glib-object.h>

#include "purpleaddcontactrequest.h"
#include "purplenotification.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleNotificationAddContact:
 *
 * A [class@Notification] for [class@AddContactRequest]s.
 *
 * Since: 3.0
 */

#define PURPLE_TYPE_NOTIFICATION_ADD_CONTACT (purple_notification_add_contact_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleNotificationAddContact,
                     purple_notification_add_contact,
                     PURPLE, NOTIFICATION_ADD_CONTACT,
                     PurpleNotification)

/**
 * purple_notification_add_contact_new:
 * @id: (nullable): An identifier for this notification.
 * @request: The add contact request.
 *
 * Creates a new [class@Notification] for @request.
 *
 * Since @request has a [class@Contact] which has a [class@Account],
 * [property@Notification:account] will automatically be set to that.
 *
 * Returns: The new notification.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleNotification *purple_notification_add_contact_new(const char *id, PurpleAddContactRequest *request);

/**
 * purple_notification_add_contact_get_request:
 * @notification: The instance.
 *
 * Gets the [class@AddContactRequest] for @notification.
 *
 * Returns: (transfer none): The add contact request.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAddContactRequest *purple_notification_add_contact_get_request(PurpleNotificationAddContact *notification);

G_END_DECLS

#endif /* PURPLE_NOTIFICATION_ADD_CONTACT_H */
