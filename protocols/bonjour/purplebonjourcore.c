/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
#include <glib/gi18n-lib.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

#include "purplebonjourcore.h"

#include "purplebonjourconnection.h"
#include "purplebonjourprotocol.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static PurpleProtocol *bonjour_protocol = NULL;

/******************************************************************************
 * GPlugin Exports
 *****************************************************************************/
static GPluginPluginInfo *
purple_bonjour_query(G_GNUC_UNUSED GError **error) {
	PurplePluginInfoFlags flags = PURPLE_PLUGIN_INFO_FLAGS_INTERNAL |
	                              PURPLE_PLUGIN_INFO_FLAGS_AUTO_LOAD;
	const char * const authors[] = {
		"Pidgin Developers <devel@pidgin.im>",
		NULL
	};

	return purple_plugin_info_new(
		"id", "prpl-bonjour-nouveau",
		"name", "Bonjour Protocol",
		"authors", authors,
		"version", DISPLAY_VERSION,
		"category", N_("Protocol"),
		"summary", N_("Bonjour Protocol Plugin"),
		"description", N_("Modern Bonjour Support"),
		"website", PURPLE_WEBSITE,
		"abi-version", PURPLE_ABI_VERSION,
		"flags", flags,
		NULL);
}

static gboolean
purple_bonjour_load(GPluginPlugin *plugin, GError **error) {
	PurpleProtocolManager *manager = NULL;

	if(PURPLE_IS_PROTOCOL(bonjour_protocol)) {
		g_set_error_literal(error, PURPLE_BONJOUR_DOMAIN, 0,
		                    "plugin was not cleaned up properly");

		return FALSE;
	}

	purple_bonjour_protocol_register(GPLUGIN_NATIVE_PLUGIN(plugin));
	purple_bonjour_connection_register(GPLUGIN_NATIVE_PLUGIN(plugin));

	bonjour_protocol = purple_bonjour_protocol_new();

	manager = purple_protocol_manager_get_default();
	if(PURPLE_IS_PROTOCOL_MANAGER(manager)) {
		if(!purple_protocol_manager_register(manager, bonjour_protocol, error)) {
			g_clear_object(&bonjour_protocol);

			return FALSE;
		}
	}

	return TRUE;
}

static gboolean
purple_bonjour_unload(G_GNUC_UNUSED GPluginPlugin *plugin,
                      G_GNUC_UNUSED gboolean shutdown,
                      GError **error)
{
	PurpleProtocolManager *manager = NULL;

	if(!PURPLE_IS_PROTOCOL(bonjour_protocol)) {
		g_set_error_literal(error, PURPLE_BONJOUR_DOMAIN, 0,
		                    "plugin was not setup properly");

		return FALSE;
	}

	manager = purple_protocol_manager_get_default();
	if(PURPLE_IS_PROTOCOL_MANAGER(manager)) {
		if(!purple_protocol_manager_unregister(manager, bonjour_protocol,
		                                       error))
		{
			return FALSE;
		}
	}

	g_clear_object(&bonjour_protocol);

	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(purple_bonjour)
