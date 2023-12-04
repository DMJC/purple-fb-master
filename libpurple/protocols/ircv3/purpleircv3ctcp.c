/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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

#include <glib/gi18n-lib.h>

#include "purpleircv3ctcp.h"

#include "purpleircv3constants.h"
#include "purpleircv3source.h"

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_ircv3_ctcp_respond(PurpleIRCv3Connection *connection,
                          PurpleMessage *message,
                          const char *response)
{
	const char *author = NULL;
	char *nick = NULL;

	author = purple_message_get_author(message);
	purple_ircv3_source_parse(author, &nick, NULL, NULL);
	if(!purple_strempty(nick)) {
		PurpleContact *contact = NULL;

		contact = purple_ircv3_connection_find_or_create_contact(connection,
		                                                         nick);

		if(PURPLE_IS_CONTACT(contact)) {
			const char *id = NULL;

			id = purple_contact_info_get_id(PURPLE_CONTACT_INFO(contact));

			purple_ircv3_connection_writef(connection, "%s %s :%s",
			                               PURPLE_IRCV3_MSG_NOTICE,
			                               id,
			                               response);
		}
	}

	g_clear_pointer(&nick, g_free);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
purple_ircv3_ctcp_handler(PurpleIRCv3Connection *connection,
                          PurpleConversation *conversation,
                          PurpleMessage *message,
                          const char *command,
                          const char *params,
                          G_GNUC_UNUSED gpointer data)
{
	if(purple_strequal(command, PURPLE_IRCV3_CTCP_ACTION)) {
		purple_message_set_contents(message, params);
		purple_message_set_action(message, TRUE);

		purple_conversation_write_message(conversation, message);

		return TRUE;
	} else if(purple_strequal(command, PURPLE_IRCV3_CTCP_VERSION)) {
		purple_ircv3_ctcp_respond(connection, message, "Purple3 IRCv3");
	}

	return FALSE;
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
gboolean
purple_ircv3_ctcp_handle(PurpleIRCv3Connection *connection,
                         PurpleConversation *conversation,
                         PurpleMessage *message)
{
	PurpleMessageFlags flags;
	char *command = NULL;
	char *params = NULL;
	char *ptr = NULL;
	const char *contents = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail(PURPLE_IRCV3_IS_CONNECTION(connection), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);
	g_return_val_if_fail(PURPLE_IS_MESSAGE(message), FALSE);

	contents = purple_message_get_contents(message);
	if(contents == NULL || contents[0] != PURPLE_IRCV3_CTCP_DELIMITER) {
		return FALSE;
	}

	/* Move past the delimiter. */
	contents += 1;

	/* Find the delimiter for the command. */
	ptr = g_strstr_len(contents, -1, " ");

	/* If we don't have a space, then we have no parameters. */
	if(ptr == NULL) {
		size_t len = strlen(contents);

		command = g_strdup(contents);
		if(command[len - 1] == PURPLE_IRCV3_CTCP_DELIMITER) {
			command[len - 1] = '\0';
		}
	} else {
		size_t len = 0;

		command = g_strndup(contents, ptr - contents);

		params = g_strdup(ptr + 1);
		len = strlen(params);
		if(params[len - 1] == PURPLE_IRCV3_CTCP_DELIMITER) {
			params[len - 1] = '\0';
		}
	}

	flags = purple_message_get_flags(message);
	if(flags & PURPLE_MESSAGE_NOTIFY) {
		ret = purple_ircv3_connection_emit_ctcp_response(connection,
		                                                 conversation, message,
		                                                 command, params);
	} else {
		ret = purple_ircv3_connection_emit_ctcp_request(connection, conversation,
		                                                message, command,
		                                                params);
	}

	g_clear_pointer(&command, g_free);
	g_clear_pointer(&params, g_free);

	return ret;
}

void
purple_ircv3_ctcp_add_default_handlers(PurpleIRCv3Connection *connection) {
	g_signal_connect(connection, "ctcp-request",
	                 G_CALLBACK(purple_ircv3_ctcp_handler), NULL);
}
