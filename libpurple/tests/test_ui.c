/*
 * pidgin
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */

#include "purple.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include <signal.h>
#include <string.h>
#ifdef _WIN32
#  include <conio.h>
#else
#  include <unistd.h>
#endif

#include "test_ui.h"

/******************************************************************************
 * PurpleUi Implementation
 *****************************************************************************/
struct _TestPurpleUi {
	PurpleUi parent;
};

G_DECLARE_FINAL_TYPE(TestPurpleUi, test_purple_ui, TEST_PURPLE, UI, PurpleUi)

G_DEFINE_FINAL_TYPE(TestPurpleUi, test_purple_ui, PURPLE_TYPE_UI)

static gboolean
test_purple_ui_start(G_GNUC_UNUSED PurpleUi *ui, G_GNUC_UNUSED GError **error) {
	return TRUE;
}

static PurpleConversationManager *
test_purple_ui_get_conversation_manager(G_GNUC_UNUSED PurpleUi *ui) {
	return purple_conversation_manager_new(NULL);
}

static gpointer
test_purple_ui_get_settings_backend(G_GNUC_UNUSED PurpleUi *ui) {
	return g_memory_settings_backend_new();
}

static void
test_purple_ui_init(G_GNUC_UNUSED TestPurpleUi *ui) {
}

static void
test_purple_ui_class_init(TestPurpleUiClass *klass) {
	PurpleUiClass *ui_class = PURPLE_UI_CLASS(klass);

	ui_class->start = test_purple_ui_start;
	ui_class->get_conversation_manager =
		test_purple_ui_get_conversation_manager;
	ui_class->get_settings_backend = test_purple_ui_get_settings_backend;
}

static PurpleUi *
test_purple_ui_new(void) {
	return g_object_new(
		test_purple_ui_get_type(),
		"id", "test",
		"name", "Test-UI",
		"version", VERSION,
		"website", PURPLE_WEBSITE,
		"support-website", PURPLE_WEBSITE,
		"client-type", "test",
		NULL);
}

void
test_ui_purple_init(void) {
	PurpleUi *ui = NULL;
	GError *error = NULL;

	/* set the magic PURPLE_PLUGINS_SKIP environment variable */
	g_setenv("PURPLE_PLUGINS_SKIP", "1", TRUE);

	ui = test_purple_ui_new();

	/* Now that all the essential stuff has been set, let's try to init the core. It's
	 * necessary to provide a non-NULL name for the current ui to the core. This name
	 * is used by stuff that depends on this ui, for example the ui-specific plugins. */
	if (!purple_core_init(ui, &error)) {
		/* Initializing the core failed. Terminate. */
		fprintf(stderr,
		        _("Initialization of the libpurple core failed. %s\n"
		          "Aborting!\nPlease report this!\n"),
		        (error != NULL) ? error->message : "unknown error");
		g_clear_error(&error);
		abort();
	}

	/* Make sure our configuration directory exists. */
	g_mkdir_with_parents(purple_config_dir(), 0755);

	/* Load the preferences. */
	purple_prefs_load();

	purple_prefs_add_none("/purple");
	purple_prefs_add_none("/purple/test_ui");
	purple_prefs_add_none("/purple/test_ui/plugins");
	purple_prefs_add_path_list("/purple/test_ui/plugins/saved", NULL);

	/* Load the desired plugins. The client should save the list of loaded plugins in
	 * the preferences using purple_plugins_save_loaded() */
	purple_plugins_load_saved("/purple/test_ui/plugins/saved");
}

void
test_ui_purple_uninit(void) {
	purple_core_quit();
}
