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

#include <glib/gi18n-lib.h>

#include <ibis.h>

#include "purpleircv3protocolconversation.h"

#include "purpleircv3connection.h"
#include "purpleircv3core.h"

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
	IbisClient *client = NULL;
	IbisMessage *ibis_message = NULL;
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

	client = purple_ircv3_connection_get_client(v3_connection);

	ibis_message = ibis_message_new(IBIS_MSG_PRIVMSG);
	ibis_message_set_params(ibis_message, id,
	                        purple_message_get_contents(message), NULL);
	ibis_client_write(client, ibis_message);

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

static PurpleChannelJoinDetails *
purple_ircv3_protocol_conversation_get_channel_join_details(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                                            G_GNUC_UNUSED PurpleAccount *account)
{
	return purple_channel_join_details_new(0, FALSE, 0, TRUE, 0);
}

static void
purple_ircv3_protocol_conversation_join_channel_async(PurpleProtocolConversation *protocol,
                                                      PurpleAccount *account,
                                                      PurpleChannelJoinDetails* details,
                                                      GCancellable* cancellable,
                                                      GAsyncReadyCallback callback,
                                                      gpointer data)
{
	PurpleIRCv3Connection *v3_connection = NULL;
	PurpleConnection *connection = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;
	IbisClient *client = NULL;
	IbisMessage *message = NULL;
	GTask *task = NULL;
	const char *name = NULL;
	const char *password = NULL;

	connection = purple_account_get_connection(account);
	v3_connection = PURPLE_IRCV3_CONNECTION(connection);

	task = g_task_new(protocol, cancellable, callback, data);

	/* Validate that the name isn't empty. */
	/* TODO: check that name match the ISUPPORT channel prefixes. */
	name = purple_channel_join_details_get_name(details);
	if(purple_strempty(name)) {
		g_task_return_new_error(task, PURPLE_IRCV3_DOMAIN, 0,
		                        "channel name is empty");
		g_clear_object(&task);

		return;
	}

	manager = purple_conversation_manager_get_default();
	conversation = purple_conversation_manager_find_with_id(manager, account,
	                                                        name);

	/* If the conversation already exists, just return TRUE. */
	if(PURPLE_IS_CONVERSATION(conversation)) {
		g_task_return_boolean(task, TRUE);
		g_clear_object(&task);

		return;
	}

	message = ibis_message_new(IBIS_MSG_JOIN);

	password = purple_channel_join_details_get_password(details);
	if(!purple_strempty(password)) {
		ibis_message_set_params(message, name, password, NULL);
	} else {
		ibis_message_set_params(message, name, NULL);
	}

	conversation = g_object_new(
		PURPLE_TYPE_CONVERSATION,
		"account", account,
		"type", PURPLE_CONVERSATION_TYPE_CHANNEL,
		"id", name,
		"name", name,
		NULL);
	purple_conversation_manager_register(manager, conversation);
	g_clear_object(&conversation);

	client = purple_ircv3_connection_get_client(v3_connection);
	ibis_client_write(client, message);

	g_task_return_boolean(task, TRUE);
	g_clear_object(&task);
}

static gboolean
purple_ircv3_protocol_conversation_join_channel_finish(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                                       GAsyncResult *result,
                                                       GError **error)
{
	return g_task_propagate_boolean(G_TASK(result), error);
}

static void
purple_ircv3_protocol_conversation_send_typing(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                               PurpleConversation *conversation,
                                               PurpleTypingState state)
{
	PurpleIRCv3Connection *connection;
	PurpleAccount *account = NULL;
	PurpleConnection *purple_connection = NULL;
	IbisClient *client = NULL;
	IbisMessage *message = NULL;
	IbisTags *tags = NULL;
	const char *value = NULL;

	if(state == PURPLE_TYPING_STATE_TYPING) {
		value = IBIS_TYPING_ACTIVE;
	} else if(state == PURPLE_TYPING_STATE_PAUSED) {
		value = IBIS_TYPING_PAUSED;
	} else if(state == PURPLE_TYPING_STATE_NONE) {
		value = IBIS_TYPING_DONE;
	}

	if(value == NULL) {
		return;
	}

	account = purple_conversation_get_account(conversation);
	purple_connection = purple_account_get_connection(account);
	connection = PURPLE_IRCV3_CONNECTION(purple_connection);

	message = ibis_message_new(IBIS_MSG_TAGMSG);
	ibis_message_set_params(message, purple_conversation_get_id(conversation),
	                        NULL);

	tags = ibis_tags_new();
	ibis_message_set_tags(message, tags);
	ibis_tags_add(tags, IBIS_TAG_TYPING, value);
	g_clear_object(&tags);

	client = purple_ircv3_connection_get_client(connection);
	ibis_client_write(client, message);
}

void
purple_ircv3_protocol_conversation_init(PurpleProtocolConversationInterface *iface) {
	iface->send_message_async =
		purple_ircv3_protocol_conversation_send_message_async;
	iface->send_message_finish =
		purple_ircv3_protocol_conversation_send_message_finish;

	iface->get_channel_join_details =
		purple_ircv3_protocol_conversation_get_channel_join_details;
	iface->join_channel_async =
		purple_ircv3_protocol_conversation_join_channel_async;
	iface->join_channel_finish =
		purple_ircv3_protocol_conversation_join_channel_finish;

	iface->send_typing  =
		purple_ircv3_protocol_conversation_send_typing;
}
