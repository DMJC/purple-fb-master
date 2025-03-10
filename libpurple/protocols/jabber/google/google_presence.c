/**
 * Purple is the legal property of its developers, whose names are too numerous
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "internal.h"
#include <purple.h>

#include "google_presence.h"

void jabber_google_presence_incoming(JabberStream *js, const char *user, JabberBuddyResource *jbr)
{
	if (!js->googletalk)
		return;
	if (jbr->status && g_str_has_prefix(jbr->status, "♫ ")) {
		purple_protocol_got_user_status(purple_connection_get_account(js->gc), user, "tune",
					    PURPLE_TUNE_TITLE, jbr->status + strlen("♫ "), NULL);
		g_free(jbr->status);
		jbr->status = NULL;
	} else {
		purple_protocol_got_user_status_deactive(purple_connection_get_account(js->gc), user, "tune");
	}
}

char *jabber_google_presence_outgoing(PurpleStatus *tune)
{
	const char *attr = purple_status_get_attr_string(tune, PURPLE_TUNE_TITLE);
	return attr ? g_strdup_printf("♫ %s", attr) : g_strdup("");
}
