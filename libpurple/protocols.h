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

#ifndef PURPLE_PROTOCOLS_H
#define PURPLE_PROTOCOLS_H

/**************************************************************************/
/* Basic Protocol Information                                             */
/**************************************************************************/

#include "media.h"
#include "purpleprotocol.h"
#include "purpleversion.h"
#include "status.h"

G_BEGIN_DECLS

/**************************************************************************/
/* Protocol API                                                           */
/**************************************************************************/

/**
 * purple_protocol_change_account_status:
 * @account:    The account the user is on.
 * @old_status: The previous status.
 * @new_status: The status that was activated, or deactivated
 *                   (in the case of independent statuses).
 *
 * Informs the server that our account's status changed.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_change_account_status(PurpleAccount *account,
                                           PurpleStatus *old_status,
                                           PurpleStatus *new_status);

/**
 * purple_protocol_get_statuses:
 * @account: The account the user is on.
 * @presence: The presence for which we're going to get statuses
 *
 * Retrieves the list of stock status types from a protocol.
 *
 * Returns: (transfer full) (element-type PurpleStatus): List of statuses
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GList *purple_protocol_get_statuses(PurpleAccount *account,
                                    PurplePresence *presence);

G_END_DECLS

#endif /* PURPLE_PROTOCOLS_H */
