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

#include "purplexmppcore.h"

#include "purplexmppprotocol.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static PurpleProtocol *xmpp_protocol = NULL;

/******************************************************************************
 * GPlugin Exports
 *****************************************************************************/
static GPluginPluginInfo *
purple_xmpp_query(G_GNUC_UNUSED GError **error) {
	PurplePluginInfoFlags flags = PURPLE_PLUGIN_INFO_FLAGS_INTERNAL |
	                              PURPLE_PLUGIN_INFO_FLAGS_AUTO_LOAD;
	const gchar * const authors[] = {
		"Pidgin Developers <devel@pidgin.im>",
		NULL
	};

	return purple_plugin_info_new(
		"id", "prpl-xmpp",
		"name", "XMPP Protocol",
		"authors", authors,
		"version", DISPLAY_VERSION,
		"category", N_("Protocol"),
		"summary", N_("XMPP Protocol Plugin"),
		"description", N_("Modern XMPP Support"),
		"website", PURPLE_WEBSITE,
		"abi-version", PURPLE_ABI_VERSION,
		"flags", flags,
		NULL);
}

static gboolean
purple_xmpp_load(GPluginPlugin *plugin, GError **error) {
	PurpleProtocolManager *manager = NULL;

	if(PURPLE_IS_PROTOCOL(xmpp_protocol)) {
		g_set_error_literal(error, PURPLE_XMPP_DOMAIN, 0,
		                    "plugin was not cleaned up properly");

		return FALSE;
	}

	purple_xmpp_protocol_register(GPLUGIN_NATIVE_PLUGIN(plugin));

	xmpp_protocol = purple_xmpp_protocol_new();

	manager = purple_protocol_manager_get_default();
	if(PURPLE_IS_PROTOCOL_MANAGER(manager)) {
		if(!purple_protocol_manager_register(manager, xmpp_protocol, error)) {
			g_clear_object(&xmpp_protocol);

			return FALSE;
		}
	}

	return TRUE;
}

static gboolean
purple_xmpp_unload(G_GNUC_UNUSED GPluginPlugin *plugin,
                   G_GNUC_UNUSED gboolean shutdown,
                   GError **error)
{
	PurpleProtocolManager *manager = NULL;

	if(!PURPLE_IS_PROTOCOL(xmpp_protocol)) {
		g_set_error_literal(error, PURPLE_XMPP_DOMAIN, 0,
		                    "plugin was not setup properly");

		return FALSE;
	}

	manager = purple_protocol_manager_get_default();
	if(PURPLE_IS_PROTOCOL_MANAGER(manager)) {
		if(!purple_protocol_manager_unregister(manager, xmpp_protocol,
		                                       error))
		{
			return FALSE;
		}
	}

	g_clear_object(&xmpp_protocol);

	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(purple_xmpp)
