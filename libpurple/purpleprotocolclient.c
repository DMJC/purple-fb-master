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

#include "purpleprotocolclient.h"

#include "purpleconversation.h"

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_INTERFACE(PurpleProtocolClient, purple_protocol_client,
                   PURPLE_TYPE_PROTOCOL)

static void
purple_protocol_client_default_init(G_GNUC_UNUSED PurpleProtocolClientInterface *iface)
{
}

/******************************************************************************
 * Public API
 *****************************************************************************/
void
purple_protocol_client_convo_closed(PurpleProtocolClient *client,
                                    PurpleConnection *connection,
                                    const gchar *who)
{
	PurpleProtocolClientInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CLIENT(client));
	g_return_if_fail(PURPLE_IS_CONNECTION(connection));
	g_return_if_fail(who != NULL);

	iface = PURPLE_PROTOCOL_CLIENT_GET_IFACE(client);
	if(iface != NULL && iface->convo_closed != NULL) {
		iface->convo_closed(client, connection, who);
	}
}

const gchar *
purple_protocol_client_normalize(PurpleProtocolClient *client,
                                 PurpleAccount *account, const gchar *who)
{
	PurpleProtocolClientInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CLIENT(client), NULL);
	g_return_val_if_fail(who != NULL, NULL);

	iface = PURPLE_PROTOCOL_CLIENT_GET_IFACE(client);
	if(iface != NULL && iface->normalize != NULL) {
		return iface->normalize(client, account, who);
	}

	return NULL;
}
