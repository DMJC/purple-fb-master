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
gboolean
purple_protocol_conversation_implements_create_conversation(PurpleProtocolConversation *protocol)
{
	PurpleProtocolConversation *protocol_conversation = NULL;
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);

	protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);
	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol_conversation);

	if(iface->get_create_conversation_details == NULL) {
		return FALSE;
	}

	if(iface->create_conversation_async == NULL) {
		return FALSE;
	}

	if(iface->create_conversation_finish == NULL) {
		return FALSE;
	}

	return TRUE;
}

PurpleCreateConversationDetails *
purple_protocol_conversation_get_create_conversation_details(PurpleProtocolConversation *protocol,
                                                             PurpleAccount *account)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), NULL);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->get_create_conversation_details != NULL) {
		return iface->get_create_conversation_details(protocol, account);
	}

	g_warning("%s does not implement get_create_conversation_details",
	          G_OBJECT_TYPE_NAME(protocol));

	return NULL;
}

void
purple_protocol_conversation_create_conversation_async(PurpleProtocolConversation *protocol,
                                                       PurpleAccount *account,
                                                       PurpleCreateConversationDetails *details,
                                                       GCancellable *cancellable,
                                                       GAsyncReadyCallback callback,
                                                       gpointer data)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(PURPLE_IS_CREATE_CONVERSATION_DETAILS(details));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->create_conversation_async != NULL) {
		iface->create_conversation_async(protocol, account, details,
		                                 cancellable, callback, data);
	} else {
		GTask *task = NULL;

		task = g_task_new(protocol, cancellable, callback, data);
		g_task_return_new_error(task, PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0,
		                        "%s does not implement create_conversation_async",
		                        G_OBJECT_TYPE_NAME(protocol));
		g_task_set_source_tag(task,
		                      purple_protocol_conversation_create_conversation_async);

		/* details is transfer full and has already been null checked. */
		g_object_unref(details);

		g_clear_object(&task);
	}
}

PurpleConversation *
purple_protocol_conversation_create_conversation_finish(PurpleProtocolConversation *protocol,
                                                        GAsyncResult *result,
                                                        GError **error)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->create_conversation_finish != NULL) {
		return iface->create_conversation_finish(protocol, result, error);
	}

	if(G_IS_TASK(result)) {
		GTask *task = G_TASK(result);
		gpointer source = NULL;

		source = g_task_get_source_tag(task);
		if(source == purple_protocol_conversation_create_conversation_async) {
			return g_task_propagate_pointer(task, error);
		} else {
			g_warning("%s does not implement create_conversation_finish",
			          G_OBJECT_TYPE_NAME(protocol));
		}
	}

	return FALSE;
}

gboolean
purple_protocol_conversation_implements_leave_conversation(PurpleProtocolConversation *protocol)
{
	PurpleProtocolConversation *protocol_conversation = NULL;
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);

	protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);
	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol_conversation);

	if(iface->leave_conversation_async == NULL) {
		return FALSE;
	}

	if(iface->leave_conversation_finish == NULL) {
		return FALSE;
	}

	return TRUE;
}

void
purple_protocol_conversation_leave_conversation_async(PurpleProtocolConversation *protocol,
                                                      PurpleConversation *conversation,
                                                      GCancellable *cancellable,
                                                      GAsyncReadyCallback callback,
                                                      gpointer data)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->leave_conversation_async != NULL) {
		iface->leave_conversation_async(protocol, conversation, cancellable,
		                                callback, data);
	} else {
		GTask *task = NULL;

		task = g_task_new(protocol, cancellable, callback, data);
		g_task_return_new_error(task, PURPLE_PROTOCOL_CONVERSATION_DOMAIN, 0,
		                        "%s does not implement leave_conversation_async",
		                        G_OBJECT_TYPE_NAME(protocol));
		g_task_set_source_tag(task,
		                      purple_protocol_conversation_leave_conversation_async);

		g_clear_object(&task);
	}
}

