/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include "purpleircv3ctcphandlers.h"

/******************************************************************************
 * Handlers
 *****************************************************************************/
static void
purple_ircv3_ctcp_handle_version(G_GNUC_UNUSED PurpleIRCv3Connection *connection,
                                 IbisClient *client, IbisMessage *message,
                                 G_GNUC_UNUSED IbisCTCPMessage *ctcp_message)
{
	PurpleUi *ui = NULL;
	IbisMessage *response = NULL;
	IbisCTCPMessage *ctcp_response = NULL;
	char *nick = NULL;
	const char *id = NULL;
	const char *source = NULL;

	source = ibis_message_get_source(message);
	ibis_source_parse(source, &nick, NULL, NULL);

	ui = purple_core_get_ui();
	id = purple_ui_get_id(ui);

	ctcp_response = ibis_ctcp_message_new(IBIS_CTCP_VERSION);
	ibis_ctcp_message_set_params(ctcp_response, id, NULL);

	response = ibis_message_new(IBIS_MSG_NOTICE);
	ibis_message_set_params(response, nick, NULL);
	g_free(nick);

	ibis_message_set_ctcp_message(response, ctcp_response);
	g_clear_object(&ctcp_response);

	ibis_client_write(client, response);
}

static void
purple_ircv3_ctcp_handle_ping(G_GNUC_UNUSED PurpleIRCv3Connection *connection,
                              IbisClient *client, IbisMessage *message,
                              IbisCTCPMessage *ctcp_message)
{
	IbisMessage *response = NULL;
	IbisCTCPMessage *ctcp_response = NULL;
	GStrv params = NULL;
	char *nick = NULL;
	const char *source = NULL;

	ctcp_response = ibis_ctcp_message_new(IBIS_CTCP_PING);
	params = ibis_ctcp_message_get_params(ctcp_message);
	if(params != NULL) {
		ibis_ctcp_message_set_paramsv(ctcp_response, params);
	}

	response = ibis_message_new(IBIS_MSG_NOTICE);

	source = ibis_message_get_source(message);
	ibis_source_parse(source, &nick, NULL, NULL);
	ibis_message_set_params(response, nick, NULL);
	g_free(nick);

	ibis_message_set_ctcp_message(response, ctcp_response);
	g_clear_object(&ctcp_response);

	ibis_client_write(client, response);
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_ircv3_ctcp_handler(PurpleIRCv3Connection *connection,
                          IbisClient *client, IbisMessage *message)
{
	IbisCTCPMessage *ctcp_message = NULL;
	const char *command = NULL;

	ctcp_message = ibis_message_get_ctcp_message(message);
	command = ibis_ctcp_message_get_command(ctcp_message);

	if(purple_strequal(command, IBIS_CTCP_VERSION)) {
		purple_ircv3_ctcp_handle_version(connection, client, message,
		                                 ctcp_message);
	} else if(purple_strequal(command, IBIS_CTCP_PING)) {
		purple_ircv3_ctcp_handle_ping(connection, client, message,
		                              ctcp_message);
	}
}
