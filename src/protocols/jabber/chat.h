/**
 * @file chat.h Chat stuff
 *
 * gaim
 *
 * Copyright (C) 2003 Nathan Walp <faceprint@faceprint.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _GAIM_JABBER_CHAT_H_
#define _GAIM_JABBER_CHAT_H_

#include "internal.h"
#include "connection.h"
#include "conversation.h"

#include "jabber.h"


typedef struct _JabberChat {
	JabberStream *js;
	char *room;
	char *server;
	char *nick;
	int id;
	GaimConversation *conv;
	gboolean muc;
	gboolean xhtml;
} JabberChat;

GList *jabber_chat_info(GaimConnection *gc);
void jabber_chat_join(GaimConnection *gc, GHashTable *data);
JabberChat *jabber_chat_find(JabberStream *js, const char *room,
		const char *server);
JabberChat *jabber_chat_find_by_id(JabberStream *js, int id);
void jabber_chat_destroy(JabberChat *chat);
gboolean jabber_chat_find_buddy(GaimConversation *conv, const char *name);
void jabber_chat_whisper(GaimConnection *gc, int id, const char *who,
		const char *message);
void jabber_chat_invite(GaimConnection *gc, int id, const char *message,
		const char *name);
void jabber_chat_leave(GaimConnection *gc, int id);

#endif /* _GAIM_JABBER_CHAT_H_ */
