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

#if !defined(PURPLE_IRCV3_GLOBAL_HEADER_INSIDE) && \
    !defined(PURPLE_IRCV3_COMPILATION)
# error "only <libpurple/protocols/ircv3.h> may be included directly"
#endif

#ifndef PURPLE_IRCV3_CTCP_H
#define PURPLE_IRCV3_CTCP_H

#include <glib.h>
#include <glib-object.h>

#include <purple.h>

#include "purpleircv3connection.h"

G_BEGIN_DECLS

/**
 * purple_ircv3_ctcp_handle: (skip)
 * @connection: The connection.
 * @conversation: The conversation.
 * @message: The message.
 *
 * Check if @message is a CTCP message and handles it accordingly.
 *
 * Returns: %TRUE if the message was a CTCP message and %FALSE otherwise.
 *
 * Since: 3.0.0
 */
G_GNUC_INTERNAL
gboolean purple_ircv3_ctcp_handle(PurpleIRCv3Connection *connection, PurpleConversation *conversation, PurpleMessage *message);

/**
 * purple_ircv3_ctcp_add_default_handlers: (skip)
 * @connection: The connection.
 *
 * Adds handlers for the CTCP commands that we support directly.
 *
 * Since: 3.0.0
 */
G_GNUC_INTERNAL
void purple_ircv3_ctcp_add_default_handlers(PurpleIRCv3Connection *connection);

G_END_DECLS

#endif /* PURPLE_IRCV3_CTCP_H */
