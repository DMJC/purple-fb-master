/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PURPLE_TESTS_H
#define PURPLE_TESTS_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * PurpleTestStringData:
 *
 * Test data for testing strings.
 *
 * Since: 2.0
 */
typedef struct {
	const gchar *input;
	const gchar *output;
} PurpleTestStringData;

/**
 * PurpleTestStringFunc:
 * @str: The string to test.
 *
 * A function to call with @str as the input.
 *
 * Returns: The output of the function.
 *
 * Since: 2.0
 */
typedef const gchar *(*PurpleTestStringFunc)(const gchar *str);

/**
 * PurpleTestStringFreeFunc:
 * @str: The string to test.
 *
 * A function to call with @str as the input.
 *
 * Returns: (transfer full): The output of the function.
 *
 * Since: 2.0
 */
typedef gchar *(*PurpleTestStringFreeFunc)(const gchar *str);

static inline void
purple_test_string_compare(PurpleTestStringFunc func,
                           PurpleTestStringData data[])
{
	gint i;

	for(i = 0; data[i].input; i++)
		g_assert_cmpstr(data[i].output, ==, func(data[i].input));
}

static inline void
purple_test_string_compare_free(PurpleTestStringFreeFunc func,
                                PurpleTestStringData data[])
{
	gint i;

	for(i = 0; data[i].input; i++) {
		gchar *got = func(data[i].input);

		g_assert_cmpstr(data[i].output, ==, got);

		g_free(got);
	}
}


G_END_DECLS

#endif /* PURPLE_TESTS_H */
