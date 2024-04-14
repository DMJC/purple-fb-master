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

#ifndef PURPLE_ACCOUNT_H
#define PURPLE_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>

#define PURPLE_TYPE_ACCOUNT (purple_account_get_type())

typedef struct _PurpleAccount PurpleAccount;

#include "connection.h"
#include "purpleconnectionerrorinfo.h"
#include "purplecontactinfo.h"
#include "purpleprotocol.h"
#include "purpleproxyinfo.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleAccount:
 *
 * A #PurpleAccount is the core configuration for connecting to a specific
 * account. User interfaces typically allow users to create these in a dialog
 * or wizard.
 *
 * Since: 2.0
 */

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleAccount, purple_account, PURPLE, ACCOUNT,
                     PurpleContactInfo)

/**
 * purple_account_new:
 * @username: The username.
 * @protocol_id: The protocol ID.
 *
 * Creates a new account.
 *
 * Returns: The new account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleAccount *purple_account_new(const char *username, const char *protocol_id);

/**
 * purple_account_connect:
 * @account: The account to connect to.
 *
 * Connects to an account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_connect(PurpleAccount *account);

/**
 * purple_account_disconnect:
 * @account: The account to disconnect from.
 *
 * Disconnects from an account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_disconnect(PurpleAccount *account);

/**
 * purple_account_is_disconnecting:
 * @account: The account
 *
 * Indicates if the account is currently being disconnected.
 *
 * Returns: TRUE if the account is being disconnected.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_account_is_disconnecting(PurpleAccount *account);

/**
 * purple_account_request_close_with_account:
 * @account: The account for which requests should be closed
 *
 * Close account requests registered for the given PurpleAccount
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_request_close_with_account(PurpleAccount *account);

/**
 * purple_account_request_close:
 * @ui_handle: The ui specific handle for which requests should be closed
 *
 * Close the account request for the given ui handle
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_request_close(void *ui_handle);

/**
 * purple_account_request_password:
 * @account: The account to request the password for.
 * @ok_cb: (scope call): The callback for the OK button.
 * @cancel_cb: (scope call): The callback for the cancel button.
 * @user_data: User data to be passed into callbacks.
 *
 * Requests a password from the user for the account. Does not set the
 * account password on success; do that in ok_cb if desired.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_request_password(PurpleAccount *account, GCallback ok_cb, GCallback cancel_cb, void *user_data);

/**
 * purple_account_request_change_password:
 * @account: The account to change the password on.
 *
 * Requests information from the user to change the account's password.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_request_change_password(PurpleAccount *account);

/**
 * purple_account_request_change_user_info:
 * @account: The account to change the user information on.
 *
 * Requests information from the user to change the account's
 * user information.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_request_change_user_info(PurpleAccount *account);

/**
 * purple_account_set_user_info:
 * @account: The account.
 * @user_info: The user information.
 *
 * Sets the account's user information
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_user_info(PurpleAccount *account, const char *user_info);

/**
 * purple_account_set_buddy_icon_path:
 * @account: The account.
 * @path: The buddy icon non-cached path.
 *
 * Sets the account's buddy icon path.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_buddy_icon_path(PurpleAccount *account, const char *path);

/**
 * purple_account_set_protocol_id:
 * @account: The account.
 * @protocol_id: The protocol ID.
 *
 * Sets the account's protocol ID.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_protocol_id(PurpleAccount *account, const char *protocol_id);

/**
 * purple_account_set_connection:
 * @account: The account.
 * @gc: The connection.
 *
 * Sets the account's connection.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_connection(PurpleAccount *account, PurpleConnection *gc);

/**
 * purple_account_set_remember_password:
 * @account: The account.
 * @value: %TRUE if it should remember the password.
 *
 * Sets whether or not this account should save its password.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_remember_password(PurpleAccount *account, gboolean value);

/**
 * purple_account_set_enabled:
 * @account: The account.
 * @value: %TRUE if it is enabled.
 *
 * Sets whether or not this account is enabled.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_enabled(PurpleAccount *account, gboolean value);

/**
 * purple_account_set_proxy_info:
 * @account: The account.
 * @info: The proxy information.
 *
 * Sets the account's proxy information.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_proxy_info(PurpleAccount *account, PurpleProxyInfo *info);

/**
 * purple_account_set_int:
 * @account: The account.
 * @name: The name of the setting.
 * @value: The setting's value.
 *
 * Sets a protocol-specific integer setting for an account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_int(PurpleAccount *account, const char *name, int value);

/**
 * purple_account_set_string:
 * @account: The account.
 * @name: The name of the setting.
 * @value: The setting's value.
 *
 * Sets a protocol-specific string setting for an account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_string(PurpleAccount *account, const char *name, const char *value);

/**
 * purple_account_set_bool:
 * @account: The account.
 * @name: The name of the setting.
 * @value: The setting's value.
 *
 * Sets a protocol-specific boolean setting for an account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_set_bool(PurpleAccount *account, const char *name, gboolean value);

/**
 * purple_account_is_connected:
 * @account: The account.
 *
 * Returns whether or not the account is connected.
 *
 * Returns: %TRUE if connected, or %FALSE otherwise.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_account_is_connected(PurpleAccount *account);

/**
 * purple_account_is_connecting:
 * @account: The account.
 *
 * Returns whether or not the account is connecting.
 *
 * Returns: %TRUE if connecting, or %FALSE otherwise.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_account_is_connecting(PurpleAccount *account);

/**
 * purple_account_is_disconnected:
 * @account: The account.
 *
 * Returns whether or not the account is disconnected.
 *
 * Returns: %TRUE if disconnected, or %FALSE otherwise.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_account_is_disconnected(PurpleAccount *account);

/**
 * purple_account_get_user_info:
 * @account: The account.
 *
 * Returns the account's user information.
 *
 * Returns: The user information.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_get_user_info(PurpleAccount *account);

/**
 * purple_account_get_buddy_icon_path:
 * @account: The account.
 *
 * Gets the account's buddy icon path.
 *
 * Returns: The buddy icon's non-cached path.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_get_buddy_icon_path(PurpleAccount *account);

/**
 * purple_account_get_protocol_id:
 * @account: The account.
 *
 * Returns the account's protocol ID.
 *
 * Returns: The protocol ID.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_get_protocol_id(PurpleAccount *account);

/**
 * purple_account_get_protocol:
 * @account: The #PurpleAccount instance.
 *
 * Gets the #PurpleProtocol instance for @account.
 *
 * Returns: (transfer none): The #PurpleProtocol for @account or %NULL if it
 *          could not be found.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleProtocol *purple_account_get_protocol(PurpleAccount *account);

/**
 * purple_account_get_protocol_name:
 * @account: The account.
 *
 * Returns the account's protocol name.
 *
 * Returns: The protocol name.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_get_protocol_name(PurpleAccount *account);

/**
 * purple_account_get_connection:
 * @account: The account.
 *
 * Returns the account's connection.
 *
 * Returns: (transfer none): The connection.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleConnection *purple_account_get_connection(PurpleAccount *account);

/**
 * purple_account_get_remember_password:
 * @account: The account.
 *
 * Returns whether or not this account should save its password.
 *
 * Returns: %TRUE if it should remember the password.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_account_get_remember_password(PurpleAccount *account);

/**
 * purple_account_get_enabled:
 * @account: The account.
 *
 * Returns whether or not this account is enabled.
 *
 * Returns: %TRUE if it enabled on this UI.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_account_get_enabled(PurpleAccount *account);

/**
 * purple_account_get_proxy_info:
 * @account: The account.
 *
 * Returns the account's proxy information.
 *
 * Returns: (transfer none): The proxy information.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleProxyInfo *purple_account_get_proxy_info(PurpleAccount *account);

/**
 * purple_account_get_presence:
 * @account: The account.
 *
 * Returns the account's presence.
 *
 * Returns: (transfer none): The account's presence.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurplePresence *purple_account_get_presence(PurpleAccount *account);

/**
 * purple_account_get_int:
 * @account: The account.
 * @name: The name of the setting.
 * @default_value: The default value.
 *
 * Returns a protocol-specific integer setting for an account.
 *
 * Returns: The value.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
int purple_account_get_int(PurpleAccount *account, const char *name, int default_value);

/**
 * purple_account_get_string:
 * @account: The account.
 * @name: The name of the setting.
 * @default_value: The default value.
 *
 * Returns a protocol-specific string setting for an account.
 *
 * Returns: The value.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
const char *purple_account_get_string(PurpleAccount *account, const char *name, const char *default_value);

/**
 * purple_account_get_bool:
 * @account: The account.
 * @name: The name of the setting.
 * @default_value: The default value.
 *
 * Returns a protocol-specific boolean setting for an account.
 *
 * Returns: The value.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_account_get_bool(PurpleAccount *account, const char *name, gboolean default_value);

/**
 * purple_account_change_password:
 * @account: The account.
 * @orig_pw: The old password.
 * @new_pw: The new password.
 *
 * Changes the password on the specified account.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_account_change_password(PurpleAccount *account, const char *orig_pw, const char *new_pw);

/**
 * purple_account_get_error:
 * @account: The account whose error should be retrieved.
 *
 * Get the error that caused the account to be disconnected, or %NULL if the
 * account is happily connected or disconnected without an error.
 *
 * Returns: (transfer none): The type of error and a human-readable description
 *          of the current error, or %NULL if there is no current error.  This
 *          pointer is guaranteed to remain valid until the @ref
 *          account-error-changed signal is emitted for @account.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const PurpleConnectionErrorInfo *purple_account_get_error(PurpleAccount *account);

/**
 * purple_account_set_error:
 * @account: The account whose error should be set.
 * @info: (nullable) (transfer full): The [struct@Purple.ConnectionErrorInfo]
 *        to set.
 *
 * Sets the error of @account to @info. Note that setting this won't disconnect
 * the account. This is intended to be called by libpurple when there is a
 * connection failure, when invalid settings are entered in an account editor,
 * or similar situations.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_account_set_error(PurpleAccount *account, PurpleConnectionErrorInfo *info);

/**
 * purple_account_set_require_password:
 * @account: The instance.
 * @require_password: Whether or not this account should require a password.
 *
 * For protocols that have an optional password, this settings tells libpurple
 * that it should look for a password in the [class@Purple.CredentialManager]
 * or prompt the user if a password can not be found.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_account_set_require_password(PurpleAccount *account, gboolean require_password);

/**
 * purple_account_get_require_password:
 * @account: The instance.
 *
 * Gets whether or not @account requires a password.
 *
 * Returns: %TRUE if the account requires a password, %FALSE otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_account_get_require_password(PurpleAccount *account);

/**
 * purple_account_freeze_notify_settings:
 * @account: The instance.
 *
 * Increment the freeze count for settings on @account. When the freeze count
 * is greater than 0, the [signal@Purple.Account::setting-changed] signal will
 * not be emitted until the freeze count returns to 0.
 *
 * This is intended to only notify once @account has reached a consistent
 * state. Most user interfaces change all of the properties on an account at
 * once and some of them may be co-dependent, so all values need to be updated
 * before the change can be acted upon.
 *
 * Call [method@Purple.Account.thaw_notify_settings] to decrement the freeze
 * counter.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_account_freeze_notify_settings(PurpleAccount *account);

/**
 * purple_account_thaw_notify_settings:
 * @account: The instance.
 *
 * Decrements the freeze count for settings on @account.
 *
 * See [method@Purple.Account.freeze_notify_settings] for more information.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_account_thaw_notify_settings(PurpleAccount *account);

G_END_DECLS

#endif /* PURPLE_ACCOUNT_H */

