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

#include <glib/gi18n-lib.h>

#include "purplebonjourprotocol.h"

#include "purplebonjourconnection.h"
#include "purplebonjourconstants.h"
#include "purplebonjourcore.h"

struct _PurpleBonjourProtocol {
	PurpleProtocol parent;
};

/******************************************************************************
 * PurpleProtocol Implementation
 *****************************************************************************/
static GList *
purple_bonjour_protocol_get_account_options(G_GNUC_UNUSED PurpleProtocol *protocol)
{
	PurpleAccountOption *option = NULL;
	GList *options = NULL;

	option = purple_account_option_int_new(_("Port"), PURPLE_BONJOUR_OPTION_PORT,
	                                       PURPLE_BONJOUR_DEFAULT_PORT);
	options = g_list_append(options, option);

	return options;
}

static void
purple_bonjour_protocol_can_connect_async(PurpleProtocol *protocol,
                                          G_GNUC_UNUSED PurpleAccount *account,
                                          GCancellable *cancellable,
                                          GAsyncReadyCallback callback,
                                          gpointer data)
{
	GTask *task = NULL;
	GNetworkMonitor *monitor = NULL;
	GNetworkConnectivity connectivity = 0;

	task = g_task_new(protocol, cancellable, callback, data);

	/* Bonjour runs on LANs so we just need local connectivity. */
	monitor = g_network_monitor_get_default();
	connectivity = g_network_monitor_get_connectivity(monitor);

	if(connectivity >= G_NETWORK_CONNECTIVITY_LOCAL) {
		g_task_return_boolean(task, TRUE);
	} else {
		g_task_return_new_error(task, PURPLE_BONJOUR_DOMAIN, 0,
		                        _("Network connection not detected."));
	}

	g_clear_object(&task);
}

static gboolean
purple_bonjour_protocol_can_connect_finish(G_GNUC_UNUSED PurpleProtocol *protocol,
                                           GAsyncResult *result,
                                           GError **error)
{
	return g_task_propagate_boolean(G_TASK(result), error);
}

static PurpleConnection *
purple_bonjour_protocol_create_connection(PurpleProtocol *protocol,
                                          PurpleAccount *account,
                                          const char *password,
                                          G_GNUC_UNUSED GError **error)
{
	return g_object_new(
		PURPLE_BONJOUR_TYPE_CONNECTION,
		"protocol", protocol,
		"account", account,
		"password", password,
		NULL);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_DYNAMIC_TYPE_EXTENDED(
	PurpleBonjourProtocol,
	purple_bonjour_protocol,
	PURPLE_TYPE_PROTOCOL,
	G_TYPE_FLAG_FINAL,
	{})

static void
purple_bonjour_protocol_init(G_GNUC_UNUSED PurpleBonjourProtocol *protocol) {
}

static void
purple_bonjour_protocol_class_finalize(G_GNUC_UNUSED PurpleBonjourProtocolClass *klass)
{
}

static void
purple_bonjour_protocol_class_init(PurpleBonjourProtocolClass *klass) {
	PurpleProtocolClass *protocol_class = PURPLE_PROTOCOL_CLASS(klass);

	protocol_class->get_account_options = purple_bonjour_protocol_get_account_options;
	protocol_class->can_connect_async = purple_bonjour_protocol_can_connect_async;
	protocol_class->can_connect_finish = purple_bonjour_protocol_can_connect_finish;
	protocol_class->create_connection = purple_bonjour_protocol_create_connection;
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_bonjour_protocol_register(GPluginNativePlugin *plugin) {
	purple_bonjour_protocol_register_type(G_TYPE_MODULE(plugin));
}

PurpleProtocol *
purple_bonjour_protocol_new(void) {
	return g_object_new(
		PURPLE_BONJOUR_TYPE_PROTOCOL,
		"id", "prpl-bonjour-nouveau",
		"name", "Bonjour",
		"description", _("Bonjour is a serverless protocol for local networks."),
		"icon-name", "im-bonjour",
		"icon-resource-path", "/im/pidgin/libpurple/protocols/bonjour/icons",
		"options", OPT_PROTO_NO_PASSWORD,
		NULL);
}
