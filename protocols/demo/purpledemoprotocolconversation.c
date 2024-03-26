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

#include "purpledemoprotocol.h"
#include "purpledemoprotocolconversation.h"

/******************************************************************************
 * PurpleProtocolConversation Implementation
 *****************************************************************************/
typedef struct {
	PurpleConversation *conversation;
	PurpleMessage *message;
} PurpleDemoProtocolIMInfo;

static void
purple_demo_protocol_im_info_free(PurpleDemoProtocolIMInfo *info) {
	g_clear_object(&info->conversation);
	g_clear_object(&info->message);
	g_free(info);
}

static gboolean
purple_demo_protocol_echo_im_cb(gpointer data) {
	PurpleDemoProtocolIMInfo *info = data;
	PurpleMessage *message = NULL;
	PurpleMessageFlags flags;
	const char *who = NULL;

	/* Turn outgoing message back incoming. */
	who = purple_conversation_get_name(info->conversation);

	flags = purple_message_get_flags(info->message);
	flags &= ~PURPLE_MESSAGE_SEND;
	flags |= PURPLE_MESSAGE_RECV;

	message = purple_message_new_incoming(who,
	                                      purple_message_get_contents(info->message),
	                                      flags, 0);

	purple_conversation_write_message(info->conversation, message);
	g_clear_object(&message);

	return G_SOURCE_REMOVE;
}

static void
purple_demo_protocol_send_message_async(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                        PurpleConversation *conversation,
                                        PurpleMessage *message,
                                        GCancellable *cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer data)
{
	GTask *task = NULL;
	const char *who = NULL;

	who = purple_conversation_get_name(conversation);
	if(purple_strempty(who)) {
		who = purple_message_get_recipient(message);
	}

	if(purple_strequal(who, "Echo")) {
		PurpleDemoProtocolIMInfo *info = g_new(PurpleDemoProtocolIMInfo, 1);

		info->conversation = g_object_ref(conversation);
		info->message = g_object_ref(message);

		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
		                purple_demo_protocol_echo_im_cb, info,
		                (GDestroyNotify)purple_demo_protocol_im_info_free);
	} else if(purple_strequal(who, "Aegina")) {
		PurpleDemoProtocolIMInfo *info = g_new(PurpleDemoProtocolIMInfo, 1);
		const char *author = purple_message_get_author(message);
		const char *contents = NULL;

		if(purple_strequal(author, "Hades")) {
			contents = "🫥️";
		} else {
			/* TRANSLATORS: This is a reference to the Cap of Invisibility owned by
			 * various Greek gods, such as Hades, as mentioned. */
			contents = _("Don't tell Hades I have his Cap");
		}

		info->conversation = g_object_ref(conversation);
		info->message = purple_message_new_outgoing(author, who, contents,
		                                            PURPLE_MESSAGE_SEND);

		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, purple_demo_protocol_echo_im_cb,
		                info, (GDestroyNotify)purple_demo_protocol_im_info_free);
	}

	purple_conversation_write_message(conversation, message);

	task = g_task_new(protocol, cancellable, callback, data);
	g_task_return_boolean(task, TRUE);

	g_clear_object(&task);
}

static gboolean
purple_demo_protocol_send_message_finish(G_GNUC_UNUSED PurpleProtocolConversation *protocol,
                                         GAsyncResult *result,
                                         GError **error)
{
	g_return_val_if_fail(G_IS_TASK(result), FALSE);

	return g_task_propagate_boolean(G_TASK(result), error);
}

void
purple_demo_protocol_conversation_init(PurpleProtocolConversationInterface *iface) {
	iface->send_message_async = purple_demo_protocol_send_message_async;
	iface->send_message_finish = purple_demo_protocol_send_message_finish;
}
