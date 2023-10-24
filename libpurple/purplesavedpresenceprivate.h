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
# error "purplesavedpresenceprivate.h may only be include by libpurple"
#endif

#ifndef PURPLE_SAVED_PRESENCE_PRIVATE_H
#define PURPLE_SAVED_PRESENCE_PRIVATE_H

#include <glib.h>

#include "purplesavedpresence.h"

G_BEGIN_DECLS

/**
 * purple_saved_presence_reset: (skip)
 * @presence: The instance.
 *
 * Resets all of the settings of @presence to their default values.
 *
 * As of at least glib 2.78.0, the GSettings API doesn't have a way to remove a
 * relocatable settings object. We use relocatable settings to store each saved
 * presence which means we can't delete the section from the GKeyFile.
 *
 * To minimize the amount of disk space wasted by this issue, this function
 * will default all of the keys in the settings object for @presence so that
 * the values will be removed from the GKeyFile.
 *
 * Since: 3.0.0
 */
G_GNUC_INTERNAL void purple_saved_presence_reset(PurpleSavedPresence *presence);

G_END_DECLS

#endif /* PURPLE_SAVED_PRESENCE_PRIVATE_H */
