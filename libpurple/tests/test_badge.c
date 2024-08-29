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
test_purple_badge_properties(void) {
	PurpleBadge *badge;
	char *description;
	char *icon_name;
	char *id;
	char *link_text;
	char *link_uri;
	char *mnemonic;
	char *tooltip_text;
	int priority;

	badge = g_object_new(
		PURPLE_TYPE_BADGE,
		"description", "description",
		"icon-name", "icon-name",
		"id", "test-badge",
		"link-text", "pidgy",
		"link-uri", "https://pidgin.im/",
		"mnemonic", "@",
		"priority", 1000,
		"tooltip-text", "tooltip-text",
		NULL);

	g_object_get(
		badge,
		"description", &description,
		"icon-name", &icon_name,
		"id", &id,
		"link-text", &link_text,
		"link-uri", &link_uri,
		"mnemonic", &mnemonic,
		"priority", &priority,
		"tooltip-text", &tooltip_text,
		NULL);

	g_assert_cmpstr(description, ==, "description");
	g_clear_pointer(&description, g_free);

	g_assert_cmpstr(icon_name, ==, "icon-name");
	g_clear_pointer(&icon_name, g_free);

	g_assert_cmpstr(id, ==, "test-badge");
	g_clear_pointer(&id, g_free);

	g_assert_cmpstr(link_text, ==, "pidgy");
	g_clear_pointer(&link_text, g_free);

	g_assert_cmpstr(link_uri, ==, "https://pidgin.im/");
	g_clear_pointer(&link_uri, g_free);

	g_assert_cmpstr(mnemonic, ==, "@");
	g_clear_pointer(&mnemonic, g_free);

	g_assert_cmpint(priority, ==, 1000);

	g_assert_cmpstr(tooltip_text, ==, "tooltip-text");
	g_clear_pointer(&tooltip_text, g_free);

	g_assert_finalize_object(badge);
}

static void
test_purple_badge_compare_null_null(void) {
	int result = 0;

	result = purple_badge_compare(NULL, NULL);
	g_assert_cmpint(result, ==, 0);
}

static void
test_purple_badge_compare_non_null_null(void) {
	PurpleBadge *badge = NULL;
	int result = 0;

	badge = purple_badge_new("test-badge", 0, "icon-name", "+");

	result = purple_badge_compare(badge, NULL);
	g_assert_cmpint(result, <, 0);

	g_assert_finalize_object(badge);
}

static void
test_purple_badge_compare_null_non_null(void) {
	PurpleBadge *badge = NULL;
	int result = 0;

	badge = purple_badge_new("test-badge", 0, "icon-name", "+");

	result = purple_badge_compare(NULL, badge);
	g_assert_cmpint(result, >, 0);

	g_assert_finalize_object(badge);
}

static void
test_purple_badge_compare_first(void) {
	PurpleBadge *badge1 = NULL;
	PurpleBadge *badge2 = NULL;
	int result = 0;

	badge1 = purple_badge_new("one", 1000, "icon1", "@");
	badge2 = purple_badge_new("two", -1000, "icon2", "+");

	result = purple_badge_compare(badge1, badge2);

	g_assert_cmpint(result, <, 0);

	g_assert_finalize_object(badge1);
	g_assert_finalize_object(badge2);
}

static void
test_purple_badge_compare_equal(void) {
	PurpleBadge *badge1 = NULL;
	PurpleBadge *badge2 = NULL;
	int result = 0;

	badge1 = purple_badge_new("one", 500, "icon1", "@");
	badge2 = purple_badge_new("two", 500, "icon2", "+");

	result = purple_badge_compare(badge1, badge2);

	g_assert_cmpint(result, ==, 0);

	g_assert_finalize_object(badge1);
	g_assert_finalize_object(badge2);
}

static void
test_purple_badge_compare_second(void) {
	PurpleBadge *badge1 = NULL;
	PurpleBadge *badge2 = NULL;
	int result = 0;

	badge1 = purple_badge_new("one", -1000, "icon1", "@");
	badge2 = purple_badge_new("two", -500, "icon2", "+");

	result = purple_badge_compare(badge1, badge2);

	g_assert_cmpint(result, >, 0);

	g_assert_finalize_object(badge1);
	g_assert_finalize_object(badge2);
}

static void
test_purple_badge_equal_true(void) {
	PurpleBadge *badge1 = NULL;
	PurpleBadge *badge2 = NULL;

	badge1 = purple_badge_new("one", -1000, "icon1", "@");
	badge2 = purple_badge_new("one", -500, "icon2", "+");

	g_assert_true(purple_badge_equal(badge1, badge2));

	g_assert_finalize_object(badge1);
	g_assert_finalize_object(badge2);
}

static void
test_purple_badge_equal_false(void) {
	PurpleBadge *badge1 = NULL;
	PurpleBadge *badge2 = NULL;

	badge1 = purple_badge_new("one", -1000, "icon1", "@");
	badge2 = purple_badge_new("two", -500, "icon2", "+");

	g_assert_false(purple_badge_equal(badge1, badge2));

	g_assert_finalize_object(badge1);
	g_assert_finalize_object(badge2);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/badge/properties", test_purple_badge_properties);

	g_test_add_func("/badge/compare/null_null",
	                test_purple_badge_compare_null_null);
	g_test_add_func("/badge/compare/non_null_null",
	                test_purple_badge_compare_non_null_null);
	g_test_add_func("/badge/compare/null_non_null",
	                test_purple_badge_compare_null_non_null);
	g_test_add_func("/badge/compare/first", test_purple_badge_compare_first);
	g_test_add_func("/badge/compare/equal", test_purple_badge_compare_equal);
	g_test_add_func("/badge/compare/second", test_purple_badge_compare_second);

	g_test_add_func("/badge/equal/true", test_purple_badge_equal_true);
	g_test_add_func("/badge/equal/false", test_purple_badge_equal_false);

	return g_test_run();
}
