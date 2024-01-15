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

#ifndef PURPLE_IRCV3_FORMATTING_H
#define PURPLE_IRCV3_FORMATTING_H

#include <glib.h>

#include "purpleircv3version.h"

G_BEGIN_DECLS

/**
 * purple_ircv3_formatting_strip:
 * @text: (nullable): The text to strip.
 *
 * Removes all formatting from @text and returns the result.
 *
 * Returns: (transfer full) (nullable): The result of stripping @text.
 *
 * Since: 3.0.0
 */
PURPLE_IRCV3_AVAILABLE_IN_ALL
char *purple_ircv3_formatting_strip(const char *text);

G_END_DECLS

#endif /* PURPLE_IRCV3_FORMATTING_H */
