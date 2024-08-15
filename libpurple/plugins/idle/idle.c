/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
purple_idle_unset(void) {
	PurpleIdleManager *manager = NULL;

	manager = purple_idle_manager_get_default();

	purple_idle_manager_set_source(manager, "idle-maker", NULL);
}

static void
purple_idle_set(gint32 minutes) {
	PurpleIdleManager *manager = NULL;
	GDateTime *now = NULL;
	GDateTime *idled_at = NULL;

	now = g_date_time_new_now_utc();
	idled_at = g_date_time_add_minutes(now, -1 * minutes);

	manager = purple_idle_manager_get_default();
	purple_idle_manager_set_source(manager, "idle-maker", idled_at);

	g_clear_pointer(&idled_at, g_date_time_unref);
	g_clear_pointer(&now, g_date_time_unref);
}

/******************************************************************************
 * Actions
 *****************************************************************************/
static void
purple_idle_set_cb(G_GNUC_UNUSED GSimpleAction *action, GVariant *parameter,
                   G_GNUC_UNUSED gpointer data)
{
	gint32 minutes = 0;

	if(!g_variant_is_of_type(parameter, G_VARIANT_TYPE_INT32)) {
		g_warning("unknown parameter type (%s)",
		          g_variant_get_type_string(parameter));

		return;
	}

	minutes = g_variant_get_int32(parameter);
	if(minutes < 0) {
		minutes = g_random_int_range(1, 1337);
	}

	purple_idle_set(minutes);
}

static void
purple_idle_unset_cb(G_GNUC_UNUSED GSimpleAction *action,
                  G_GNUC_UNUSED GVariant *parameter,
                  G_GNUC_UNUSED gpointer data)
{
	purple_idle_unset();
}

/******************************************************************************
 * GPlugin Exports
 *****************************************************************************/
static GPluginPluginInfo *
idle_query(G_GNUC_UNUSED GError **error)
{
	GSimpleActionGroup *group = NULL;
	GActionEntry entries[] = {
		{
			.name = "set-idle-time",
			.activate = purple_idle_set_cb,
			.parameter_type = "i",
		}, {
			.name = "unset-idle-time",
			.activate = purple_idle_unset_cb,
		}
	};
	GMenu *menu = NULL;
	GMenuItem *item = NULL;
	const gchar * const authors[] = {
		"Pidgin Developers",
		NULL
	};

	group = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(group), entries,
	                                G_N_ELEMENTS(entries), NULL);

	menu = g_menu_new();

	/* Since we have targets, we need build the menu the verbose way. */
	item = g_menu_item_new(_("Set idle to 5 minutes"), NULL);
	g_menu_item_set_action_and_target(item, "set-idle-time", "i", 5);
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Set idle to 10 minutes"), NULL);
	g_menu_item_set_action_and_target(item, "set-idle-time", "i", 10);
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Set idle to 15 minutes"), NULL);
	g_menu_item_set_action_and_target(item, "set-idle-time", "i", 15);
	g_menu_append_item(menu, item);
	g_object_unref(item);

	item = g_menu_item_new(_("Set idle to random time"), NULL);
	g_menu_item_set_action_and_target(item, "set-idle-time", "i", -1);
	g_menu_append_item(menu, item);
	g_object_unref(item);

	g_menu_append(menu, _("Unset idle time"), "unset-idle-time");

	return purple_plugin_info_new(
		"id", "core-idle",

		/* TRANSLATORS: This is a cultural reference.  Dy'er Mak'er is a song
	     * by Led Zeppelin. If that doesn't translate well into your language,
	     * drop the 's before translating.
	     */
		"name", N_("I'dle Mak'er"),
		"version", DISPLAY_VERSION,
		"category", N_("Utility"),
		"summary", N_("Allows you to set how long you've been idle"),
		"description", N_("Allows you set how long you've been idle"),
		"authors", authors,
		"website", PURPLE_WEBSITE,
		"abi-version", PURPLE_ABI_VERSION,
		"action-group", group,
		"action-menu", menu,
		NULL
	);
}

static gboolean
idle_load(G_GNUC_UNUSED GPluginPlugin *plugin, G_GNUC_UNUSED GError **error) {
	return TRUE;
}

static gboolean
idle_unload(G_GNUC_UNUSED GPluginPlugin *plugin, gboolean shutdown,
            G_GNUC_UNUSED GError **error)
{
	if(!shutdown) {
		purple_idle_unset();
	}

	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(idle);
