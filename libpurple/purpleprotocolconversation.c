/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "purpleprotocolconversation.h"

/******************************************************************************
 * GInterface Implementation
 *****************************************************************************/
G_DEFINE_INTERFACE(PurpleProtocolConversation, purple_protocol_conversation,
                   PURPLE_TYPE_PROTOCOL)

static void
purple_protocol_conversation_default_init(G_GNUC_UNUSED PurpleProtocolConversationInterface *iface)
{
}

/******************************************************************************
 * Public API
 *****************************************************************************/
void
purple_protocol_conversation_send_message_async(PurpleProtocolConversation *protocol,
                                                PurpleConversation *conversation,
                                                PurpleMessage *message,
                                                GCancellable *cancellable,
                                                GAsyncReadyCallback callback,
                                                gpointer data)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));
	g_return_if_fail(PURPLE_IS_MESSAGE(message));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->send_message_async != NULL) {
		iface->send_message_async(protocol, conversation, message, cancellable,
		                          callback, data);
	} else {
		g_warning("%s does not implement send_message_async",
		          G_OBJECT_TYPE_NAME(protocol));
	}
}

gboolean
purple_protocol_conversation_send_message_finish(PurpleProtocolConversation *protocol,
                                                 GAsyncResult *result,
                                                 GError **error)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->send_message_finish != NULL) {
		return iface->send_message_finish(protocol, result, error);
	} else {
		g_warning("%s does not implement send_message_finish",
		          G_OBJECT_TYPE_NAME(protocol));
	}

	return FALSE;
}

void
purple_protocol_conversation_set_topic_async(PurpleProtocolConversation *protocol,
                                             PurpleConversation *conversation,
                                             const char *topic,
                                             GCancellable *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer data)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->set_topic_async != NULL) {
		iface->set_topic_async(protocol, conversation, topic, cancellable,
		                       callback, data);
	} else {
		g_warning("%s does not implement set_topic_async",
		          G_OBJECT_TYPE_NAME(protocol));
	}
}

gboolean
purple_protocol_conversation_set_topic_finish(PurpleProtocolConversation *protocol,
                                              GAsyncResult *result,
                                              GError **error)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->set_topic_finish != NULL) {
		return iface->set_topic_finish(protocol, result, error);
	} else {
		g_warning("%s does not implement set_topic_finish",
		          G_OBJECT_TYPE_NAME(protocol));
	}

	return FALSE;
}
