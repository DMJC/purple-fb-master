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
 * Callbacks
 *****************************************************************************/
static void
test_purple_badges_items_changed(GListModel *model,
                                 G_GNUC_UNUSED guint position,
                                 G_GNUC_UNUSED guint removed,
                                 G_GNUC_UNUSED guint added, gpointer data)
{
	guint *counter = data;

	g_assert_true(PURPLE_IS_BADGES(model));

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_badges_properties(void) {
	PurpleBadges *badges;
	GType item_type = G_TYPE_INVALID;
	guint n_items = 0;

	badges = g_object_new(
		PURPLE_TYPE_BADGES,
		NULL);

	g_object_get(
		badges,
		"item-type", &item_type,
		"n-items", &n_items,
		NULL);

	g_assert_cmpuint(item_type, ==, PURPLE_TYPE_BADGE);
	g_assert_cmpuint(n_items, ==, 0);

	g_assert_finalize_object(badges);
}

static void
test_purple_badges_auto_sort(void) {
	PurpleBadge *high = NULL;
	PurpleBadge *medium = NULL;
	PurpleBadge *low = NULL;
	PurpleBadge *badge = NULL;
	PurpleBadges *badges = NULL;
	gboolean success = FALSE;
	guint counter = 0;
	guint n_items = 0;

	/* This test will add badges with 3 different priorities; high, medium, and
	 * low. medium will be added first, then low, then high. The order will be
	 * checked after each add a counter will be used to see if the
	 * "items-changed" signal was called once per addition.
	 *
	 * Once that's all good, low will be removed, then high, then medium, and
	 * the order and "items-changed" signal counter will be checked as well.
	 */

	badges = purple_badges_new();
	g_signal_connect(badges, "items-changed",
	                 G_CALLBACK(test_purple_badges_items_changed), &counter);

	high = purple_badge_new("high", 1000, "icon-name", "+");
	medium = purple_badge_new("medium", 0, "icon-name", "=");
	low = purple_badge_new("low", -1000, "icon-name", "-");

	/* Start by adding medium. */
	counter = 0;
	success = purple_badges_add_badge(badges, medium);
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(badges));
	g_assert_cmpuint(n_items, ==, 1);

	/* Add low. */
	counter = 0;
	success = purple_badges_add_badge(badges, low);
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(badges));
	g_assert_cmpuint(n_items, ==, 2);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 0);
	g_assert_true(badge == medium);
	g_clear_object(&badge);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 1);
	g_assert_true(badge == low);
	g_clear_object(&badge);

	/* Add High. */
	counter = 0;
	success = purple_badges_add_badge(badges, high);
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(badges));
	g_assert_cmpuint(n_items, ==, 3);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 0);
	g_assert(badge == high);
	g_clear_object(&badge);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 1);
	g_assert(badge == medium);
	g_clear_object(&badge);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 2);
	g_assert(badge == low);
	g_clear_object(&badge);

	/* Remove low. */
	counter = 0;
	success = purple_badges_remove_badge(badges, "low");
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(badges));
	g_assert_cmpuint(n_items, ==, 2);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 0);
	g_assert_true(badge == high);
	g_clear_object(&badge);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 1);
	g_assert_true(badge == medium);
	g_clear_object(&badge);

	/* Remove high. */
	counter = 0;
	success = purple_badges_remove_badge(badges, "high");
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(badges));
	g_assert_cmpuint(n_items, ==, 1);

	badge = g_list_model_get_item(G_LIST_MODEL(badges), 0);
	g_assert_true(badge == medium);
	g_clear_object(&badge);

	/* Remove medium. */
	counter = 0;
	success = purple_badges_remove_badge(badges, "medium");
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	n_items = g_list_model_get_n_items(G_LIST_MODEL(badges));
	g_assert_cmpuint(n_items, ==, 0);

	/* Cleanup. */
	g_assert_finalize_object(badges);
	g_assert_finalize_object(high);
	g_assert_finalize_object(medium);
	g_assert_finalize_object(low);
}

static void
test_purple_badges_double_add(void) {
	PurpleBadge *badge = NULL;
	PurpleBadges *badges = NULL;
	gboolean success = FALSE;
	guint counter = 0;

	badges = purple_badges_new();
	g_signal_connect(badges, "items-changed",
	                 G_CALLBACK(test_purple_badges_items_changed), &counter);

	badge = purple_badge_new("double-add", 0, "icon", "i");

	/* The first add. */
	counter = 0;
	success = purple_badges_add_badge(badges, badge);
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	/* The double add. */
	counter = 0;
	success = purple_badges_add_badge(badges, badge);
	g_assert_false(success);
	g_assert_cmpuint(counter, ==, 0);

	g_assert_finalize_object(badges);
	g_assert_finalize_object(badge);
}

static void
test_purple_badges_double_remove(void) {
	PurpleBadge *badge = NULL;
	PurpleBadges *badges = NULL;
	gboolean success = FALSE;
	guint counter = 0;

	badges = purple_badges_new();
	g_signal_connect(badges, "items-changed",
	                 G_CALLBACK(test_purple_badges_items_changed), &counter);

	badge = purple_badge_new("double-remove", 0, "icon", "i");

	/* The first add. */
	counter = 0;
	success = purple_badges_add_badge(badges, badge);
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	/* The first remove. */
	counter = 0;
	success = purple_badges_remove_badge(badges, "double-remove");
	g_assert_true(success);
	g_assert_cmpuint(counter, ==, 1);

	/* The second remove. */
	counter = 0;
	success = purple_badges_remove_badge(badges, "double-remove");
	g_assert_false(success);
	g_assert_cmpuint(counter, ==, 0);

	g_assert_finalize_object(badges);
	g_assert_finalize_object(badge);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char *argv[]) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/badges/properties", test_purple_badges_properties);
	g_test_add_func("/badges/auto-sort", test_purple_badges_auto_sort);
	g_test_add_func("/badges/double-add", test_purple_badges_double_add);
	g_test_add_func("/badges/double-remove", test_purple_badges_double_remove);

	return g_test_run();
}
