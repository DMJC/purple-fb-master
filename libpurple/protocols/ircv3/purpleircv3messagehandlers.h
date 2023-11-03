/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
#if !defined(PURPLE_IRCV3_GLOBAL_HEADER_INSIDE) && \
    !defined(PURPLE_IRCV3_COMPILATION)
# error "only <libpurple/protocols/ircv3.h> may be included directly"
#endif

#ifndef PURPLE_IRCV3_MESSAGE_HANDLERS_H
#define PURPLE_IRCV3_MESSAGE_HANDLERS_H

#include <glib.h>

#include <purple.h>

#include "purpleircv3message.h"
#include "purpleircv3version.h"

G_BEGIN_DECLS

/**
 * PurpleIRCv3MessageHandler:
 * @message: The [class@Message] to handle.
 * @error: (nullable): A return address for a [type@GLib.Error].
 * @data: The user data that was passed to [method@PurpleIRCv3.Parser.parse].
 *
 * Defines the type of a function that will be called to handle @message.
 *
 * Returns: %TRUE if the command was handled properly, otherwise %FALSE and
 *          @error may be set.
 *
 * Since: 3.0.0
 */
PURPLE_IRCV3_AVAILABLE_TYPE_IN_3_0
typedef gboolean (*PurpleIRCv3MessageHandler)(PurpleIRCv3Message *message,
                                              GError **error,
                                              gpointer data);

G_GNUC_INTERNAL gboolean purple_ircv3_message_handler_fallback(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_message_handler_status(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_message_handler_status_ignore_param0(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_message_handler_ping(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_message_handler_privmsg(PurpleIRCv3Message *message, GError **error, gpointer data);

G_END_DECLS

#endif /* PURPLE_IRCV3_MESSAGE_HANDLERS_H */
