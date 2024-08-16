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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_PROTOCOL_CONTACTS_H
#define PURPLE_PROTOCOL_CONTACTS_H

#include <glib.h>
#include <glib-object.h>

#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_CONTACTS (purple_protocol_contacts_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolContacts,
                    purple_protocol_contacts, PURPLE,
                    PROTOCOL_CONTACTS, PurpleProtocol)

/**
 * PurpleProtocolContacts:
 *
 * #PurpleProtocolContacts provides methods for interacting with remote
 * contacts.
 *
 * Since: 3.0
 */

/**
 * PURPLE_PROTOCOL_CONTACTS_DOMAIN:
 *
 * A domain for errors from the [class@ProtocolContacts] API.
 *
 * Since: 3.0
 */
#define PURPLE_PROTOCOL_CONTACTS_DOMAIN \
	g_quark_from_static_string("purple-protocol-contacts") \
	PURPLE_AVAILABLE_MACRO_IN_3_0

struct _PurpleProtocolContactsInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	guint (*get_minimum_search_length)(PurpleProtocolContacts *protocol_contacts);
	void (*search_async)(PurpleProtocolContacts *protocol_contacts, PurpleAccount *account, const char *text, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	GListModel *(*search_finish)(PurpleProtocolContacts *protocol_contacts, GAsyncResult *result, GError **error);

	void (*get_profile_async)(PurpleProtocolContacts *protocol_contacts, PurpleContactInfo *info, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	char *(*get_profile_finish)(PurpleProtocolContacts *protocol_contacts, GAsyncResult *result, GError **error);

	GActionGroup *(*get_actions)(PurpleProtocolContacts *protocol_contacts, PurpleContactInfo *info);
	GMenuModel *(*get_menu)(PurpleProtocolContacts *protocol_contacts, PurpleContactInfo *info);

	/*< private >*/
	gpointer reserved[4];
};

G_BEGIN_DECLS

/**
 * purple_protocol_contacts_implements_search:
 * @protocol_contacts: The instance.
 *
 * Checks if @protocol_contacts implements
 * [vfunc@ProtocolContacts.search_async] and
 * [vfunc@ProtocolContacts.search_finish].
 *
 * Returns: %TRUE if the search interface is implemented, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_contacts_implements_search(PurpleProtocolContacts *protocol_contacts);

/**
 * purple_protocol_contacts_get_minimum_search_length:
 * @protocol_contacts: The instance.
 *
 * Gets the minimum length of the search term before
 * [method@ProtocolContacts.search_async] should be called.
 *
 * The default implementation returns 3.
 *
 * Returns: The minimum length of the search term.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
guint purple_protocol_contacts_get_minimum_search_length(PurpleProtocolContacts *protocol_contacts);

/**
 * purple_protocol_contacts_search_async:
 * @protocol_contacts: The instance.
 * @account: The [class@Account] to search under.
 * @text: The text to search for which must not be an empty string.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is
 *            satisfied.
 * @data: User data to pass to @callback.
 *
 * Starts the process of searching for contacts using @account that match
 * @text.
 *
 * Call [method@ProtocolContacts.search_finish] to get the results.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_contacts_search_async(PurpleProtocolContacts *protocol_contacts, PurpleAccount *account, const char *text, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_contacts_search_finish:
 * @protocol_contacts: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolContacts.search_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@ProtocolContacts.search_async] and
 * gets the result.
 *
 * Returns: (transfer full): A [iface@Gio.ListModel] of the matched contacts or
 *          %NULL with @error set on error.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GListModel *purple_protocol_contacts_search_finish(PurpleProtocolContacts *protocol_contacts, GAsyncResult *result, GError **error);

/**
 * purple_protocol_contacts_get_profile_async:
 * @protocol_contacts: The instance.
 * @info: The [class@ContactInfo] whose profile to get.
 * @cancellable: (nullable): optional GCancellable object, %NULL to ignore.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is
 *            satisfied.
 * @data: User data to pass to @callback.
 *
 * Starts the process requesting the user profile for @info.
 *
 * Call [method@ProtocolContacts.get_profile_finish] to get the results.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_contacts_get_profile_async(PurpleProtocolContacts *protocol_contacts, PurpleContactInfo *info, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_protocol_contacts_get_profile_finish:
 * @protocol_contacts: The instance.
 * @result: The [iface@Gio.AsyncResult] from the previous
 *          [method@ProtocolContacts.get_profile_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@ProtocolContacts.get_profile_async] and
 * gets the result.
 *
 * Returns: (transfer full): A plain text or markdown formatted string of the
 *          contact info's profile, or %NULL with @error set on error.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
char *purple_protocol_contacts_get_profile_finish(PurpleProtocolContacts *protocol_contacts, GAsyncResult *result, GError **error);

/**
 * purple_protocol_contacts_get_actions:
 * @protocol_contacts: The instance.
 * @info: The [class@ContactInfo] to get the actions for.
 *
 * Gets a [iface@Gio.ActionGroup] for @info. When this action group is used,
 * it should use the prefix of `contact`.
 *
 * Returns: (transfer full): The action group or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GActionGroup *purple_protocol_contacts_get_actions(PurpleProtocolContacts *protocol_contacts, PurpleContactInfo *info);

/**
 * purple_protocol_contacts_get_menu:
 * @protocol_contacts: The instance.
 * @info: The [class@ContactInfo] to get the menu for.
 *
 * Gets a [class@Gio.MenuModel] for @info.
 *
 * Returns: (transfer full): The menu model or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GMenuModel *purple_protocol_contacts_get_menu(PurpleProtocolContacts *protocol_contacts, PurpleContactInfo *info);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_CONTACTS_H */
