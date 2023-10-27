/**
 * @file gg.h
 *
 * purple
 *
 * Copyright (C) 2005  Bartosz Oler <bartosz@bzimage.us>
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

#ifndef PURPLE_GG_GG_H
#define PURPLE_GG_GG_H

#define GGP_UIN_LEN_MAX 10

#include <gmodule.h>
#include <libgadu.h>
#include <libsoup/soup.h>

#include "search.h"

#include <purple.h>

#include "image-prpl.h"
#include "avatar.h"
#include "roster.h"
#include "multilogon.h"
#include "status.h"
#include "chat.h"
#include "message-prpl.h"
#include "edisc.h"

#define GGP_TYPE_PROTOCOL (ggp_protocol_get_type())

G_MODULE_EXPORT
G_DECLARE_FINAL_TYPE(GGPProtocol, ggp_protocol, GGP, PROTOCOL, PurpleProtocol)

typedef struct {
	struct gg_session *session;
	SoupSession *http;
	guint inpa;

	gchar *imtoken;
	gboolean imtoken_warned;

	ggp_image_session_data *image_data;
	ggp_avatar_session_data *avatar_data;
	ggp_roster_session_data roster_data;
	ggp_multilogon_session_data *multilogon_data;
	ggp_status_session_data *status_data;
	ggp_chat_session_data *chat_data;
	ggp_message_session_data *message_data;
	ggp_edisc_session_data *edisc_data;
} GGPInfo;

typedef struct
{
	gboolean blocked;
	gboolean not_a_friend;
} ggp_buddy_data;

ggp_buddy_data * ggp_buddy_get_data(PurpleBuddy *buddy);

const gchar * ggp_get_imtoken(PurpleConnection *gc);

uin_t ggp_own_uin(PurpleConnection *gc);

void ggp_async_login_handler(gpointer _gc, gint fd, PurpleInputCondition cond);

#endif /* PURPLE_GG_GG_H */
