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

#ifndef PURPLE_BADGES_H
#define PURPLE_BADGES_H

#include <glib.h>
#include <glib-object.h>

#include "purplebadge.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleBadges:
 *
 * #PurpleBadges is a collection of [class@Badge]s sorted by their priority.
 * This is intended to be used on [class@ConversationMembers] and perhaps
 * elsewhere to display information like "moderator", "admin", "broadcaster",
 * etc. Items with the highest priority will be at the start of the collection.
 *
 * Since: 3.0
 */

#define PURPLE_TYPE_BADGES (purple_badges_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleBadges, purple_badges, PURPLE, BADGES, GObject)

/**
 * purple_badges_new:
 *
 * Creates a new badges collection.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleBadges *purple_badges_new(void);

/**
 * purple_badges_add_badge:
 * @badges: The instance.
 * @badge: The new badge to add.
 *
 * Adds @badge to @badges.
 *
 * If a badge with the same id of @badge already exists, @badge will not be
 * added and %FALSE will be returned.
 *
 * Returns: %TRUE if @badge was added successfully, otherwise %FALSE if an
 *          existing badge has the same id.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_badges_add_badge(PurpleBadges *badges, PurpleBadge *badge);

/**
 * purple_badges_remove_badge:
 * @badges: The instance.
 * @id: The id of the badge to remove.
 *
 * Removes the badge with an id of @id.
 *
 * Returns: %TRUE if a badge was found with an id of @id, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_badges_remove_badge(PurpleBadges *badges, const char *id);

G_END_DECLS

#endif /* PURPLE_BADGES_H */
