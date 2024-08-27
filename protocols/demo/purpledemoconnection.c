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

#include "purpledemoconnection.h"

#include "purpledemocontacts.h"

struct _PurpleDemoConnection {
	PurpleConnection parent;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(PurpleDemoConnection, purple_demo_connection,
                               PURPLE_TYPE_CONNECTION, G_TYPE_FLAG_FINAL, {})

/******************************************************************************
 * PurpleConnection Implementation
 *****************************************************************************/
static gboolean
purple_demo_connection_connect(PurpleConnection *connection,
                               G_GNUC_UNUSED GError **error)
{
	PurpleAccount *account = purple_connection_get_account(connection);
	PurpleContactInfo *info = purple_account_get_contact_info(account);
	const char *username = NULL;

	username = purple_account_get_username(account);
	purple_contact_info_set_id(info, username);
	purple_contact_info_set_username(info, username);

	purple_connection_set_state(connection, PURPLE_CONNECTION_STATE_CONNECTED);

	purple_demo_contacts_load(account);

	return TRUE;
}

static gboolean
purple_demo_connection_disconnect(G_GNUC_UNUSED PurpleConnection *connection,
                                  G_GNUC_UNUSED GError **error)
{
	return TRUE;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_demo_connection_init(G_GNUC_UNUSED PurpleDemoConnection *connection) {
}

static void
purple_demo_connection_class_finalize(G_GNUC_UNUSED PurpleDemoConnectionClass *klass) {
}

static void
purple_demo_connection_class_init(PurpleDemoConnectionClass *klass) {
	PurpleConnectionClass *connection_class = PURPLE_CONNECTION_CLASS(klass);

	connection_class->connect = purple_demo_connection_connect;
	connection_class->disconnect = purple_demo_connection_disconnect;
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_demo_connection_register(GPluginNativePlugin *plugin) {
	purple_demo_connection_register_type(G_TYPE_MODULE(plugin));
}
