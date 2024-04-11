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

#ifndef PURPLE_PROTOCOL_CLIENT_H
#define PURPLE_PROTOCOL_CLIENT_H

#include <glib.h>
#include <glib-object.h>

#include "connection.h"
#include "purpleaccount.h"
#include "purpleconversation.h"
#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_CLIENT (purple_protocol_client_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolClient, purple_protocol_client, PURPLE,
                    PROTOCOL_CLIENT, PurpleProtocol)

/**
 * PurpleProtocolClient:
 *
 * #PurpleProtocolClient interface defines the behavior of a typical chat
 * service's client interface.
 *
 * Since: 3.0
 */

struct _PurpleProtocolClientInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	void (*buddy_free)(PurpleProtocolClient *client, PurpleBuddy *buddy);

	void (*convo_closed)(PurpleProtocolClient *client, PurpleConnection *connection, const gchar *who);

	const gchar *(*normalize)(PurpleProtocolClient *client, PurpleAccount *account, const gchar *who);

	PurpleChat *(*find_blist_chat)(PurpleProtocolClient *client, PurpleAccount *account, const gchar *name);

	gboolean (*offline_message)(PurpleProtocolClient *client, PurpleBuddy *buddy);

	/*< private >*/
	gpointer reserved[4];
};

G_BEGIN_DECLS

/**
 * purple_protocol_client_buddy_free:
 * @client: The #PurpleProtocolClient instance.
 * @buddy: A #PurpleBuddy instance.
 *
 * Cleans up any protocol specific data for @buddy.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_client_buddy_free(PurpleProtocolClient *client, PurpleBuddy *buddy);

/**
 * purple_protocol_client_convo_closed:
 * @client: The #PurpleProtocolClient instance.
 * @connection: A #PurpleConnection instance.
 * @who: The name of the conversation to close.
 *
 * Closes the conversation named @who on connection @connection.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_client_convo_closed(PurpleProtocolClient *client, PurpleConnection *connection, const gchar *who);

/**
 * purple_protocol_client_normalize:
 * @client: The #PurpleProtocolClient instance.
 * @account: (nullable): A #PurpleAccount instance.
 * @who: The name to normalize.
 *
 * Normalizes a @who to the canonical form for the protocol.  For example, many
 * protocols only support all lower case, but might have display version where
 * there are capital letters.
 *
 * Returns: The normalized version of @who for @account.
 *
 * Since: 3.0
 *
 * Deprecated: 3.0: This should use purple_protocol_client_normalize_name when
 *             it is created which will return an allocated value.
 */
PURPLE_DEPRECATED
const gchar *purple_protocol_client_normalize(PurpleProtocolClient *client, PurpleAccount *account, const gchar *who);

/**
 * purple_protocol_client_find_blist_chat:
 * @client: The #PurpleProtocolClient instance.
 * @account: A #PurpleAccount instance.
 * @name: The name of the chat to find.
 *
 * Looks for a chat named @name in the contact list of @account.
 *
 * Returns: (transfer none): The #PurpleChat instance or %NULL if no chat could
 *          be found.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleChat *purple_protocol_client_find_blist_chat(PurpleProtocolClient *client, PurpleAccount *account, const gchar *name);

/**
 * purple_protocol_client_offline_message:
 * @client: The #PurpleProtocolClient instance.
 * @buddy: A #PurpleBuddy instance.
 *
 * Checks whether offline messages to @buddy are supported.
 *
 * Returns: %TRUE if @buddy supports offline messages, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_protocol_client_offline_message(PurpleProtocolClient *client, PurpleBuddy *buddy);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_CLIENT_H */
