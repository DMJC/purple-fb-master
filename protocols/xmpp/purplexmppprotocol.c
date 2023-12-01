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

#include "purplexmppprotocol.h"

struct _PurpleXmppProtocol {
	PurpleProtocol parent;
};

/******************************************************************************
 * PurpleProtocol Implementation
 *****************************************************************************/
static GList *
purple_xmpp_protocol_get_user_splits(G_GNUC_UNUSED PurpleProtocol *protocol) {
	PurpleAccountUserSplit *split = NULL;
	GList *splits = NULL;

	split = purple_account_user_split_new(_("Server"), NULL, '@');
	splits = g_list_append(splits, split);

	return splits;
}

static GList *
purple_xmpp_protocol_status_types(G_GNUC_UNUSED PurpleProtocol *protocol,
                                  G_GNUC_UNUSED PurpleAccount *account)
{
	PurpleStatusType *type = NULL;
	GList *types = NULL;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_DYNAMIC_TYPE_EXTENDED(
	PurpleXmppProtocol,
	purple_xmpp_protocol,
	PURPLE_TYPE_PROTOCOL,
	0,
	)

static void
purple_xmpp_protocol_init(G_GNUC_UNUSED PurpleXmppProtocol *protocol) {
}

static void
purple_xmpp_protocol_class_finalize(G_GNUC_UNUSED PurpleXmppProtocolClass *klass) {
}

static void
purple_xmpp_protocol_class_init(PurpleXmppProtocolClass *klass) {
	PurpleProtocolClass *protocol_class = PURPLE_PROTOCOL_CLASS(klass);

	protocol_class->get_user_splits = purple_xmpp_protocol_get_user_splits;
	protocol_class->status_types = purple_xmpp_protocol_status_types;
}

/******************************************************************************
 * Internal API
 *****************************************************************************/
void
purple_xmpp_protocol_register(GPluginNativePlugin *plugin) {
	purple_xmpp_protocol_register_type(G_TYPE_MODULE(plugin));
}

PurpleProtocol *
purple_xmpp_protocol_new(void) {
	return g_object_new(
		PURPLE_XMPP_TYPE_PROTOCOL,
		"id", "prpl-xmpp",
		"name", "XMPP",
		"description", _("Modern Extensible Messaging and Presence Protocol."),
		"icon-name", "im-xmpp",
		"icon-resource-path", "/im/pidgin/libpurple/protocols/xmpp/icons",
		"options", OPT_PROTO_PASSWORD_OPTIONAL,
		NULL);
}
