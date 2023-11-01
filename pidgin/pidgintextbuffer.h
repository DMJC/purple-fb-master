/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(PIDGIN_GLOBAL_HEADER_INSIDE) && !defined(PIDGIN_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PIDGIN_TEXT_BUFFER_H
#define PIDGIN_TEXT_BUFFER_H

#include <gtk/gtk.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * pidgin_text_buffer_get_html:
 * @buffer: The text buffer.
 *
 * Extracts text from @buffer as HTML.
 *
 * This will work with any [class@Gtk.TextBuffer] as it deciphers the tags it
 * finds.
 *
 * This is the same as calling [func@text_buffer_get_html_range] with the start
 * and end iterators of @buffer.
 *
 * Returns: (transfer full) (nullable): The HTML string, which could be empty.
 *
 * See Also: [func@text_buffer_get_html_range]
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
char *pidgin_text_buffer_get_html(GtkTextBuffer *buffer);

/**
 * pidgin_text_buffer_get_html_range:
 * @buffer: The text buffer.
 * @start: The start iterator.
 * @end: The end iterator.
 *
 * Extracts the text between @start and @end from @buffer as HTML.
 * This will work with any [class@Gtk.TextBuffer] as it deciphers the tags it
 * finds.
 *
 * Returns: (transfer full) (nullable): The HTML string, which could be empty.
 *
 * See Also: [func@text_buffer_get_html]
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
char *pidgin_text_buffer_get_html_range(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end);

G_END_DECLS

#endif /* PIDGIN_TEXT_BUFFER_H */
