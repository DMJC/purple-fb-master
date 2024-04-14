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

/* This file is the fullcrap */

#include <glib/gi18n-lib.h>

#include "buddylist.h"
#include "conversations.h"
#include "debug.h"
#include "notify.h"
#include "prefs.h"
#include "purpleconversation.h"
#include "purpleconversationmanager.h"
#include "purpleprivate.h"
#include "purpleprotocol.h"
#include "purpleprotocolserver.h"
#include "request.h"
#include "signals.h"
#include "server.h"
#include "util.h"

/*
 * Move a buddy from one group to another on server.
 *
 * Note: For now we'll not deal with changing gc's at the same time, but
 * it should be possible.  Probably needs to be done, someday.  Although,
 * the UI for that would be difficult, because groups are Purple-wide.
 */
void purple_serv_move_buddy(PurpleBuddy *buddy, PurpleGroup *orig, PurpleGroup *dest)
{
	PurpleAccount *account;
	PurpleConnection *gc;
	PurpleProtocol *protocol;

	g_return_if_fail(buddy != NULL);
	g_return_if_fail(orig != NULL);
	g_return_if_fail(dest != NULL);

	account = purple_buddy_get_account(buddy);
	gc = purple_account_get_connection(account);

	if (gc) {
		protocol = purple_connection_get_protocol(gc);
		purple_protocol_server_group_buddy(PURPLE_PROTOCOL_SERVER(protocol),
		                                   gc, purple_buddy_get_name(buddy),
		                                   purple_group_get_name(orig),
		                                   purple_group_get_name(dest));
	}
}
