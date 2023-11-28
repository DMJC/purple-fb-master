/* purple
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
 */
#include <glib/gi18n-lib.h>

#include <purple.h>

#include <pidgin.h>

#define ICONAWAY_PLUGIN_ID "gtk-iconaway"

static void
iconify_window(gpointer data, G_GNUC_UNUSED gpointer user_data) {
	gtk_window_minimize(data);
}

static void
iconify_presence_changed_cb(GObject *obj,
                            G_GNUC_UNUSED GParamSpec *pspec,
                            G_GNUC_UNUSED gpointer data)
{
	PurplePresenceManager *manager = PURPLE_PRESENCE_MANAGER(obj);
	PurplePresencePrimitive primitive = PURPLE_PRESENCE_PRIMITIVE_OFFLINE;
	PurpleSavedPresence *presence = NULL;

	presence = purple_presence_manager_get_active(manager);
	primitive = purple_saved_presence_get_primitive(presence);

	if(primitive == PURPLE_PRESENCE_PRIMITIVE_AWAY ||
	   primitive == PURPLE_PRESENCE_PRIMITIVE_DO_NOT_DISTURB)
	{
		GApplication *application = NULL;
		GList *windows = NULL;

		application = g_application_get_default();
		windows = gtk_application_get_windows(GTK_APPLICATION(application));
		g_list_foreach(windows, iconify_window, NULL);
	}
}

/******************************************************************************
 * Plugin Exports
 *****************************************************************************/
static GPluginPluginInfo *
icon_away_query(G_GNUC_UNUSED GError **error)
{
	const gchar * const authors[] = {
		"Pidgin Developers <devel@pidgin.im>",
		NULL
	};

	return purple_plugin_info_new(
		"id",           ICONAWAY_PLUGIN_ID,
		"name",         N_("Minimize on Away"),
		"version",      DISPLAY_VERSION,
		"category",     N_("User interface"),
		"summary",      N_("Minimizes the buddy list and your conversations "
		                   "when you go away."),
		"description",  N_("Minimizes the buddy list and your conversations "
		                   "when you go away."),
		"authors",      authors,
		"website",      PURPLE_WEBSITE,
		"abi-version",  PURPLE_ABI_VERSION,
		NULL
	);
}

static gboolean
icon_away_load(GPluginPlugin *plugin, G_GNUC_UNUSED GError **error) {
	PurplePresenceManager *manager = NULL;

	manager = purple_presence_manager_get_default();
	g_signal_connect_object(manager, "notify::active-presence",
	                        G_CALLBACK(iconify_presence_changed_cb),
	                        plugin, 0);

	return TRUE;
}

static gboolean
icon_away_unload(G_GNUC_UNUSED GPluginPlugin *plugin,
                 G_GNUC_UNUSED gboolean shutdown,
                 G_GNUC_UNUSED GError **error)
{
	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(icon_away)
