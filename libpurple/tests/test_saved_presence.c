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
test_purple_saved_presence_properties(void) {
	PurpleSavedPresence *presence = NULL;
	PurplePresencePrimitive primitive;
	GDateTime *last_used = NULL;
	GDateTime *last_used1 = NULL;
	guint use_count;
	char *id = NULL;
	char *name = NULL;
	char *message = NULL;
	char *emoji = NULL;

	last_used = g_date_time_new_now_local();

	/* Use g_object_new so we can test setting properties by name. All of them
	 * call the setter methods, so by doing it this way we exercise more of the
	 * code.
	 */
	presence = g_object_new(
		PURPLE_TYPE_SAVED_PRESENCE,
		"last-used", last_used,
		"use-count", 123,
		"id", "leeloo dallas multipass",
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
		"id", &id,
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

	g_assert_cmpstr(id, ==, "leeloo dallas multipass");
	g_clear_pointer(&id, g_free);

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

static void
test_purple_saved_presence_generates_id(void) {
	PurpleSavedPresence *presence = NULL;
	const char *id = NULL;

	presence = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	id = purple_saved_presence_get_id(presence);
	g_assert_nonnull(id);
	g_assert_cmpstr(id, !=, "");

	g_clear_object(&presence);
}

static void
test_purple_saved_presence_equal_null_null(void) {
	g_assert_true(purple_saved_presence_equal(NULL, NULL));
}

static void
test_purple_saved_presence_equal_null_a(void) {
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	g_assert_false(purple_saved_presence_equal(NULL, b));

	g_clear_object(&b);
}

static void
test_purple_saved_presence_equal_null_b(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	g_assert_false(purple_saved_presence_equal(a, NULL));

	g_clear_object(&a);
}

static void
test_purple_saved_presence_equal_default(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	g_assert_true(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
}

static void
test_purple_saved_presence_equal_last_used(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	GDateTime *now = g_date_time_new_now_utc();
	GDateTime *yesterday = g_date_time_add_days(now, -1);

	/* Set the last used time for a but not b. */
	purple_saved_presence_set_last_used(a, now);
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the last used time for b to the same as a. */
	purple_saved_presence_set_last_used(b, now);
	g_assert_true(purple_saved_presence_equal(a, b));

	/* Set the last used time for b to yesterday. */
	purple_saved_presence_set_last_used(b, yesterday);
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the last used time for a to NULL. */
	purple_saved_presence_set_last_used(a, NULL);
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the last used time for b to NULL. */
	purple_saved_presence_set_last_used(b, NULL);
	g_assert_true(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
	g_clear_pointer(&now, g_date_time_unref);
	g_clear_pointer(&yesterday, g_date_time_unref);
}

static void
test_purple_saved_presence_equal_use_count(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	/* Set the use-count of a to 100. */
	purple_saved_presence_set_use_count(a, 100);
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Check an explicit value is equal. */
	purple_saved_presence_set_use_count(b, 100);
	g_assert_true(purple_saved_presence_equal(a, b));

	/* Set the use-count of b to 101. */
	purple_saved_presence_set_use_count(b, 101);
	g_assert_false(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
}

static void
test_purple_saved_presence_equal_name(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	/* Set the name of a. */
	purple_saved_presence_set_name(a, "present");
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the name of b to the same as a. */
	purple_saved_presence_set_name(b, "present");
	g_assert_true(purple_saved_presence_equal(a, b));

	/* Set the name of b to something else. */
	purple_saved_presence_set_name(b, "future");
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the name of a to NULL. */
	purple_saved_presence_set_name(a, NULL);
	g_assert_false(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
}

static void
test_purple_saved_presence_equal_primitive(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	/* Set the primitive of a to streaming. */
	purple_saved_presence_set_primitive(a,
	                                    PURPLE_PRESENCE_PRIMITIVE_STREAMING);
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the primitive of b to idle. */
	purple_saved_presence_set_primitive(b, PURPLE_PRESENCE_PRIMITIVE_IDLE);
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the primitives to the same value. */
	purple_saved_presence_set_primitive(b,
	                                    PURPLE_PRESENCE_PRIMITIVE_STREAMING);
	g_assert_true(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
}

static void
test_purple_saved_presence_equal_message(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	/* Set the message for a. */
	purple_saved_presence_set_message(a, "sleeping...");
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the message for b. */
	purple_saved_presence_set_message(b, "working!");
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the messages to the same. */
	purple_saved_presence_set_message(a, "working!");
	g_assert_true(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
}

static void
test_purple_saved_presence_equal_emoji(void) {
	PurpleSavedPresence *a = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);
	PurpleSavedPresence *b = g_object_new(PURPLE_TYPE_SAVED_PRESENCE, NULL);

	/* Set the emoji for a. */
	purple_saved_presence_set_emoji(a, "💤");
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the emoji for b. */
	purple_saved_presence_set_emoji(b, "🏢");
	g_assert_false(purple_saved_presence_equal(a, b));

	/* Set the emoji to the same. */
	purple_saved_presence_set_emoji(a, "🏢");
	g_assert_true(purple_saved_presence_equal(a, b));

	g_clear_object(&a);
	g_clear_object(&b);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/saved-presence/properties",
	                test_purple_saved_presence_properties);

	g_test_add_func("/saved-presence/generates-id",
	                test_purple_saved_presence_generates_id);

	g_test_add_func("/saved-presence/equal/null_null",
	                test_purple_saved_presence_equal_null_null);
	g_test_add_func("/saved-presence/equal/null_a",
	                test_purple_saved_presence_equal_null_a);
	g_test_add_func("/saved-presence/equal/null_b",
	                test_purple_saved_presence_equal_null_b);
	g_test_add_func("/saved-presence/equal/default",
	                test_purple_saved_presence_equal_default);

	g_test_add_func("/saved-presence/equal/last-used",
	                test_purple_saved_presence_equal_last_used);
	g_test_add_func("/saved-presence/equal/use-count",
	                test_purple_saved_presence_equal_use_count);
	g_test_add_func("/saved-presence/equal/name",
	                test_purple_saved_presence_equal_name);
	g_test_add_func("/saved-presence/equal/primitive",
	                test_purple_saved_presence_equal_primitive);
	g_test_add_func("/saved-presence/equal/message",
	                test_purple_saved_presence_equal_message);
	g_test_add_func("/saved-presence/equal/emoji",
	                test_purple_saved_presence_equal_emoji);

	return g_test_run();
}
