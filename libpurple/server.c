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

/* This file is the fullcrap */

#include <glib/gi18n-lib.h>

#include "buddylist.h"
#include "conversations.h"
#include "debug.h"
#include "notify.h"
#include "prefs.h"
#include "purpleconversation.h"
#include "purpleconversationmanager.h"
#include "purpleprivate.h"
#include "purpleprotocol.h"
#include "purpleprotocolchat.h"
#include "purpleprotocolim.h"
#include "purpleprotocolserver.h"
#include "request.h"
#include "signals.h"
#include "server.h"
#include "util.h"

unsigned int
purple_serv_send_typing(PurpleConnection *gc, const char *name, PurpleIMTypingState state)
{
	if(gc) {
		PurpleProtocol *protocol = purple_connection_get_protocol(gc);
		PurpleProtocolIM *im = PURPLE_PROTOCOL_IM(protocol);

		return purple_protocol_im_send_typing(im, gc, name, state);
	}

	return 0;
}

int purple_serv_send_im(PurpleConnection *gc, PurpleMessage *msg)
{
	PurpleAccount *account = NULL;
	PurpleConversation *im = NULL;
	PurpleConversationManager *manager = NULL;
	PurpleProtocol *protocol = NULL;
	int val = -EINVAL;
	const gchar *recipient;

	g_return_val_if_fail(gc != NULL, val);
	g_return_val_if_fail(msg != NULL, val);

	protocol = purple_connection_get_protocol(gc);

	g_return_val_if_fail(protocol != NULL, val);
	g_return_val_if_fail(PURPLE_IS_PROTOCOL_IM(protocol), val);

	account  = purple_connection_get_account(gc);
	recipient = purple_message_get_recipient(msg);

	manager = purple_conversation_manager_get_default();
	im = purple_conversation_manager_find_im(manager, account, recipient);

	/* we probably shouldn't be here if the protocol doesn't know how to send
	 * im's... but there was a similar check here before so I just reproduced
	 * it until we can reevaluate this function.
	 */
	if(PURPLE_IS_PROTOCOL_IM(protocol)) {
		PurpleProtocolIM *pim = PURPLE_PROTOCOL_IM(protocol);

		val = purple_protocol_im_send(pim, gc, im, msg);
	}

	if(im && purple_im_conversation_get_send_typed_timeout(PURPLE_IM_CONVERSATION(im)))
		purple_im_conversation_stop_send_typed_timeout(PURPLE_IM_CONVERSATION(im));

	return val;
}

/*
 * Move a buddy from one group to another on server.
 *
 * Note: For now we'll not deal with changing gc's at the same time, but
 * it should be possible.  Probably needs to be done, someday.  Although,
 * the UI for that would be difficult, because groups are Purple-wide.
 */
void purple_serv_move_buddy(PurpleBuddy *buddy, PurpleGroup *orig, PurpleGroup *dest)
{
	PurpleAccount *account;
	PurpleConnection *gc;
	PurpleProtocol *protocol;

	g_return_if_fail(buddy != NULL);
	g_return_if_fail(orig != NULL);
	g_return_if_fail(dest != NULL);

	account = purple_buddy_get_account(buddy);
	gc = purple_account_get_connection(account);

	if (gc) {
		protocol = purple_connection_get_protocol(gc);
		purple_protocol_server_group_buddy(PURPLE_PROTOCOL_SERVER(protocol),
		                                   gc, purple_buddy_get_name(buddy),
		                                   purple_group_get_name(orig),
		                                   purple_group_get_name(dest));
	}
}

void purple_serv_join_chat(PurpleConnection *gc, GHashTable *data)
{
	PurpleProtocol *protocol;

	if (gc) {
		protocol = purple_connection_get_protocol(gc);
		purple_protocol_chat_join(PURPLE_PROTOCOL_CHAT(protocol), gc, data);
	}
}

