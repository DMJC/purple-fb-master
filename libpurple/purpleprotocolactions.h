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

#ifndef PURPLE_PROTOCOL_ACTIONS_H
#define PURPLE_PROTOCOL_ACTIONS_H

#include <glib.h>
#include <glib-object.h>

#include "connection.h"
#include "purpleprotocol.h"
#include "purpleversion.h"

#define PURPLE_TYPE_PROTOCOL_ACTIONS (purple_protocol_actions_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PurpleProtocolActions, purple_protocol_actions, PURPLE,
                    PROTOCOL_ACTIONS, PurpleProtocol)

/**
 * PurpleProtocolActions:
 *
 * The #PurpleProtocolActions interface defines the behavior of a protocol's
 * actions interface.
 *
 * Since: 3.0
 */

struct _PurpleProtocolActionsInterface {
	/*< private >*/
	GTypeInterface parent;

	/*< public >*/
	const gchar *(*get_prefix)(PurpleProtocolActions *actions);

	GActionGroup *(*get_action_group)(PurpleProtocolActions *actions, PurpleConnection *connection);

	GMenu *(*get_menu)(PurpleProtocolActions *actions, PurpleConnection *connection);

	/*< private >*/
	gpointer reserved[4];
};

G_BEGIN_DECLS

/**
 * purple_protocol_actions_get_prefix:
 * @actions: The PurpleProtocolActions instance.
 *
 * The prefix that should be used when inserting the action group into widgets.
 *
 * Returns: Gets the prefix for the name of the actions in @actions.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const gchar *purple_protocol_actions_get_prefix(PurpleProtocolActions *actions);

/**
 * purple_protocol_actions_get_action_group:
 * @actions: The PurpleProtocolActions instance.
 * @connection: The [class@Connection] instance.
 *
 * Gets a group of actions for @connection.
 *
 * Returns: (transfer full): The group of actions for @connection.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GActionGroup *purple_protocol_actions_get_action_group(PurpleProtocolActions *actions, PurpleConnection *connection);

/**
 * purple_protocol_actions_get_menu:
 * @actions: The [iface@ProtocolActions] instance.
 * @connection: (nullable): The [class@Connection] instance.
 *
 * Gets the menu used to display the protocol actions for @connection.
 *
 * Returns: (transfer full): The menu to display or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GMenu *purple_protocol_actions_get_menu(PurpleProtocolActions *actions, PurpleConnection *connection);

/**
 * purple_protocol_actions_changed:
 * @actions: The instance.
 * @account: The [class@Account] whose actions changed.
 *
 * Emits the [signal@ProtocolActions::actions-changed] signal. This is meant to
 * be called by [iface@ProtocolActions] implementations when actions have
 * changed.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_protocol_actions_changed(PurpleProtocolActions *actions, PurpleAccount *account);

G_END_DECLS

#endif /* PURPLE_PROTOCOL_ACTIONS_H */
