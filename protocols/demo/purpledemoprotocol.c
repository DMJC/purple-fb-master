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

#include <glib/gi18n-lib.h>

#include "purpledemoprotocol.h"

#include "purpledemoconnection.h"
#include "purpledemoprotocolactions.h"
#include "purpledemoprotocolcontacts.h"
#include "purpledemoprotocolconversation.h"

struct _PurpleDemoProtocol {
	PurpleProtocol parent;
};

/******************************************************************************
 * PurpleProtocol Implementation
 *****************************************************************************/
static PurpleConnection *
purple_demo_protocol_create_connection(PurpleProtocol *protocol,
                                       PurpleAccount *account,
                                       const char *password,
                                       G_GNUC_UNUSED GError **error)
{
	g_return_val_if_fail(PURPLE_IS_PROTOCOL(protocol), NULL);
	g_return_val_if_fail(PURPLE_IS_ACCOUNT(account), NULL);

	return g_object_new(
		PURPLE_DEMO_TYPE_CONNECTION,
		"protocol", protocol,
		"account", account,
		"password", password,
		NULL);

}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_DYNAMIC_TYPE_EXTENDED(
	PurpleDemoProtocol,
	purple_demo_protocol,
	PURPLE_TYPE_PROTOCOL,
	G_TYPE_FLAG_FINAL,
	G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_CONTACTS,
	                              purple_demo_protocol_contacts_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC(PURPLE_TYPE_PROTOCOL_CONVERSATION,
	                              purple_demo_protocol_conversation_init))

static void
purple_demo_protocol_init(G_GNUC_UNUSED PurpleDemoProtocol *protocol) {
}

static void
purple_demo_protocol_class_finalize(G_GNUC_UNUSED PurpleDemoProtocolClass *klass) {
}

static void
purple_demo_protocol_class_init(PurpleDemoProtocolClass *klass) {
	PurpleProtocolClass *protocol_class = PURPLE_PROTOCOL_CLASS(klass);

	protocol_class->create_connection = purple_demo_protocol_create_connection;
	protocol_class->get_action_menu = purple_demo_protocol_get_action_menu;
}

/******************************************************************************
 * Local Exports
 *****************************************************************************/
void
purple_demo_protocol_register(GPluginNativePlugin *plugin) {
	purple_demo_protocol_register_type(G_TYPE_MODULE(plugin));
}

PurpleProtocol *
purple_demo_protocol_new(void) {
	return g_object_new(
		PURPLE_DEMO_TYPE_PROTOCOL,
		"id", "prpl-demo",
		"name", "Demo",
		"description", "A protocol plugin with static data to be used in "
		               "screen shots.",
		"icon-name", "im-purple-demo",
		"icon-resource-path", "/im/pidgin/purple/demo/icons",
		"options", OPT_PROTO_NO_PASSWORD,
		NULL);
}
