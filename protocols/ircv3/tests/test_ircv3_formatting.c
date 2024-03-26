/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include <purple.h>

#include "../purpleircv3formatting.h"

/******************************************************************************
 * Strip Tests
 *****************************************************************************/
static void
test_ircv3_formatting_strip(const char *input, const char *expected) {
	char *actual = NULL;

	actual = purple_ircv3_formatting_strip(input);
	g_assert_cmpstr(actual, ==, expected);
	g_clear_pointer(&actual, g_free);
}

static void
test_ircv3_formatting_strip_null(void) {
	test_ircv3_formatting_strip(NULL, NULL);
}

static void
test_ircv3_formatting_strip_empty(void) {
	test_ircv3_formatting_strip("", "");
}

static void
test_ircv3_formatting_strip_color_comma(void) {
	test_ircv3_formatting_strip("\003,", ",");
}

static void
test_ircv3_formatting_strip_color_foreground_comma(void) {
	test_ircv3_formatting_strip("\0033,", ",");
}

static void
test_ircv3_formatting_strip_color_full(void) {
	test_ircv3_formatting_strip("\0033,9wee", "wee");
}

static void
test_ircv3_formatting_strip_color_foreground_3_digit(void) {
	test_ircv3_formatting_strip("\003314", "4");
}

static void
test_ircv3_formatting_strip_color_background_3_digit(void) {
	test_ircv3_formatting_strip("\0031,234", "4");
}

static void
test_ircv3_formatting_strip_hex_color(void) {
	test_ircv3_formatting_strip("\004FF00FFwoo!", "woo!");
}

static void
test_ircv3_formatting_strip_hex_color_full(void) {
	test_ircv3_formatting_strip("\004FF00FF,00FF00woo!", "woo!");
}

static void
test_ircv3_formatting_strip_hex_color_comma(void) {
	test_ircv3_formatting_strip("\004,", ",");
}

static void
test_ircv3_formatting_strip_hex_color_foreground_comma(void) {
	test_ircv3_formatting_strip("\004FEFEFE,", ",");
}

static void
test_ircv3_formatting_strip_hex_color_foreground_7_characters(void) {
	test_ircv3_formatting_strip("\004FEFEFEF", "F");
}

static void
test_ircv3_formatting_strip_hex_color_background_7_characters(void) {
	test_ircv3_formatting_strip("\004FEFEFE,2222223", "3");
}

static void
test_ircv3_formatting_strip_bold(void) {
	test_ircv3_formatting_strip("this is \002bold\002!", "this is bold!");
}

static void
test_ircv3_formatting_strip_italic(void) {
	test_ircv3_formatting_strip("what do you \035mean\035?!",
	                            "what do you mean?!");
}

static void
test_ircv3_formatting_strip_monospace(void) {
	test_ircv3_formatting_strip("\021i++;\021", "i++;");
}

static void
test_ircv3_formatting_strip_reset(void) {
	test_ircv3_formatting_strip("end of formatting\017", "end of formatting");
}

static void
test_ircv3_formatting_strip_reverse(void) {
	test_ircv3_formatting_strip("re\026ver\026se", "reverse");
}

static void
test_ircv3_formatting_strip_strikethrough(void) {
	test_ircv3_formatting_strip("\036I could be wrong\036",
	                            "I could be wrong");
}

static void
test_ircv3_formatting_strip_underline(void) {
	test_ircv3_formatting_strip("You can't handle the \037truth\037!",
	                            "You can't handle the truth!");
}

static void
test_ircv3_formatting_strip_spec_example1(void) {
	test_ircv3_formatting_strip(
		"I love \0033IRC! \003It is the \0037best protocol ever!",
		"I love IRC! It is the best protocol ever!");
}

static void
test_ircv3_formatting_strip_spec_example2(void) {
	test_ircv3_formatting_strip(
		"This is a \035\00313,9cool \003message",
		"This is a cool message");
}

static void
test_ircv3_formatting_strip_spec_example3(void) {
	test_ircv3_formatting_strip(
		"IRC \002is \0034,12so \003great\017!",
		"IRC is so great!");
}

static void
test_ircv3_formatting_strip_spec_example4(void) {
	test_ircv3_formatting_strip(
		"Rules: Don't spam 5\00313,8,6\003,7,8, and especially not \0029\002\035!",
		"Rules: Don't spam 5,6,7,8, and especially not 9!");
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/ircv3/formatting/strip/null",
	                test_ircv3_formatting_strip_null);
	g_test_add_func("/ircv3/formatting/strip/empty",
	                test_ircv3_formatting_strip_empty);

	g_test_add_func("/ircv3/formatting/strip/color-comma",
	                test_ircv3_formatting_strip_color_comma);
	g_test_add_func("/ircv3/formatting/strip/color-full",
	                test_ircv3_formatting_strip_color_full);
	g_test_add_func("/ircv3/formatting/strip/color-foreground-comma",
	                test_ircv3_formatting_strip_color_foreground_comma);
	g_test_add_func("/ircv3/formatting/strip/color-foreground-3-digit",
	                test_ircv3_formatting_strip_color_foreground_3_digit);
	g_test_add_func("/ircv3/formatting/strip/color-background-3-digit",
	                test_ircv3_formatting_strip_color_background_3_digit);

	g_test_add_func("/ircv3/formatting/strip/hex-color",
	                test_ircv3_formatting_strip_hex_color);
	g_test_add_func("/ircv3/formatting/strip/hex-color-full",
	                test_ircv3_formatting_strip_hex_color_full);
	g_test_add_func("/ircv3/formatting/strip/hex-color-comma",
	                test_ircv3_formatting_strip_hex_color_comma);
	g_test_add_func("/ircv3/formatting/strip/hex-color-foreground-comma",
	                test_ircv3_formatting_strip_hex_color_foreground_comma);
	g_test_add_func("/ircv3/formatting/strip/hex-color-foreground-7-characters",
	                test_ircv3_formatting_strip_hex_color_foreground_7_characters);
	g_test_add_func("/ircv3/formatting/strip/hex-color-background-7-characters",
	                test_ircv3_formatting_strip_hex_color_background_7_characters);

	g_test_add_func("/ircv3/formatting/strip/bold",
	                test_ircv3_formatting_strip_bold);
	g_test_add_func("/ircv3/formatting/strip/italic",
	                test_ircv3_formatting_strip_italic);
	g_test_add_func("/ircv3/formatting/strip/monospace",
	                test_ircv3_formatting_strip_monospace);
	g_test_add_func("/ircv3/formatting/strip/reset",
	                test_ircv3_formatting_strip_reset);
	g_test_add_func("/ircv3/formatting/strip/reverse",
	                test_ircv3_formatting_strip_reverse);
	g_test_add_func("/ircv3/formatting/strip/strikethrough",
	                test_ircv3_formatting_strip_strikethrough);
	g_test_add_func("/ircv3/formatting/strip/underline",
	                test_ircv3_formatting_strip_underline);

	/* These tests are based on the examples here
	 * https://modern.ircdocs.horse/formatting.html#examples
	 */
	g_test_add_func("/ircv3/formatting/strip/spec-example1",
	                test_ircv3_formatting_strip_spec_example1);
	g_test_add_func("/ircv3/formatting/strip/spec-example2",
	                test_ircv3_formatting_strip_spec_example2);
	g_test_add_func("/ircv3/formatting/strip/spec-example3",
	                test_ircv3_formatting_strip_spec_example3);
	g_test_add_func("/ircv3/formatting/strip/spec-example4",
	                test_ircv3_formatting_strip_spec_example4);

	return g_test_run();
}
