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

#include <purple.h>

#include "pidgintextbuffer.h"

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
pidgin_text_buffer_get_html_convert_tag(GtkTextTag *tag, gboolean start,
                                        GString *output)
{
	/* In practice gtk_text_buffer_insert_markup seems to create a single tag
	 * for each attribute, but we'll check everything anyways to be try and
	 * work with any buffer.
	 */
	gboolean strikethrough;
	gboolean strikethrough_set;
	PangoStyle style;
	gboolean style_set;
	PangoUnderline underline;
	gboolean underline_set;
	int weight;
	gboolean weight_set;

	const char *format = "<%s>";

	if(!start) {
		format = "</%s>";
	}

	g_object_get(
		G_OBJECT(tag),
		"strikethrough", &strikethrough,
		"strikethrough-set", &strikethrough_set,
		"style", &style,
		"style-set", &style_set,
		"underline", &underline,
		"underline-set", &underline_set,
		"weight", &weight,
		"weight-set", &weight_set,
		NULL);

	if(style_set && style != PANGO_STYLE_NORMAL) {
		g_string_append_printf(output, format, "i");
	}

	if(weight_set && weight > PANGO_WEIGHT_NORMAL) {
		g_string_append_printf(output, format, "b");
	}

	if(underline_set && underline != PANGO_UNDERLINE_NONE) {
		g_string_append_printf(output, format, "u");
	}

	if(strikethrough_set && strikethrough) {
		g_string_append_printf(output, format, "s");
	}
}

static void
pidgin_text_buffer_get_html_start_tag(gpointer data, gpointer user_data) {
	pidgin_text_buffer_get_html_convert_tag(data, TRUE, user_data);
}

static void
pidgin_text_buffer_get_html_process_start_tags(GSList *tags, GString *output) {
	g_slist_foreach(tags, pidgin_text_buffer_get_html_start_tag, output);
}

static void
pidgin_text_buffer_get_html_end_tag(gpointer data, gpointer user_data) {
	pidgin_text_buffer_get_html_convert_tag(data, FALSE, user_data);
}

static void
pidgin_text_buffer_get_html_process_end_tags(GSList *tags, GString *output) {
	g_slist_foreach(tags, pidgin_text_buffer_get_html_end_tag, output);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
char *
pidgin_text_buffer_get_html(GtkTextBuffer *buffer) {
	GtkTextIter start;
	GtkTextIter end;

	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), NULL);

	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);

	return pidgin_text_buffer_get_html_range(buffer, &start, &end);
}

char *
pidgin_text_buffer_get_html_range(GtkTextBuffer *buffer, GtkTextIter *start,
                                  GtkTextIter *end)
{
	GSList *tags = NULL;
	GString *output = NULL;
	GtkTextIter real_start;
	GtkTextIter real_end;
	GtkTextIter next;
	GtkTextIter position;
	char *text = NULL;

	g_return_val_if_fail(GTK_IS_TEXT_BUFFER(buffer), NULL);
	g_return_val_if_fail(start != NULL, NULL);
	g_return_val_if_fail(end != NULL, NULL);

	/* Copy start and end to not modify the caller's copies, then make sure
	 * they're ordered correctly.
	 */
	real_start = *start;
	real_end = *end;
	gtk_text_iter_order(&real_start, &real_end);

	/* Create our iterators that we use to walk the text. */
	position = real_start;
	next = real_start;

	output = g_string_new("");

	/* Process the tags at our starting position. */
	tags = gtk_text_iter_get_tags(&position);
	pidgin_text_buffer_get_html_process_start_tags(tags, output);
	g_clear_slist(&tags, NULL);

	/* Now walk the text looking for tags that are being toggled. */
	while(gtk_text_iter_forward_to_tag_toggle(&next, NULL)) {
		/* If next is past real_end, we need to bail. */
		if(gtk_text_iter_compare(&next, &real_end) >= 0) {
			break;
		}

		/* Append the text in the range. */
		text = gtk_text_buffer_get_text(buffer, &position, &next, TRUE);
		g_string_append(output, text);
		g_clear_pointer(&text, g_free);

		/* Process the closed tags. */
		tags = gtk_text_iter_get_toggled_tags(&next, FALSE);
		pidgin_text_buffer_get_html_process_end_tags(tags, output);
		g_clear_slist(&tags, NULL);

		/* Process the opened tags. */
		tags = gtk_text_iter_get_toggled_tags(&next, TRUE);
		pidgin_text_buffer_get_html_process_start_tags(tags, output);
		g_clear_slist(&tags, NULL);

		/* Increment our iterators. */
		gtk_text_iter_forward_to_tag_toggle(&position, NULL);
	}

	/* Copy the remaining text. */
	text = gtk_text_buffer_get_text(buffer, &position, &real_end, TRUE);
	if(text != NULL) {
		g_string_append(output, text);
		g_clear_pointer(&text, g_free);
	}

	/* Write out any final tags, we reverse them because they seem to be
	 * prepended. This is more of a guess than anything really. */
	tags = gtk_text_iter_get_tags(&position);
	tags = g_slist_reverse(tags);
	pidgin_text_buffer_get_html_process_end_tags(tags, output);
	g_clear_slist(&tags, NULL);

	return g_string_free(output, FALSE);
}
