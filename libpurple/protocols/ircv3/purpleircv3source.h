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

#ifndef PURPLE_IRCV3_SOURCE_H
#define PURPLE_IRCV3_SOURCE_H

#include <glib.h>
#include <glib-object.h>

#include "purpleircv3version.h"

G_BEGIN_DECLS

/**
 * purple_ircv3_source_parse:
 * @source: The source to parse.
 * @nick: (out) (transfer full): A return address for the nick.
 * @user: (out) (transfer full): A return address for the user.
 * @host: (out) (transfer full): A return address for the host.
 *
 * Parses a `source` string like `pidgy!~u@53unc8n42i868.irc` into its nick,
 * user, and host parts per https://modern.ircdocs.horse/#source.
 *
 * If the user or host aren't present in @source, but a return address is
 * provided for them, that pointer will be set to %NULL.
 *
 * Since: 3.0.0
 */
PURPLE_IRCV3_AVAILABLE_IN_ALL
void purple_ircv3_source_parse(const char *source, char **nick, char **user, char **host);

G_END_DECLS

#endif /* PURPLE_IRCV3_SOURCE_H */