void purple_serv_chat_invite(PurpleConnection *gc, int id, const char *message, const char *name)
{
	PurpleAccount *account;
	PurpleConversation *chat;
	PurpleConversationManager *manager;
	PurpleProtocol *protocol = NULL;
	char *buffy;

	account = purple_connection_get_account(gc);
	manager = purple_conversation_manager_get_default();
	chat = purple_conversation_manager_find_chat_by_id(manager, account, id);

	if(chat == NULL)
		return;

	if(gc)
		protocol = purple_connection_get_protocol(gc);

	buffy = message && *message ? g_strdup(message) : NULL;
	purple_signal_emit(purple_conversations_get_handle(), "chat-inviting-user",
					 chat, name, &buffy);

	if(protocol) {
		purple_protocol_chat_invite(PURPLE_PROTOCOL_CHAT(protocol), gc, id,
		                            buffy, name);
	}

	purple_signal_emit(purple_conversations_get_handle(), "chat-invited-user",
					 chat, name, buffy);

	g_free(buffy);
}

/* Ya know, nothing uses this except purple_chat_conversation_finalize(),
 * I think I'll just merge it into that later...
 * Then again, something might want to use this, from outside protocol-land
 * to leave a chat without destroying the conversation.
 */
void purple_serv_chat_leave(PurpleConnection *gc, int id)
{
	PurpleProtocol *protocol;

	protocol = purple_connection_get_protocol(gc);
	purple_protocol_chat_leave(PURPLE_PROTOCOL_CHAT(protocol), gc, id);
}

int
purple_serv_chat_send(PurpleConnection *gc, int id, PurpleMessage *msg) {
	PurpleAccount *account = NULL;
	PurpleProtocol *protocol = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *manager = NULL;

	g_return_val_if_fail(msg != NULL, -EINVAL);

	account = purple_connection_get_account(gc);
	protocol = purple_connection_get_protocol(gc);

	manager = purple_conversation_manager_get_default();
	conversation = purple_conversation_manager_find_chat_by_id(manager,
	                                                           account, id);

	if (PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, send)) {
		return purple_protocol_chat_send(PURPLE_PROTOCOL_CHAT(protocol), gc,
		                                 id, conversation, msg);
	}

	return -EINVAL;
}

/*
 * woo. i'm actually going to comment this function. isn't that fun. make
 * sure to follow along, kids
 */
void purple_serv_got_im(PurpleConnection *gc, const char *who, const char *msg,
				 PurpleMessageFlags flags, time_t mtime)
{
	PurpleAccount *account;
	PurpleConversation *im;
	PurpleConversationManager *manager;
	char *message, *name;
	char *angel, *buffy;
	int plugin_return;
	PurpleMessage *pmsg;

	g_return_if_fail(msg != NULL);

	account = purple_connection_get_account(gc);

	if (mtime < 0) {
		purple_debug_error("server",
				"purple_serv_got_im ignoring negative timestamp\n");
		/* TODO: Would be more appropriate to use a value that indicates
		   that the timestamp is unknown, and surface that in the UI. */
		mtime = time(NULL);
	}

	/*
	 * XXX: Should we be setting this here, or relying on protocols to set it?
	 */
	flags |= PURPLE_MESSAGE_RECV;

	manager = purple_conversation_manager_get_default();

	/*
	 * We should update the conversation window buttons and menu,
	 * if it exists.
	 */
	im = purple_conversation_manager_find_im(manager, account, who);

	/*
	 * Make copies of the message and the sender in case plugins want
	 * to free these strings and replace them with a modified version.
	 */
	buffy = g_strdup(msg);
	angel = g_strdup(who);

	plugin_return = GPOINTER_TO_INT(
		purple_signal_emit_return_1(purple_conversations_get_handle(),
								  "receiving-im-msg", account,
								  &angel, &buffy, im, &flags));

	if (!buffy || !angel || plugin_return) {
		g_free(buffy);
		g_free(angel);
		return;
	}

	name = angel;
	message = buffy;

	purple_signal_emit(purple_conversations_get_handle(), "received-im-msg",
	                   account, name, message, im, flags);

	/* search for conversation again in case it was created by received-im-msg handler */
	if(im == NULL) {
		im = purple_conversation_manager_find_im(manager, account, name);
	}

	if(im == NULL) {
		im = purple_im_conversation_new(account, name);
	}

	pmsg = purple_message_new_incoming(name, message, flags, mtime);
	purple_conversation_write_message(im, pmsg);
	g_free(message);
	g_object_unref(pmsg);

	g_free(name);
}
