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
#include <glib/gstdio.h>

#include <purple.h>

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
test_purple_presence_manager_add_remove_counter(G_GNUC_UNUSED PurplePresenceManager *manager,
                                                G_GNUC_UNUSED PurpleSavedPresence *presence,
                                                gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

static void
test_purple_presence_manager_list_changed_counter(G_GNUC_UNUSED GListModel *list,
                                                  G_GNUC_UNUSED guint position,
                                                  G_GNUC_UNUSED guint removed,
                                                  G_GNUC_UNUSED guint added,
                                                  gpointer data)
{
	guint *counter = data;

	*counter = *counter + 1;
}

/******************************************************************************
 * Tests
 *****************************************************************************/
static void
test_purple_presence_manager_new(void) {
	PurplePresenceManager *manager = NULL;
	GListModel *model = NULL;

	manager = purple_presence_manager_new(NULL);
	g_assert_true(PURPLE_IS_PRESENCE_MANAGER(manager));

	/* Make sure we have our 2 default saved presences.
	 *
	 * This may change over time, but we should update this for those changes.
	 */
	model = G_LIST_MODEL(manager);
	g_assert_cmpuint(g_list_model_get_n_items(model), ==, 2);

	g_clear_object(&manager);
}

static void
test_purple_presence_manager_add_remove(void) {
	PurplePresenceManager *manager = NULL;
	PurpleSavedPresence *presence = NULL;
	gboolean success = FALSE;
	guint len = 0;
	guint added = 0;
	guint removed = 0;
	guint changed = 0;
	const char *id = NULL;

	manager = purple_presence_manager_new(NULL);

	/* Connect all of our signals to make sure they're being emitted. */
	g_signal_connect(manager, "added",
	                 G_CALLBACK(test_purple_presence_manager_add_remove_counter),
	                 &added);
	g_signal_connect(manager, "removed",
	                 G_CALLBACK(test_purple_presence_manager_add_remove_counter),
	                 &removed);
	g_signal_connect(manager, "items-changed",
	                 G_CALLBACK(test_purple_presence_manager_list_changed_counter),
	                 &changed);

	len = g_list_model_get_n_items(G_LIST_MODEL(manager));
	/* The manager makes sure we always have online and offline presences. */
	g_assert_cmpuint(len, ==, 2);

	presence = purple_presence_manager_create(manager);
	g_assert_true(PURPLE_IS_SAVED_PRESENCE(presence));
	purple_saved_presence_set_name(presence, "test presence");
	g_assert_cmpuint(added, ==, 1);
	g_assert_cmpuint(removed, ==, 0);
	g_assert_cmpuint(changed, ==, 1);

	id = purple_saved_presence_get_id(presence);
	g_assert_nonnull(id);

	len = g_list_model_get_n_items(G_LIST_MODEL(manager));
	g_assert_cmpuint(len, ==, 3);

	success = purple_presence_manager_remove(manager, id);
	g_assert_true(success);
	g_assert_cmpuint(added, ==, 1);
	g_assert_cmpuint(removed, ==, 1);
	g_assert_cmpuint(changed, ==, 2);

	len = g_list_model_get_n_items(G_LIST_MODEL(manager));
	g_assert_cmpuint(len, ==, 2);

	g_clear_object(&presence);
	g_clear_object(&manager);
}

static void
test_purple_presence_manager_persistence(void) {
	PurplePresenceManager *manager = NULL;
	PurpleSavedPresence *presence = NULL;
	char *path = NULL;
	char *filename = NULL;
	char *old_id = NULL;
	const char *id = NULL;
	const char *id1 = NULL;
	gboolean ret = FALSE;

	path = g_build_filename(TEST_CACHE_DIR, "presence_manager_persistence",
	                        NULL);

	/* Remove the file if it exists so we can start from a known state. */
	filename = g_build_filename(path, "manager.ini", NULL);
	g_remove(filename);
	g_clear_pointer(&filename, g_free);

	/* Create the manager, add a presence, and make it active. */
	manager = purple_presence_manager_new(path);
	g_assert_true(PURPLE_IS_PRESENCE_MANAGER(manager));

	presence = purple_presence_manager_create(manager);
	purple_saved_presence_set_name(presence, "test-presence");

	/* Save the id of the presence as we need to use it later. */
	id = purple_saved_presence_get_id(presence);
	old_id = g_strdup(id);

	/* Make the presence active. */
	ret = purple_presence_manager_set_active(manager, presence);
	g_assert_true(ret);
	g_clear_object(&presence);

	presence = purple_presence_manager_get_active(manager);
	g_assert_true(PURPLE_IS_SAVED_PRESENCE(presence));

	id1 = purple_saved_presence_get_id(presence);
	g_assert_cmpstr(id1, ==, id);

	/* Clean everything up. The last presence was transfer none so we just need
	 * to null it out.
	 */
	presence = NULL;
	g_clear_object(&manager);

	/* Now create the manager again and verify that everything was restored. */
	manager = purple_presence_manager_new(path);
	g_assert_true(PURPLE_IS_PRESENCE_MANAGER(manager));

	presence = purple_presence_manager_get_active(manager);
	g_assert_true(PURPLE_IS_SAVED_PRESENCE(presence));

	id = purple_saved_presence_get_id(presence);
	g_assert_cmpstr(id, ==, old_id);
	g_clear_pointer(&old_id, g_free);

	/* Cleanup. */
	g_clear_object(&manager);
	g_clear_pointer(&path, g_free);
}

/******************************************************************************
 * Main
 *****************************************************************************/
gint
main(gint argc, gchar *argv[]) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/presence-manager/new",
	                test_purple_presence_manager_new);
	g_test_add_func("/presence-manager/add-remove",
	                test_purple_presence_manager_add_remove);
	g_test_add_func("/presence-manager/persistence",
	                test_purple_presence_manager_persistence);

	return g_test_run();
}
