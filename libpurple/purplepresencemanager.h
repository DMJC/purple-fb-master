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

#ifndef PURPLE_PRESENCE_MANAGER_H
#define PURPLE_PRESENCE_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include <gio/gio.h>

#include "purplesavedpresence.h"
#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_PRESENCE_MANAGER purple_presence_manager_get_type()

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurplePresenceManager, purple_presence_manager, PURPLE,
                     PRESENCE_MANAGER, GObject)

/**
 * PurplePresenceManager:
 *
 * A manager for [class@SavedPresence]'s.
 *
 * Since: 3.0
 */

/**
 * purple_presence_manager_get_default:
 *
 * Gets the default [class@PresenceManager] that should be used.
 *
 * This may be %NULL if libpurple hasn't been initialized or if someone
 * incorrectly unref'd it.
 *
 * See also [func@PresenceManager.get_default_as_model].
 *
 * Returns: (transfer none) (nullable): The default presence manager.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurplePresenceManager *purple_presence_manager_get_default(void);

/**
 * purple_presence_manager_get_default_as_model:
 *
 * Gets the default [class@PresenceManager] as a [iface@Gio.ListModel].
 *
 * This is the equivalent to calling [func@PresenceManager.get_default] and
 * type casting it to [iface@Gio.ListModel].
 *
 * This may be %NULL if libpurple hasn't been initialized or if someone
 * incorrectly unref'd it.
 *
 * See also [func@PresenceManager.get_default].
 *
 * Returns: (transfer none) (nullable): The default presence manager type cast
 *          to a list model.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
GListModel *purple_presence_manager_get_default_as_model(void);

/**
 * purple_presence_manager_new:
 * @path: (nullable): An optional directory path where settings will be stored.
 *
 * Creates a new presence manager instance.
 *
 * If @path is %NULL, settings will not be stored to disk.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurplePresenceManager *purple_presence_manager_new(const char *path);

/**
 * purple_presence_manager_get_path:
 * @manager: The instance.
 *
 * Gets the directory path that @manager is using for storage.
 *
 * Returns: (nullable): The directory path where settings are being saved to
 *          disk or %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_presence_manager_get_path(PurplePresenceManager *manager);

/**
 * purple_presence_manager_get_active:
 * @manager: The instance.
 *
 * Gets the active [class@SavedPresence] from @manager.
 *
 * Returns: (transfer none) (nullable): The active presence if there is one,
 *          otherwise %NULL.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleSavedPresence *purple_presence_manager_get_active(PurplePresenceManager *manager);

/**
 * purple_presence_manager_set_active:
 * @manager: The instance.
 * @presence: The presence to set as active.
 *
 * Sets the active presence to @presence.
 *
 * If @manager doesn't know about @presence, %FALSE will be returned to
 * indicate a failure.
 *
 * Returns: %TRUE if a presence was found and made active, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_presence_manager_set_active(PurplePresenceManager *manager, PurpleSavedPresence *presence);

/**
 * purple_presence_manager_create:
 * @manager: The instance.
 *
 * Creates a new [class@SavedPresence] that is automatically added to @manager.
 *
 * This is the only way to add a [class@SavedPresence] to @manager. This means
 * that user interfaces will need to call this before displaying an editor to
 * create a [class@SavedPresence].
 *
 * Returns: (transfer full): The new [class@SavedPresence].
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleSavedPresence *purple_presence_manager_create(PurplePresenceManager *manager);

/**
 * purple_presence_manager_remove:
 * @manager: The instance.
 * @id: The id of the [class@SavedPresence].
 *
 * Attempts to remove the [class@SavedPresence] identified by @id from
 * @manager.
 *
 * Returns: %TRUE if a [class@SavedPresence] was found with @id and removed,
 *          otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_presence_manager_remove(PurplePresenceManager *manager, const char *id);

G_END_DECLS

#endif /* PURPLE_PRESENCE_MANAGER_H */
