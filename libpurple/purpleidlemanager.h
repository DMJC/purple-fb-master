/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_IDLE_MANAGER_H
#define PURPLE_IDLE_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

#define PURPLE_TYPE_IDLE_MANAGER (purple_idle_manager_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleIdleManager, purple_idle_manager, PURPLE,
                     IDLE_MANAGER, GObject)

/**
 * PurpleIdleManager:
 *
 * The idle manager keeps track of multiple idle sources and aggregates them
 * to the oldest one to report a global idle state.
 *
 * Idle sources include application usage, device usage, or a manually set
 * value, among other possibilities. User interfaces should allow users to
 * determine what if any idle sources are tracked in the idle manager.
 *
 * The idle source with the oldest timestamp is used as the value for
 * [property@IdleManager:timestamp] as it is most likely what the user is
 * looking for based on the settings they would choose in the user interface.
 *
 * Most users will only ever have a single idle source, but could add an
 * additional manual source to set a specific time that they went idle.
 *
 * If the user has chosen no idle reporting, which means no sources are ever
 * added to [class@IdleManager], then [property@IdleManager:timestamp] will
 * always be %NULL.
 *
 * Most users will choose between application and device usage. The difference
 * being that application usage is updated whenever you send a message whereas
 * device usage is only updated when you haven't interacted with your device.
 *
 * However, there is also the ability to manually set an idle time via plugins.
 * Typically users will manually set their idle time to something exaggerated
 * like months or years.
 *
 * A manual idle source could also be created to replicate an existing idle
 * source like the application usage, so that the user can start using the
 * application without resetting the idle time. This would in effect allow the
 * user to use the application in "stealth mode" by remaining idle.
 *
 * In both of these examples, the user wishes to remain idle while still using
 * the application. This is precisely why the oldest idle time is used as the
 * aggregate.
 *
 * Since: 3.0.0
 */

G_BEGIN_DECLS

/**
 * purple_idle_manager_get_default:
 *
 * Gets the default idle manager that libpurple is using.
 *
 * Returns: (transfer none): The default idle manager.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleIdleManager *purple_idle_manager_get_default(void);

/**
 * purple_idle_manager_set_source:
 * @manager: The instance.
 * @source: The name of the source.
 * @timestamp: (nullable): The new timestamp for @source.
 *
 * Sets the timestamp of when @source went idle to @timestamp. If @timestamp is
 * %NULL, @source will be removed from @manager.
 *
 * Returns: %TRUE if [property@IdleManager:timestamp] has changed due to this
 *          call.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_idle_manager_set_source(PurpleIdleManager *manager, const char *source, GDateTime *timestamp);

/**
 * purple_idle_manager_get_timestamp:
 * @manager: The instance.
 *
 * Gets the oldest timestamp of all the sources that @manager knows about.
 *
 * Returns: (transfer none) (nullable): The oldest timestamp or %NULL if no
 *          sources are idle.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GDateTime *purple_idle_manager_get_timestamp(PurpleIdleManager *manager);

G_END_DECLS

#endif /* PURPLE_IDLE_MANAGER_H */
