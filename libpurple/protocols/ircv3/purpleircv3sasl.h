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

#ifndef PURPLE_IRCV3_SASL_H
#define PURPLE_IRCV3_SASL_H

#include <glib.h>

#include <purple.h>

#include "purpleircv3capabilities.h"
#include "purpleircv3message.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL void purple_ircv3_sasl_request(PurpleIRCv3Capabilities *capabilities);

G_GNUC_INTERNAL gboolean purple_ircv3_sasl_logged_in(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_logged_out(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_nick_locked(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_success(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_failed(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_message_too_long(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_aborted(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_already_authed(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_mechanisms(PurpleIRCv3Message *message, GError **error, gpointer data);
G_GNUC_INTERNAL gboolean purple_ircv3_sasl_authenticate(PurpleIRCv3Message *message, GError **error, gpointer data);

G_END_DECLS

#endif /* PURPLE_IRCV3_SASL_H */
