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

#ifndef PURPLE_PRIVATE_H
#define PURPLE_PRIVATE_H

#include <glib.h>
#include <glib/gstdio.h>

#include "core.h"
#include "accounts.h"
#include "connection.h"
#include "purplecredentialprovider.h"
#include "purplehistoryadapter.h"
#include "purpleversion.h"
#include "xmlnode.h"

G_BEGIN_DECLS

/**
 * _purple_account_to_xmlnode:
 * @account:  The account
 *
 * Get an XML description of an account.
 *
 * Returns:  The XML description of the account.
 */
G_GNUC_INTERNAL
PurpleXmlNode *_purple_account_to_xmlnode(PurpleAccount *account);

/**
 * _purple_connection_wants_to_die:
 * @gc:  The connection to check
 *
 * Checks if a connection is disconnecting, and should not attempt to reconnect.
 *
 * Note: This function should only be called by purple_account_set_enabled()
 *       in account.c.
 */
G_GNUC_INTERNAL
gboolean _purple_connection_wants_to_die(PurpleConnection *gc);

/**
 * purple_account_manager_startup:
 *
 * Starts up the account manager by creating the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_account_manager_startup(void);

/**
 * purple_account_manager_shutdown:
 *
 * Shuts down the account manager by destroying the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_account_manager_shutdown(void);

/**
 * purple_contact_manager_startup:
 *
 * Starts up the contact manager by creating the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_contact_manager_startup(void);

/**
 * purple_contact_manager_shutdown:
 *
 * Shuts down the contact manager by destroying the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_contact_manager_shutdown(void);

/**
 * purple_conversation_manager_startup:
 *
 * Starts up the conversation manager by creating the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_conversation_manager_startup(void);

/**
 * purple_conversation_manager_shutdown:
 *
 * Shuts down the conversation manager by destroying the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_conversation_manager_shutdown(void);

/**
 * purple_credential_manager_startup:
 * @core: The core instance.
 *
 * Starts up the credential manager by creating the default instance for @core.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_credential_manager_startup(PurpleCore *core);

/**
 * purple_credential_manager_shutdown:
 * @core: The core instance.
 *
 * Shuts down the credential manager by destroying the default instance for
 * @core.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_credential_manager_shutdown(PurpleCore *core);

/**
 * purple_protocol_manager_startup:
 *
 * Starts up the protocol manager by creating the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_protocol_manager_startup(void);

/**
 * purple_protocol_manager_shutdown:
 *
 * Shuts down the protocol manager by destroying the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_protocol_manager_shutdown(void);

/**
 * purple_credential_provider_activate:
 * @provider: The #PurpleCredentialProvider instance.
 *
 * Tells a @provider that it has become the active provider.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_credential_provider_activate(PurpleCredentialProvider *provider);

/**
 * purple_credential_provider_deactivate:
 * @provider: The #PurpleCredentialProvider instance.
 *
 * Tells @provider that another #PurpleCredentialProvider has become the active
 * provider.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_credential_provider_deactivate(PurpleCredentialProvider *provider);

/**
 * purple_history_adapter_activate:
 * @adapter: The #PurpleHistoryAdapter instance.
 * @error: A return address for a #GError.
 *
 * Asks @adapter to become the active adapter. If @adapter can not become active
 * it should return %FALSE and set @error.
 *
 * NOTE: This is public only for tests. Do not use outside of libpurple.
 *
 * Returns: %TRUE on success otherwise %FALSE with @error set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_history_adapter_activate(PurpleHistoryAdapter *adapter, GError **error);

/**
 * purple_history_adapter_deactivate:
 * @adapter: The #PurpleHistoryAdapter instance.
 * @error: A return address for a #GError.
 *
 * Asks @adapter to stop being the active adapter. If @adapter can not
 * deactivate it should return %FALSE and set @error.
 *
 * NOTE: This is public only for tests. Do not use outside of libpurple.
 *
 * Returns: %TRUE on success otherwise %FALSE with @error set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_history_adapter_deactivate(PurpleHistoryAdapter *adapter, GError **error);

/**
 * purple_notification_manager_startup:
 *
 * Starts up the notification manager by creating the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_notification_manager_startup(void);

/**
 * purple_notification_manager_shutdown:
 *
 * Shuts down the notification manager by destroying the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_notification_manager_shutdown(void);

/**
 * purple_whiteboard_manager_startup:
 *
 * Starts up the whiteboard manager by creating the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_whiteboard_manager_startup(void);

/**
 * purple_whiteboard_manager_shutdown:
 *
 * Shuts down the whiteboard manager by destroying the default instance.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_whiteboard_manager_shutdown(void);

/**
 * purple_account_set_enabled_plain:
 * @account: The instance.
 * @enabled: Whether or not the account is enabled.
 *
 * This is a temporary method until we overhaul serialization of accounts.
 *
 * This method sets the enabled state of an account without any side effects.
 * Its primary usage is when loading accounts from disk, as without this, the
 * account attempts to connect immediately.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL
void purple_account_set_enabled_plain(PurpleAccount *account, gboolean enabled);

G_END_DECLS

#endif /* PURPLE_PRIVATE_H */

