/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include "purpleircv3protocolconversation.h"

#include "purpleircv3connection.h"

/******************************************************************************
 * PurpleProtocolConversation Implementation
 *****************************************************************************/
static void
purple_ircv3_protocol_conversation_send_message_async(PurpleProtocolConversation *protocol,
                                                      PurpleConversation *conversation,
                                                      PurpleMessage *message,
                                                      GCancellable *cancellable,
                                                      GAsyncReadyCallback callback,
                                                      gpointer data)
{
	PurpleIRCv3Connection *v3_connection = NULL;
	PurpleAccount *account = NULL;
	PurpleConnection *connection = NULL;
	GTask *task = NULL;
	const char *id = NULL;

	account = purple_conversation_get_account(conversation);
	connection = purple_account_get_connection(account);
	v3_connection = PURPLE_IRCV3_CONNECTION(connection);

	id = purple_conversation_get_id(conversation);
	/* TODO: the new message dialog sets the name but not the id and we want to
	 * use the id only, so for now if id is NULL we grab the name.
	 */
	if(purple_strempty(id)) {
		id = purple_conversation_get_name(conversation);
	}

	purple_ircv3_connection_writef(v3_connection, "PRIVMSG %s :%s", id,
	                               purple_message_get_contents(message));

	task = g_task_new(protocol, cancellable, callback, data);
	g_task_return_boolean(task, TRUE);
	g_clear_object(&task);

	/* This will be made conditional when we add echo-message support.
	 * https://ircv3.net/specs/extensions/echo-message
	 */
	purple_conversation_write_message(conversation, message);
}

static gboolean
purple_ircv3_protocol_conversation_send_message_finish(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                                       GAsyncResult *result,
                                                       GError **error)
{
	return g_task_propagate_boolean(G_TASK(result), error);
}

void
purple_ircv3_protocol_conversation_init(PurpleProtocolConversationInterface *iface) {
	iface->send_message_async =
		purple_ircv3_protocol_conversation_send_message_async;
	iface->send_message_finish =
		purple_ircv3_protocol_conversation_send_message_finish;
}
