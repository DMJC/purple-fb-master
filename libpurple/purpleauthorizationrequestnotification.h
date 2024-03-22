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

#ifndef PURPLE_AUTHORIZATION_REQUEST_NOTIFICATION_H
#define PURPLE_AUTHORIZATION_REQUEST_NOTIFICATION_H

#include <glib.h>
#include <glib-object.h>

#include "purpleauthorizationrequest.h"
#include "purplenotification.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleAuthorizationRequestNotification:
 *
 * A [class@Notification] for [class@AuthorizationRequest]s.
 *
 * Since: 3.0
 */

#define PURPLE_TYPE_AUTHORIZATION_REQUEST_NOTIFICATION (purple_authorization_request_notification_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleAuthorizationRequestNotification,
                     purple_authorization_request_notification,
                     PURPLE, AUTHORIZATION_REQUEST_NOTIFICATION,
                     PurpleNotification)

/**
 * purple_authorization_request_notification_new:
 * @request: The authorization request instance.
 *
 * Creates a new [class@Notification] for @request.
 *
 * Returns: The new notification.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleNotification *purple_authorization_request_notification_new(PurpleAuthorizationRequest *request);

/**
 * purple_authorization_request_notification_get_request:
 * @notification: The instance.
 *
 * Gets the [class@AuthorizationRequest] for @notification.
 *
 * Returns: (transfer none): The authorization request.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleAuthorizationRequest *purple_authorization_request_notification_get_request(PurpleAuthorizationRequestNotification *notification);

G_END_DECLS

#endif /* PURPLE_AUTHORIZATION_REQUEST_NOTIFICATION_H */
