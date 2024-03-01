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

#ifndef PURPLE_COMPILATION
# error "purpleidlemanagerprivate.h may only be include by libpurple"
#endif

#ifndef PURPLE_IDLE_MANAGER_PRIVATE_H
#define PURPLE_IDLE_MANAGER_PRIVATE_H

#include <glib.h>

/**
 * purple_idle_manager_startup: (skip)
 *
 * Starts up the idle manager by creating the default instance.
 *
 * Since: 3.0
 */
void purple_idle_manager_startup(void);

/**
 * purple_idle_manager_shutdown: (skip)
 *
 * Shuts down the idle manager by destroying the default instance.
 *
 * Since: 3.0
 */
void purple_idle_manager_shutdown(void);

G_END_DECLS

#endif /* PURPLE_IDLE_MANAGER_PRIVATE_H */