gboolean
purple_protocol_conversation_leave_conversation_finish(PurpleProtocolConversation *protocol,
                                                       GAsyncResult *result,
                                                       GError **error)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->leave_conversation_finish != NULL) {
		return iface->leave_conversation_finish(protocol, result, error);
	}

	if(G_IS_TASK(result)) {
		GTask *task = G_TASK(result);
		gpointer source = NULL;

		source = g_task_get_source_tag(task);
		if(source == purple_protocol_conversation_leave_conversation_async) {
			return g_task_propagate_boolean(task, error);
		} else {
			g_warning("%s does not implement leave_conversation_finish",
			          G_OBJECT_TYPE_NAME(protocol));
		}
	}

	return FALSE;
}

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
	}

	g_warning("%s does not implement send_message_finish",
	          G_OBJECT_TYPE_NAME(protocol));

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

PurpleChannelJoinDetails *
purple_protocol_conversation_get_channel_join_details(PurpleProtocolConversation *protocol,
                                                      PurpleAccount *account)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), NULL);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->get_channel_join_details != NULL) {
		return iface->get_channel_join_details(protocol, account);
	}

	g_warning("%s does not implement get_channel_join_details",
	          G_OBJECT_TYPE_NAME(protocol));

	return NULL;
}

void
purple_protocol_conversation_join_channel_async(PurpleProtocolConversation *protocol,
                                                PurpleAccount *account,
                                                PurpleChannelJoinDetails *details,
                                                GCancellable *cancellable,
                                                GAsyncReadyCallback callback,
                                                gpointer data)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_ACCOUNT(account));
	g_return_if_fail(PURPLE_IS_CHANNEL_JOIN_DETAILS(details));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->join_channel_async != NULL) {
		iface->join_channel_async(protocol, account, details, cancellable,
		                          callback, data);
	} else {
		g_warning("%s does not implement join_channel_async",
		          G_OBJECT_TYPE_NAME(protocol));
	}
}

gboolean
purple_protocol_conversation_join_channel_finish(PurpleProtocolConversation *protocol,
                                                 GAsyncResult *result,
                                                 GError **error)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->join_channel_finish != NULL) {
		return iface->join_channel_finish(protocol, result, error);
	}

	g_warning("%s does not implement join_channel_finish",
	          G_OBJECT_TYPE_NAME(protocol));

	return FALSE;
}

void
purple_protocol_conversation_set_avatar_async(PurpleProtocolConversation *protocol,
                                              PurpleConversation *conversation,
                                              PurpleAvatar *avatar,
                                              GCancellable *cancellable,
                                              GAsyncReadyCallback callback,
                                              gpointer data)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->set_avatar_async != NULL) {
		iface->set_avatar_async(protocol, conversation, avatar, cancellable,
		                        callback, data);
	} else {
		g_warning("%s does not implement set_avatar_async",
		          G_OBJECT_TYPE_NAME(protocol));
	}
}

gboolean
purple_protocol_conversation_set_avatar_finish(PurpleProtocolConversation *protocol,
                                               GAsyncResult *result,
                                               GError **error)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_val_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol), FALSE);
	g_return_val_if_fail(G_IS_ASYNC_RESULT(result), FALSE);

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->set_avatar_finish != NULL) {
		return iface->set_avatar_finish(protocol, result, error);
	}

	g_warning("%s does not implement set_avatar_finish",
	          G_OBJECT_TYPE_NAME(protocol));

	return FALSE;
}

void
purple_protocol_conversation_send_typing(PurpleProtocolConversation *protocol,
                                         PurpleConversation *conversation,
                                         PurpleTypingState state)
{
	PurpleProtocolConversationInterface *iface = NULL;

	g_return_if_fail(PURPLE_IS_PROTOCOL_CONVERSATION(protocol));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	iface = PURPLE_PROTOCOL_CONVERSATION_GET_IFACE(protocol);
	if(iface != NULL && iface->send_typing != NULL) {
		iface->send_typing(protocol, conversation, state);
	} else {
		g_warning("%s does not implement send_typing",
		          G_OBJECT_TYPE_NAME(protocol));
	}
}
