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

#include "protocols.h"

#include "accounts.h"
#include "debug.h"
#include "network.h"
#include "notify.h"
#include "purpleaccountoption.h"
#include "purpleconversation.h"
#include "purpleconversationmanager.h"
#include "purplecredentialmanager.h"
#include "purpleprotocol.h"
#include "purpleprotocolmanager.h"
#include "purpleprotocolmedia.h"
#include "purpleprotocolserver.h"
#include "request.h"
#include "server.h"
#include "util.h"

/**************************************************************************/
/* Protocol API                                                           */
/**************************************************************************/
static void
do_protocol_change_account_status(PurpleAccount *account,
                                  G_GNUC_UNUSED PurpleStatus *old_status,
                                  PurpleStatus *new_status)
{
	PurpleProtocol *protocol;
	PurpleProtocolManager *manager;

	if (purple_status_is_online(new_status) &&
		purple_account_is_disconnected(account) &&
		purple_network_is_available())
	{
		purple_account_connect(account);
		return;
	}

	if (!purple_status_is_online(new_status))
	{
		if (!purple_account_is_disconnected(account))
			purple_account_disconnect(account);
		/* Clear out the unsaved password if we switch to offline status */
		if (!purple_account_get_remember_password(account)) {
			PurpleCredentialManager *manager = NULL;

			manager = purple_credential_manager_get_default();

			purple_credential_manager_clear_password_async(manager, account,
			                                               NULL, NULL, NULL);
		}

		return;
	}

	if (purple_account_is_connecting(account))
		/*
		 * We don't need to call the set_status protocol function because
		 * the protocol will take care of setting its status during the
		 * connection process.
		 */
		return;

	manager = purple_protocol_manager_get_default();
	protocol = purple_protocol_manager_find(manager,
	                                        purple_account_get_protocol_id(account));

	if (!PURPLE_IS_PROTOCOL_SERVER(protocol)) {
		return;
	}

	if(!purple_account_is_disconnected(account)) {
		purple_protocol_server_set_status(PURPLE_PROTOCOL_SERVER(protocol),
		                                  account, new_status);
	}
}

void
purple_protocol_change_account_status(PurpleAccount *account,
								PurpleStatus *old_status, PurpleStatus *new_status)
{
	g_return_if_fail(account    != NULL);
	g_return_if_fail(new_status != NULL);
	g_return_if_fail(!purple_status_is_exclusive(new_status) || old_status != NULL);

	do_protocol_change_account_status(account, old_status, new_status);

	purple_signal_emit(purple_accounts_get_handle(), "account-status-changed",
					account, old_status, new_status);
}

GList *
purple_protocol_get_statuses(PurpleAccount *account, PurplePresence *presence)
{
	g_return_val_if_fail(account  != NULL, NULL);
	g_return_val_if_fail(presence != NULL, NULL);

	return g_list_copy_deep(purple_account_get_status_types(account),
	                        (GCopyFunc)purple_status_new, presence);
}
