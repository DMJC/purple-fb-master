/**
 * @file message.h Message handlers
 *
 * purple
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#ifndef PURPLE_JABBER_MESSAGE_H
#define PURPLE_JABBER_MESSAGE_H

#include <purple.h>

#include "jabber.h"

typedef struct {
	JabberStream *js;
	enum {
		JABBER_MESSAGE_NORMAL,
		JABBER_MESSAGE_CHAT,
		JABBER_MESSAGE_GROUPCHAT,
		JABBER_MESSAGE_HEADLINE,
		JABBER_MESSAGE_ERROR,
		JABBER_MESSAGE_GROUPCHAT_INVITE,
		JABBER_MESSAGE_EVENT,
		JABBER_MESSAGE_OTHER
	} type;
	time_t sent;
	gboolean delayed;
	gboolean hasBuzz;
	char *id;
	char *from;
	char *to;
	char *subject;
	char *body;
	char *xhtml;
	char *password;
	char *error;
	char *thread_id;
	enum {
		JM_STATE_NONE,
		JM_STATE_ACTIVE,
		JM_STATE_COMPOSING,
		JM_STATE_PAUSED,
		JM_STATE_INACTIVE,
		JM_STATE_GONE
	} chat_state;
	GList *etc;
	GList *eventitems;
} JabberMessage;

void jabber_message_free(JabberMessage *jm);

void jabber_message_send(JabberMessage *jm);

void jabber_message_parse(JabberStream *js, PurpleXmlNode *packet);
int jabber_message_send_im(PurpleConnection *gc, PurpleMessage *msg);
int jabber_message_send_chat(PurpleConnection *gc, int id, PurpleMessage *msg);

unsigned int jabber_send_typing(PurpleConnection *gc, const char *who, PurpleIMTypingState state);

gboolean jabber_buzz_isenabled(JabberStream *js, const gchar *namespace);

gboolean jabber_custom_smileys_isenabled(JabberStream *js, const gchar *namespace);

#endif /* PURPLE_JABBER_MESSAGE_H */
