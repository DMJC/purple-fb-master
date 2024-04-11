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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_SERVER_H
#define PURPLE_SERVER_H

#include "buddy.h"
#include "group.h"
#include "purpleimconversation.h"
#include "purplemessage.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * purple_serv_send_typing:
 * @gc:    The connection over which to send the typing notification.
 * @name:  The user to send the typing notification to.
 * @state: One of PURPLE_IM_TYPING, PURPLE_IM_TYPED, or PURPLE_IM_NOT_TYPING.
 *
 * Send a typing message to a given user over a given connection.
 *
 * Returns: A quiet-period, specified in seconds, where Purple will not
 *         send any additional typing notification messages.  Most
 *         protocols should return 0, which means that no additional
 *         PURPLE_IM_TYPING messages need to be sent.  If this is 5, for
 *         example, then Purple will wait five seconds, and if the Purple
 *         user is still typing then Purple will send another PURPLE_IM_TYPING
 *         message.
 *
 * Since: 3.0
 */
/* TODO Could probably move this into the conversation API. */
PURPLE_AVAILABLE_IN_3_0
unsigned int purple_serv_send_typing(PurpleConnection *gc, const char *name, PurpleIMTypingState state);

/**
 * purple_serv_move_buddy:
 * @buddy:  The Buddy.
 * @orig:   Original group.
 * @dest:   Destiny group.
 *
 * Move a buddy from one group to another on server.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_serv_move_buddy(PurpleBuddy *buddy, PurpleGroup *orig, PurpleGroup *dest);

/**
 * purple_serv_send_im:
 * @gc:     The connection over which to send the typing notification.
 * @msg:    The message.
 *
 * Sends the message to the user through the required protocol.
 *
 * Returns: The error value returned from the protocol interface function.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
int  purple_serv_send_im(PurpleConnection *gc, PurpleMessage *msg);

/******************************************************************************
 * Chat Interface
 *****************************************************************************/

/**
 * purple_serv_chat_invite
 * @gc:     The connection over which to send the typing notification.
 * @id:     The id of the chat to invite the user to.
 * @message:A message displayed to the user when the invitation.
 * @name:   The name of the remote user to send the invitation to.
 *
 * Invite a user to join a chat.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_serv_chat_invite(PurpleConnection *gc, int id, const char *message, const char *name);

/**
 * purple_serv_chat_leave:
 * @gc:     The connection over which to send the typing notification.
 * @id:     The id of the chat to leave.
 *
 * Called when the user requests leaving a chat.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_serv_chat_leave(PurpleConnection *gc, int id);

/**
 * purple_serv_chat_send:
 * @gc:     The connection over which to send the typing notification.
 * @id:     The id of the chat to send the message to.
 * @msg:    The message to send to the chat.
 *
 * Send a message to a chat.
 *
 * This protocol function should return a positive value on
 * success. If the message is too big to be sent, return
 * <literal>-E2BIG</literal>. If the account is not connected,
 * return <literal>-ENOTCONN</literal>. If the protocol is unable
 * to send the message for another reason, return some other
 * negative value. You can use one of the valid #errno values, or
 * just big something.
 *
 * Returns:  A positive number or 0 in case of success, a
 *           negative error number in case of failure.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
int  purple_serv_chat_send(PurpleConnection *gc, int id, PurpleMessage *msg);

/******************************************************************************
 * Server Interface
 *****************************************************************************/

/**
 * purple_serv_got_im:
 * @gc:     The connection on which the typing message was received.
 * @who:    The username of the buddy that sent the message.
 * @msg:    The actual message received.
 * @flags:  The flags applicable to this message.
 * @mtime:  The timestamp of the message.
 *
 * This function is called by the protocol when it receives an IM message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_serv_got_im(PurpleConnection *gc, const char *who, const char *msg,
				 PurpleMessageFlags flags, time_t mtime);

/**
 * purple_serv_join_chat:
 * @gc:   The #PurpleConnection
 * @data: The hash function should be g_str_hash() and the equal
 *             function should be g_str_equal().
 *
 * Joins the chat described by the components in @data.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_serv_join_chat(PurpleConnection *gc, GHashTable *data);

/**
 * purple_serv_got_chat_left:
 * @g:  The connection on which the chat was left.
 * @id: The id of the chat, as assigned by the protocol.
 *
 * Called by a protocol when an account has left a chat.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_serv_got_chat_left(PurpleConnection *g, int id);

G_END_DECLS

#endif /* PURPLE_SERVER_H */
