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

#include "purplebonjourconnection.h"

#include "purplebonjourconstants.h"
#include "purplebonjourcore.h"

struct _PurpleBonjourConnection {
	PurpleConnection parent;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(PurpleBonjourConnection, purple_bonjour_connection,
                               PURPLE_TYPE_CONNECTION, G_TYPE_FLAG_FINAL, {})

/******************************************************************************
 * PurpleConnection Implementation
 *****************************************************************************/
static gboolean
purple_bonjour_connection_connect(PurpleConnection *purple_connection,
                                  G_GNUC_UNUSED GError **error)
{
	G_GNUC_UNUSED PurpleBonjourConnection *connection = NULL;
	PurpleAccount *account = NULL;
	G_GNUC_UNUSED GCancellable *cancellable = NULL;
	G_GNUC_UNUSED const char *username = NULL;
	G_GNUC_UNUSED guint16 port = PURPLE_BONJOUR_DEFAULT_PORT;

	g_return_val_if_fail(PURPLE_BONJOUR_IS_CONNECTION(purple_connection),
	                     FALSE);

	purple_connection_set_state(purple_connection,
	                            PURPLE_CONNECTION_STATE_CONNECTING);

	connection = PURPLE_BONJOUR_CONNECTION(purple_connection);
	cancellable = purple_connection_get_cancellable(purple_connection);
	account = purple_connection_get_account(purple_connection);

	/* Grab some more information that we need. */
	username = purple_contact_info_get_username(PURPLE_CONTACT_INFO(account));
	port = purple_account_get_int(account, PURPLE_BONJOUR_OPTION_PORT,
	                              PURPLE_BONJOUR_DEFAULT_PORT);

	/* Eventually, we will do something before this, but we can go straight to
	 * connected for now. */
	purple_connection_set_state(purple_connection,
	                            PURPLE_CONNECTION_STATE_CONNECTED);

	return TRUE;
}

static gboolean
purple_bonjour_connection_disconnect(PurpleConnection *purple_connection,
                                     GError **error)
{
	PurpleConnectionClass *parent_class = NULL;

	g_return_val_if_fail(PURPLE_BONJOUR_IS_CONNECTION(purple_connection),
	                     FALSE);

	/* Chain up to our parent's disconnect method. */
	parent_class = PURPLE_CONNECTION_CLASS(purple_bonjour_connection_parent_class);
	return parent_class->disconnect(purple_connection, error);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
purple_bonjour_connection_init(G_GNUC_UNUSED PurpleBonjourConnection *connection) {
}

static void
purple_bonjour_connection_class_init(PurpleBonjourConnectionClass *klass) {
	PurpleConnectionClass *connection_class = PURPLE_CONNECTION_CLASS(klass);

	connection_class->connect = purple_bonjour_connection_connect;
	connection_class->disconnect = purple_bonjour_connection_disconnect;
}

static void
purple_bonjour_connection_class_finalize(G_GNUC_UNUSED PurpleBonjourConnectionClass *klass) {
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_bonjour_connection_register(GPluginNativePlugin *plugin) {
	purple_bonjour_connection_register_type(G_TYPE_MODULE(plugin));
}
