/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
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

#include <glib.h>

#include <purple.h>

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
test_purple_idle_manager_timestamp_changed(G_GNUC_UNUSED GObject *obj,
                                           G_GNUC_UNUSED GParamSpec *pspec,
                                           gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

/******************************************************************************
 * Basic Tests
 *****************************************************************************/
static void
test_purple_idle_manager_new(void) {
	PurpleIdleManager *manager = NULL;

	manager = g_object_new(PURPLE_TYPE_IDLE_MANAGER, NULL);

	g_assert_true(PURPLE_IS_IDLE_MANAGER(manager));

	g_clear_object(&manager);
}

static void
test_purple_idle_manager_single_source(void) {
	PurpleIdleManager *manager = NULL;
	GDateTime *actual = NULL;
	GDateTime *timestamp = NULL;
	GDateTime *now = NULL;
	gboolean res = FALSE;
	guint counter = 0;

	manager = g_object_new(PURPLE_TYPE_IDLE_MANAGER, NULL);

	/* Create a timestamp from an hour ago. */
	now = g_date_time_new_now_local();
	timestamp = g_date_time_add_hours(now, -1);
	g_clear_pointer(&now, g_date_time_unref);

	/* Connect our signal to make sure the timestamp got set. */
	g_signal_connect(manager, "notify::timestamp",
	                 G_CALLBACK(test_purple_idle_manager_timestamp_changed),
	                 &counter);

	/* Set the source. */
	res = purple_idle_manager_set_source(manager, "unit-tests", timestamp);
	g_assert_true(res);

	/* Verify the source is now the active one. */
	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_nonnull(actual);
	g_assert_true(g_date_time_equal(actual, timestamp));
	g_clear_pointer(&timestamp, g_date_time_unref);

	/* Make sure the notify signal got called. */
	g_assert_cmpuint(counter, ==, 1);

	/* Now remove the source and verify that the notify signal was emitted and
	 * that there is no active source.
	 */
	counter = 0;
	res = purple_idle_manager_set_source(manager, "unit-tests", NULL);
	g_assert_true(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_null(actual);

	g_assert_cmpuint(counter, ==, 1);

	g_clear_object(&manager);
}

static void
test_purple_idle_manager_multiple_sources(void) {
	PurpleIdleManager *manager = NULL;
	GDateTime *actual = NULL;
	GDateTime *timestamp1 = NULL;
	GDateTime *timestamp2 = NULL;
	GDateTime *timestamp3 = NULL;
	GDateTime *now = NULL;
	gboolean res = FALSE;

	manager = g_object_new(PURPLE_TYPE_IDLE_MANAGER, NULL);

	/* Create a few timestamps for testing. */
	now = g_date_time_new_now_local();
	timestamp1 = g_date_time_add_minutes(now, -10);
	timestamp2 = g_date_time_add_minutes(now, -60);
	timestamp3 = g_date_time_add_minutes(now, -1);
	g_clear_pointer(&now, g_date_time_unref);

	/* Set source1 which is 10 minutes idle. */
	res = purple_idle_manager_set_source(manager, "source1", timestamp1);
	g_assert_true(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_nonnull(actual);
	g_assert_true(g_date_time_equal(actual, timestamp1));

	/* Set source2 which is 1 hour idle which should take over the global state
	 * as well.
	 */
	res = purple_idle_manager_set_source(manager, "source2", timestamp2);
	g_assert_true(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_nonnull(actual);
	g_assert_true(g_date_time_equal(actual, timestamp2));

	/* Now remove source2 and verify we fell back to source1. */
	res = purple_idle_manager_set_source(manager, "source2", NULL);
	g_assert_true(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_nonnull(actual);
	g_assert_true(g_date_time_equal(actual, timestamp1));

	/* Add source3 that won't cause a change. */
	res = purple_idle_manager_set_source(manager, "source3", timestamp3);
	g_assert_false(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_true(g_date_time_equal(actual, timestamp1));

	/* Remove source3 and verify that source1 is still the active source. */
	res = purple_idle_manager_set_source(manager, "source3", NULL);
	g_assert_false(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_true(g_date_time_equal(actual, timestamp1));

	/* Finally remove source1 and verify that we're no longer idle. */
	res = purple_idle_manager_set_source(manager, "source1", NULL);
	g_assert_true(res);

	actual = purple_idle_manager_get_timestamp(manager);
	g_assert_null(actual);

	/* Cleanup. */
	g_clear_pointer(&timestamp1, g_date_time_unref);
	g_clear_pointer(&timestamp2, g_date_time_unref);
	g_clear_pointer(&timestamp3, g_date_time_unref);
	g_clear_object(&manager);
}

/******************************************************************************
 * Main
 *****************************************************************************/
int
main(int argc, char **argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_test_add_func("/idle-manager/new", test_purple_idle_manager_new);
	g_test_add_func("/idle-manager/single-source",
	                test_purple_idle_manager_single_source);
	g_test_add_func("/idle-manager/multiple-sources",
	                test_purple_idle_manager_multiple_sources);

	return g_test_run();
}
