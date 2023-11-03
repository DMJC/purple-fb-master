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

#ifndef PURPLE_COMPILATION
# error "purplepresencemanagerprivate.h may only be include by libpurple"
#endif

#ifndef PURPLE_PRESENCE_MANAGER_PRIVATE_H
#define PURPLE_PRESENCE_MANAGER_PRIVATE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * purple_presence_manager_startup: (skip)
 *
 * This will ask the UI what [class@PresenceManager] should be used and caches
 * it for use with [func@PresenceManager.get_default].
 *
 * Since: 3.0.0
 */
G_GNUC_INTERNAL void purple_presence_manager_startup(void);

/**
 * purple_presence_manager_shutdown: (skip)
 *
 * Clears the references to the default manager if one exists.
 *
 * Since: 3.0.0
 */
G_GNUC_INTERNAL void purple_presence_manager_shutdown(void);

G_END_DECLS

#endif /* PURPLE_PRESENCE_MANAGER_PRIVATE_H */
