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

#include <gtk/gtk.h>

#include <pidgin.h>

/******************************************************************************
 * get_html Helpers
 *****************************************************************************/
static void
test_pidgin_text_buffer_get_html_helper(const char *markup,
                                        const char *expected)
{
	GtkTextBuffer *buffer = NULL;
	char *actual = NULL;

	buffer = gtk_text_buffer_new(NULL);

	if(!purple_strempty(markup)) {
		GtkTextIter start;

		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_insert_markup(buffer, &start, markup, -1);
	}

	actual = pidgin_text_buffer_get_html(buffer);
	g_assert_cmpstr(actual, ==, expected);

	g_clear_pointer(&actual, g_free);
	g_clear_object(&buffer);
}

/******************************************************************************
 * get_html Tests
 *****************************************************************************/
static void
test_pidgin_text_buffer_get_html_empty_string(void) {
	test_pidgin_text_buffer_get_html_helper(NULL, "");
}

static void
test_pidgin_text_buffer_get_html_plain_text(void) {
	test_pidgin_text_buffer_get_html_helper("the quick brown fox...",
	                                        "the quick brown fox...");
}

static void
test_pidgin_text_buffer_get_html_simple_nested_tags(void) {
	const char *markup = "<b><i>wat?!</i></b>";

	test_pidgin_text_buffer_get_html_helper(markup, markup);
}

static void
test_pidgin_text_buffer_get_html_simple_tags(void) {
	/* Using metasyntactic variables because they're easy or something... */
	const char *markup = "foo <b>bar</b> <i>baz</i> <u>qux</u> <s>quux</s> corge";

	test_pidgin_text_buffer_get_html_helper(markup, markup);
}

/******************************************************************************
 * get_html_range Helpers
 *****************************************************************************/
static void
test_pidgin_text_buffer_get_html_range_helper(const char *markup,
                                              const char *expected,
                                              int start_offset,
                                              int end_offset)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start;
	GtkTextIter end;
	char *actual = NULL;

	buffer = gtk_text_buffer_new(NULL);

	gtk_text_buffer_get_start_iter(buffer, &start);

	if(!purple_strempty(markup)) {
		gtk_text_buffer_insert_markup(buffer, &start, markup, -1);
	}

	/* Now update the iterators to point to the requested locations. Set end
	 * first, as we can just copy the original start instead of asking for it
	 * again.
	 */
	end = start;
	gtk_text_iter_set_offset(&end, end_offset);

	gtk_text_iter_set_offset(&start, start_offset);

	/* Now get the HTML. */
	actual = pidgin_text_buffer_get_html_range(buffer, &start, &end);
	g_assert_cmpstr(actual, ==, expected);

	g_clear_pointer(&actual, g_free);
	g_clear_object(&buffer);
}

/******************************************************************************
 * get_html_range Tests
 *****************************************************************************/
static void
test_pidgin_text_buffer_get_html_range_plain_text(void) {
	test_pidgin_text_buffer_get_html_range_helper("the quick brown fox...",
	                                              "quick brown",
	                                               4, 15);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);

	gtk_init();

	g_test_add_func("/pidgin/text-buffer/get-html/empty-string",
	                test_pidgin_text_buffer_get_html_empty_string);
	g_test_add_func("/pidgin/text-buffer/get-html/plain-text",
	                test_pidgin_text_buffer_get_html_plain_text);
	g_test_add_func("/pidgin/text-buffer/get-html/simple-nested",
	                test_pidgin_text_buffer_get_html_simple_nested_tags);
	g_test_add_func("/pidgin/text-buffer/get-html/simple-tags",
	                test_pidgin_text_buffer_get_html_simple_tags);

	g_test_add_func("/pidgin/text-buffer/get-html-range/plain-text",
	                test_pidgin_text_buffer_get_html_range_plain_text);

	return g_test_run();
}
