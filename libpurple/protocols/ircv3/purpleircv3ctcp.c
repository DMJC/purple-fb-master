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
