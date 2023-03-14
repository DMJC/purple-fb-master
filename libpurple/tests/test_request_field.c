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

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_request_field_notify_filled_cb(G_GNUC_UNUSED GObject *obj,
                                    G_GNUC_UNUSED GParamSpec *pspec,
                                    gpointer data)
{
	gboolean *called = data;

	*called = TRUE;
}

static void
test_request_field_filled_string(void) {
	PurpleRequestField *field = NULL;
	gboolean called = FALSE;

	field = purple_request_field_string_new("test-string", "Test string", NULL,
	                                        FALSE);
	g_signal_connect(field, "notify::filled",
	                 G_CALLBACK(test_request_field_notify_filled_cb), &called);
	g_assert_false(purple_request_field_is_filled(field));

	/* Passing same value should not trigger. */
	called = FALSE;
	purple_request_field_string_set_value(PURPLE_REQUEST_FIELD_STRING(field),
	                                      NULL);
	g_assert_false(called);
	g_assert_false(purple_request_field_is_filled(field));

	/* Passing an empty string should not trigger, as NULL and "" are
	 * considered the same. */
	called = FALSE;
	purple_request_field_string_set_value(PURPLE_REQUEST_FIELD_STRING(field),
	                                      "");
	g_assert_false(called);
	g_assert_false(purple_request_field_is_filled(field));

	/* Now that there's a change from empty to filled, notify should occur. */
	called = FALSE;
	purple_request_field_string_set_value(PURPLE_REQUEST_FIELD_STRING(field),
	                                      "text");
	g_assert_true(called);
	g_assert_true(purple_request_field_is_filled(field));

	/* Passing same value should not trigger. */
	called = FALSE;
	purple_request_field_string_set_value(PURPLE_REQUEST_FIELD_STRING(field),
	                                      "text");
	g_assert_false(called);
	g_assert_true(purple_request_field_is_filled(field));

	/* And then going back to empty should notify. */
	called = FALSE;
	purple_request_field_string_set_value(PURPLE_REQUEST_FIELD_STRING(field),
	                                      "");
	g_assert_true(called);
	g_assert_false(purple_request_field_is_filled(field));

	g_object_unref(field);
}

static void
test_request_field_filled_nonstring(void) {
	/* Anything that's not a string should always be considered filled and
	 * never notify. */
	PurpleRequestField *field = NULL;
	gboolean called = FALSE;

	field = purple_request_field_int_new("test-int", "Test int", 50, 0, 100);
	g_signal_connect(field, "notify::filled",
	                 G_CALLBACK(test_request_field_notify_filled_cb), &called);
	g_assert_true(purple_request_field_is_filled(field));

	called = FALSE;
	purple_request_field_int_set_value(PURPLE_REQUEST_FIELD_INT(field), 50);
	g_assert_false(called);
	g_assert_true(purple_request_field_is_filled(field));

	called = FALSE;
	purple_request_field_int_set_value(PURPLE_REQUEST_FIELD_INT(field), 0);
	g_assert_false(called);
	g_assert_true(purple_request_field_is_filled(field));

	called = FALSE;
	purple_request_field_int_set_value(PURPLE_REQUEST_FIELD_INT(field), 100);
	g_assert_false(called);
	g_assert_true(purple_request_field_is_filled(field));

	g_object_unref(field);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/request-field/filled-string",
	                test_request_field_filled_string);
	g_test_add_func("/request-field/filled-nonstring",
	                test_request_field_filled_nonstring);

	return g_test_run();
}
