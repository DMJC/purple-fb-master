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

#ifndef PURPLE_CREDENTIAL_MANAGER_H
#define PURPLE_CREDENTIAL_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "purpleaccount.h"
#include "purplecredentialprovider.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PURPLE_CREDENTIAL_MANAGER_DOMAIN:
 *
 * A #GError domain for errors from #PurpleCredentialManager.
 *
 * Since: 3.0
 */
#define PURPLE_CREDENTIAL_MANAGER_DOMAIN \
	g_quark_from_static_string("purple-credential-manager") \
	PURPLE_AVAILABLE_MACRO_IN_3_0

#define PURPLE_TYPE_CREDENTIAL_MANAGER (purple_credential_manager_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleCredentialManager, purple_credential_manager,
                     PURPLE, CREDENTIAL_MANAGER, GObject)

/**
 * PurpleCredentialManager:
 *
 * Purple Credential Manager is the main API access to different credential
 * providers. Providers register themselves with the manager and then the user
 * can choose which provider to use.
 *
 * Once a provider is selected, all credential access will be directed to that
 * provider.
 *
 * Since: 3.0
 */

/**
 * PurpleCredentialManagerForeachFunc:
 * @provider: The #PurpleCredentialProvider instance.
 * @data: User supplied data.
 *
 * A function to be used as a callback with purple_credential_manager_foreach().
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_TYPE_IN_3_0
typedef void (*PurpleCredentialManagerForeachFunc)(PurpleCredentialProvider *provider, gpointer data);

/**
 * purple_credential_manager_get_default:
 *
 * Gets the default [class@CredentialManager] instance.
 *
 * Returns: (transfer none): The default #PurpleCredentialManager instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleCredentialManager *purple_credential_manager_get_default(void);

/**
 * purple_credential_manager_register:
 * @manager: The instance.
 * @provider: The provider to register.
 * @error: Return address for a #GError, or %NULL.
 *
 * Registers @provider with @manager.
 *
 * Returns: %TRUE if @provider was successfully registered with @manager,
 *          %FALSE otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_credential_manager_register(PurpleCredentialManager *manager, PurpleCredentialProvider *provider, GError **error);

/**
 * purple_credential_manager_unregister:
 * @manager: The instance.
 * @provider: The provider to unregister.
 * @error: Return address for a #GError, or %NULL.
 *
 * Unregisters @provider from @manager.
 *
 * Returns: %TRUE if @provider was successfully unregistered from @provider,
 *          %FALSE otherwise.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_credential_manager_unregister(PurpleCredentialManager *manager, PurpleCredentialProvider *provider, GError **error);

/**
 * purple_credential_manager_set_active:
 * @manager: The instance.
 * @id: (nullable): The id of the provider to use or %NULL to disable the
 *      active provider.
 * @error: Return address for a #GError, or %NULL.
 *
 * Changes the active provider of @manager to the provider with an id of @id.
 *
 * If @id is %NULL, it is assumed that the process is shutting down and that
 * the active provider setting will be left intact. If you want to disable the
 * active provider and keep running, you should set the active provider to
 * [class@Purple.NoopCredentialProvider].
 *
 * Returns: %TRUE on success or %FALSE with @error set on failure.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_credential_manager_set_active(PurpleCredentialManager *manager, const char *id, GError **error);

/**
 * purple_credential_manager_get_active:
 * @manager: The instance.
 *
 * Gets the currently active [class@CredentialProvider] or %NULL if there is no
 * active provider.
 *
 * Returns: (transfer none): The active provider.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleCredentialProvider *purple_credential_manager_get_active(PurpleCredentialManager *manager);

/**
 * purple_credential_manager_read_password_async:
 * @manager: The instance.
 * @account: The associated account.
 * @cancellable: (nullable): An optional cancellable.
 * @callback: (nullable) (scope async): A callback to call when the request is
 *            satisfied.
 * @data: User data to pass to @callback.
 *
 * Reads the password for @account using the active provider of @manager.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_credential_manager_read_password_async(PurpleCredentialManager *manager, PurpleAccount *account, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_credential_manager_read_password_finish:
 * @manager: The instance.
 * @result: The result from the previous
 *          [method@CredentialManager.read_password_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@CredentialManager.read_password_async].
 *
 * Returns: (transfer full): The password or %NULL if successful, otherwise
 *                           %NULL with @error set on failure.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
char *purple_credential_manager_read_password_finish(PurpleCredentialManager *manager, GAsyncResult *result, GError **error);

/**
 * purple_credential_manager_write_password_async:
 * @manager: The instance.
 * @account: The account whose password to write.
 * @password: The password to write.
 * @cancellable: (nullable): optional cancellable.
 * @callback: (nullable) (scope async): The callback to call when the request
 *            is satisfied.
 * @data: User data to pass to @callback.
 *
 * Writes @password for @account to the active provider of @manager.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_credential_manager_write_password_async(PurpleCredentialManager *manager, PurpleAccount *account, const char *password, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_credential_manager_write_password_finish:
 * @manager: The instance.
 * @result: The result from the previous
 *          [method@CredentialManager.write_password_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@CredentialManager.write_password_async].
 *
 * Returns: %TRUE if the password was written successfully, otherwise %FALSE
 *          with @error set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_credential_manager_write_password_finish(PurpleCredentialManager *manager, GAsyncResult *result, GError **error);

/**
 * purple_credential_manager_clear_password_async:
 * @manager: The instance.
 * @account: The account whose password to clear.
 * @cancellable: (nullable): An optional cancellable.
 * @callback: (scope async): A callback to call when the request is satisfied.
 * @data: User data to pass to @callback.
 *
 * Clears the password for @account from the active provider of @manager.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_credential_manager_clear_password_async(PurpleCredentialManager *manager, PurpleAccount *account, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_credential_manager_clear_password_finish:
 * @manager: The instance.
 * @result: The result from the previous
 *          [method@CredentialManager.clear_password_async] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@CredentialManager.clear_password_async].
 *
 * Returns: %TRUE if the password didn't exist or was cleared successfully,
 *          otherwise %FALSE with @error set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_credential_manager_clear_password_finish(PurpleCredentialManager *manager, GAsyncResult *result, GError **error);

/**
 * purple_credential_manager_foreach:
 * @manager: The instance.
 * @func: (scope call): The [type@CredentialManagerForeachFunc] to call.
 * @data: User data to pass to @func.
 *
 * Calls @func for each #PurpleCredentialProvider that @manager knows about.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_credential_manager_foreach(PurpleCredentialManager *manager, PurpleCredentialManagerForeachFunc func, gpointer data);

G_END_DECLS

#endif /* PURPLE_CREDENTIAL_MANAGER_H */
