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
test_purple_saved_presence_new(void) {
	PurpleSavedPresence *presence = NULL;

	presence = purple_saved_presence_new();
	g_assert_true(PURPLE_IS_SAVED_PRESENCE(presence));

	g_clear_object(&presence);
}

static void
test_purple_saved_presence_properties(void) {
	PurpleSavedPresence *presence = NULL;
	PurplePresencePrimitive primitive;
	GDateTime *last_used = NULL;
	GDateTime *last_used1 = NULL;
	guint use_count;
	char *name;
	char *message;
	char *emoji;

	last_used = g_date_time_new_now_local();

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	presence = g_object_new(
		PURPLE_TYPE_SAVED_PRESENCE,
		"last-used", last_used,
		"use-count", 123,
		"name", "my saved status",
		"primitive", PURPLE_PRESENCE_PRIMITIVE_STREAMING,
		"message", "I'm live on twitch at https://twitch.tv/rw_grim/",
		"emoji", "💀",
		NULL);

	/* Now use g_object_get to read all of the properties. */
	g_object_get(
		presence,
		"last-used", &last_used1,
		"use-count", &use_count,
		"name", &name,
		"primitive", &primitive,
		"message", &message,
		"emoji", &emoji,
		NULL);

	/* Compare all the things. */
	g_assert_nonnull(last_used1);
	g_assert_true(g_date_time_equal(last_used, last_used1));
	g_clear_pointer(&last_used1, g_date_time_unref);

	g_assert_cmpuint(use_count, ==, 123);

	g_assert_cmpstr(name, ==, "my saved status");
	g_clear_pointer(&name, g_free);

	g_assert_cmpint(primitive, ==, PURPLE_PRESENCE_PRIMITIVE_STREAMING);

	g_assert_cmpstr(message, ==,
	                "I'm live on twitch at https://twitch.tv/rw_grim/");
	g_clear_pointer(&message, g_free);

	g_assert_cmpstr(emoji, ==, "💀");
	g_clear_pointer(&emoji, g_free);

	g_clear_pointer(&last_used, g_date_time_unref);
	g_clear_object(&presence);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/saved-presence/new",
	                test_purple_saved_presence_new);
	g_test_add_func("/saved-presence/properties",
	                test_purple_saved_presence_properties);


	return g_test_run();
}
