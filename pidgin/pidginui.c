/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib/gi18n.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include <purpleconfig.h>

#include "pidginui.h"

#include "gtkmedia.h"
#include "gtknotify.h"
#include "gtkroomlist.h"
#include "gtkrequest.h"
#include "gtkwhiteboard.h"
#include "pidgincore.h"
#include "pidgindebug.h"
#include "pidginprefs.h"
#include "pidginprivate.h"

struct _PidginUi {
	PurpleUi parent;
};

G_DEFINE_FINAL_TYPE(PidginUi, pidgin_ui, PURPLE_TYPE_UI)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
pidgin_ui_add_protocol_theme_paths(PurpleProtocol *protocol) {
	GdkDisplay *display = NULL;
	GtkIconTheme *theme = NULL;
	const gchar *path = NULL;

	display = gdk_display_get_default();

	theme = gtk_icon_theme_get_for_display(display);

	path = purple_protocol_get_icon_search_path(protocol);
	if(path != NULL) {
		gtk_icon_theme_add_search_path(theme, path);
	}

	path = purple_protocol_get_icon_resource_path(protocol);
	if(path != NULL) {
		gtk_icon_theme_add_resource_path(theme, path);
	}
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_ui_protocol_foreach_theme_cb(PurpleProtocol *protocol,
                                    G_GNUC_UNUSED gpointer data)
{
	pidgin_ui_add_protocol_theme_paths(protocol);
}

static void
pidgin_ui_protocol_registered_cb(G_GNUC_UNUSED PurpleProtocolManager *manager,
                                 PurpleProtocol *protocol)
{
	pidgin_ui_add_protocol_theme_paths(protocol);
}

/******************************************************************************
 * PurpleUi Implementation
 *****************************************************************************/
static void
pidgin_ui_prefs_init(G_GNUC_UNUSED PurpleUi *ui) {
	pidgin_prefs_init();
}

static gboolean
pidgin_ui_start(G_GNUC_UNUSED PurpleUi *ui, G_GNUC_UNUSED GError **error) {
	PurpleProtocolManager *protocol_manager = NULL;
	GdkDisplay *display = NULL;
	GtkIconTheme *theme = NULL;
	gchar *path;

	pidgin_debug_init();

	display = gdk_display_get_default();
	theme = gtk_icon_theme_get_for_display(display);

	path = g_build_filename(PURPLE_DATADIR, "pidgin", "icons", NULL);
	gtk_icon_theme_add_search_path(theme, path);
	g_free(path);

	/* Add a callback for when a protocol is registered to add its icon paths
	 * if it was found after initial startup.
	 */
	protocol_manager = purple_protocol_manager_get_default();
	g_signal_connect(protocol_manager, "registered",
	                 G_CALLBACK(pidgin_ui_protocol_registered_cb), NULL);

	/* Add the icon paths for all the protocols that libpurple found at start
	 * up.
	 */
	purple_protocol_manager_foreach(protocol_manager,
	                                pidgin_ui_protocol_foreach_theme_cb, NULL);

	/* Set the UI operation structures. */
	purple_notify_set_ui_ops(pidgin_notify_get_ui_ops());
	purple_request_set_ui_ops(pidgin_request_get_ui_ops());
	purple_whiteboard_set_ui_ops(pidgin_whiteboard_get_ui_ops());

	pidgin_request_init();
	pidgin_commands_init();
	pidgin_roomlist_init();
	pidgin_medias_init();
	pidgin_notify_init();

	return TRUE;
}

static void
pidgin_ui_stop(G_GNUC_UNUSED PurpleUi *ui) {
	/* Be sure to close all windows that are not attached to anything
	 * (e.g., the debug window), or they may access things after they are
	 * shut down. */
	pidgin_notify_uninit();
	pidgin_commands_uninit();
	pidgin_request_uninit();
	pidgin_debug_window_hide();
	pidgin_debug_uninit();

	/* and end it all... */
	g_application_quit(g_application_get_default());
}

static gpointer
pidgin_ui_get_settings_backend(G_GNUC_UNUSED PurpleUi *ui) {
	GSettingsBackend *backend = NULL;
	char *config = NULL;

	config = g_build_filename(purple_config_dir(), "pidgin3.ini", NULL);
	backend = g_keyfile_settings_backend_new(config, "/", NULL);

	g_free(config);

	return backend;
}

static PurpleHistoryAdapter *
pidgin_ui_get_history_adapter(G_GNUC_UNUSED PurpleUi *ui) {
	PurpleHistoryAdapter *adapter = NULL;
	char *filename = NULL;

	g_mkdir_with_parents(purple_config_dir(), 0700);

	filename = g_build_filename(purple_config_dir(), "history.db", NULL);
	adapter = purple_sqlite_history_adapter_new(filename);
	g_free(filename);

	return adapter;
}

static PurplePresenceManager *
pidgin_ui_get_presence_manager(G_GNUC_UNUSED PurpleUi *ui) {
	PurplePresenceManager *manager = NULL;
	char *path = NULL;

	g_mkdir_with_parents(purple_config_dir(), 0700);

	path = g_build_filename(purple_config_dir(), "presences", NULL);
	manager = purple_presence_manager_new(path);
	g_free(path);

	return manager;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_ui_init(G_GNUC_UNUSED PidginUi *ui) {
}

static void
pidgin_ui_class_init(PidginUiClass *klass) {
	PurpleUiClass *ui_class = PURPLE_UI_CLASS(klass);

	ui_class->prefs_init = pidgin_ui_prefs_init;
	ui_class->start = pidgin_ui_start;
	ui_class->stop = pidgin_ui_stop;
	ui_class->get_settings_backend = pidgin_ui_get_settings_backend;
	ui_class->get_history_adapter = pidgin_ui_get_history_adapter;
	ui_class->get_presence_manager = pidgin_ui_get_presence_manager;
}

/******************************************************************************
 * Public UI
 *****************************************************************************/
PurpleUi *
pidgin_ui_new(void) {
	return g_object_new(
		PIDGIN_TYPE_UI,
		"id", "pidgin3",
		"name", PIDGIN_NAME,
		"version", VERSION,
		"website", "https://pidgin.im",
		"support-website", "https://pidgin.im/contact/",
		"client-type", "pc",
		NULL);
}
