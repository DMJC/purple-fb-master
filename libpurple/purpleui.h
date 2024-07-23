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

#ifndef PURPLE_UI_H
#define PURPLE_UI_H

#include <glib.h>
#include <glib-object.h>

#include "purpleconversationmanager.h"
#include "purplehistoryadapter.h"
#include "purplepresencemanager.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PURPLE_UI_ERROR:
 *
 * A GError domain for PurpleUi errors.
 *
 * Since: 3.0
 */
#define PURPLE_UI_ERROR (g_quark_from_static_string("purple-ui"))

#define PURPLE_TYPE_UI (purple_ui_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_DERIVABLE_TYPE(PurpleUi, purple_ui, PURPLE, UI, GObject)

/**
 * PurpleUi:
 *
 * An abstract class representing a user interface. All user interfaces must
 * create a subclass of this and pass it to [func@Purple.Core.init].
 *
 * Since: 3.0
 */

/**
 * PurpleUiClass:
 * @prefs_init: This function is called to initialize the ui's preferences.
 *              This is slowly being phased out for @get_settings_backend, but
 *              is still required.
 * @start: Called when libpurple is done with its initialization when the user
 *         interface should start telling libpurple about the rest of the user
 *         interface's interfaces.
 * @stop: Called after most of libpurple has been uninitialized.
 * @get_settings_backend: Called to get the [class@Gio.SettingsBackend] that
 *                        the UI is using.
 *
 * The base class for all user interfaces which is used to identify themselves
 * to libpurple when calling [func@Purple.Core.init].
 *
 * Since: 3.0
 */
struct _PurpleUiClass {
	/*< private >*/
	GObjectClass parent;

	/*< public >*/
	void (*prefs_init)(PurpleUi *ui);
	gboolean (*start)(PurpleUi *ui, GError **error);
	void (*stop)(PurpleUi *ui);

	PurpleConversationManager *(*get_conversation_manager)(PurpleUi *ui);
	PurpleHistoryAdapter *(*get_history_adapter)(PurpleUi *ui);
	PurplePresenceManager *(*get_presence_manager)(PurpleUi *ui);
	gpointer (*get_settings_backend)(PurpleUi *ui);

	void (*open_uri)(PurpleUi *ui, const char *uri, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);
	gboolean (*open_uri_finish)(PurpleUi *ui, GAsyncResult *result, GError **error);

	/*< private >*/
	gpointer reserved[4];
};

/**
 * purple_ui_get_conversation_manager:
 * @ui: The instance.
 *
 * Gets the default conversation manager manage from @ui.
 *
 * Note: This should only be called by libpurple which will manage it's life
 * cycle.
 *
 * Returns: (transfer full): The conversation manager for @ui.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleConversationManager *purple_ui_get_conversation_manager(PurpleUi *ui);

/**
 * purple_ui_get_id:
 * @ui: The instance.
 *
 * Gets the id for the user interface.
 *
 * Returns: id of the user interface.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_ui_get_id(PurpleUi *ui);

/**
 * purple_ui_get_name:
 * @ui: The instance.
 *
 * Gets the name of @ui. This should be translated.
 *
 * Returns: The name of the user interface.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_ui_get_name(PurpleUi *ui);

/**
 * purple_ui_get_version:
 * @ui: The instance.
 *
 * Gets the version of @ui.
 *
 * Returns: The version of the user interface.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_ui_get_version(PurpleUi *ui);

/**
 * purple_ui_get_website:
 * @ui: The instance.
 *
 * Gets the website from @ui.
 *
 * Returns: (nullable): The website URI of the user interface or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_ui_get_website(PurpleUi *ui);

/**
 * purple_ui_get_support_website:
 * @ui: The instance.
 *
 * Gets the support website from @ui.
 *
 * Returns: (nullable): The support website URI of the user interface or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_ui_get_support_website(PurpleUi *ui);

/**
 * purple_ui_get_client_type:
 * @ui: The instance.
 *
 * Gets the client type from @ui.
 *
 * Returns: (transfer none): The client type of the user interface.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_ui_get_client_type(PurpleUi *ui);

/**
 * purple_ui_open_uri:
 * @ui: The instance.
 * @uri: The uri to open.
 * @cancellable: (nullable): An optional cancellable.
 * @callback: (nullable) (scope async): A callback to call with the results.
 * @data: User data to pass to @callback.
 *
 * Attempts to open @uri and calls @callback with the result.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_ui_open_uri(PurpleUi *ui, const char *uri, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer data);

/**
 * purple_ui_open_uri_finish:
 * @ui: The instance.
 * @result: The result from the previous [method@Ui.open_uri] call.
 * @error: Return address for a #GError, or %NULL.
 *
 * Finishes a previous call to [method@Ui.open_uri] and gets the result.
 *
 * Returns: %TRUE if the URI was opened successfully, otherwise %FALSE with
 *          @error potentially set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_ui_open_uri_finish(PurpleUi *ui, GAsyncResult *result, GError **error);

/**
 * purple_ui_prefs_init:
 * @ui: The instance.
 *
 * Tells @ui that it should be initializing its preferences.
 *
 * Note: This should only be called by libpurple.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_ui_prefs_init(PurpleUi *ui);

/**
 * purple_ui_start:
 * @ui: The instance.
 * @error: Return address for a #GError, or %NULL.
 *
 * Tells @ui that libpurple is done initializing and that @ui should continue
 * its initialization.
 *
 * The user interface can return errors here which will be propagated by
 * [func@Purple.Core.init] which calls this function.
 *
 * Note: This should only be called by libpurple.
 *
 * Returns: %TRUE if successful, otherwise %FALSE with @error optionally set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_ui_start(PurpleUi *ui, GError **error);

/**
 * purple_ui_stop:
 * @ui: The instance.
 *
 * Tells @ui that libpurple is done shutting down.
 *
 * Note: This should only be called by libpurple.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_ui_stop(PurpleUi *ui);

/**
 * purple_ui_get_settings_backend:
 * @ui: The instance:
 *
 * Get the [class@Gio.SettingsBackend] that @ui is using for its settings.
 *
 * Note: This should only be called by libpurple.
 *
 * Returns: (transfer full): The settings that @ui is using.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gpointer purple_ui_get_settings_backend(PurpleUi *ui);

/**
 * purple_ui_get_history_adapter:
 * @ui: The instance:
 *
 * Gets the history adapter that the user interface wants to use.
 *
 * > Note: This should typically only be called by libpurple. If this returns
 * %NULL, then libpurple will use an in-memory history adapter instead.
 *
 * Returns: (transfer full) (nullable): The [class@HistoryAdapter] to use or
 *          %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleHistoryAdapter *purple_ui_get_history_adapter(PurpleUi *ui);

/**
 * purple_ui_get_presence_manager:
 * @ui: The instance:
 *
 * Gets the [class@PresenceManager] that the user interface wants to use.
 *
 * > Note: This should typically only be called by libpurple. If this returns
 * %NULL, then libpurple will not be able to manage saved presences.
 *
 * Returns: (transfer full) (nullable): The new presence manager or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurplePresenceManager *purple_ui_get_presence_manager(PurpleUi *ui);

G_END_DECLS

#endif /* PURPLE_UI_H */
