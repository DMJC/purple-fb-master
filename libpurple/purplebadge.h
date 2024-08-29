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

#ifndef PURPLE_BADGE_H
#define PURPLE_BADGE_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

G_BEGIN_DECLS

/**
 * PurpleBadge:
 *
 * #PurpleBadge is a data structure for badges. They are intended to be used
 * on [class@ConversationMembers] to display badges like "admin", "moderator",
 * "broadcaster", etc.
 *
 * Since: 3.0
 */

#define PURPLE_TYPE_BADGE (purple_badge_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleBadge, purple_badge, PURPLE, BADGE, GObject)

/**
 * purple_badge_new:
 * @id: The id.
 * @priority: The priority.
 * @icon_name: The icon name.
 * @mnemonic: A character as a mnemonic.
 *
 * Creates a new badge.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleBadge *purple_badge_new(const char *id, int priority, const char *icon_name, const char *mnemonic);

/**
 * purple_badge_get_id:
 * @badge: The instance.
 *
 * Gets the id of @badge.
 *
 * Returns: The id of @badge.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_id(PurpleBadge *badge);

/**
 * purple_badge_get_priority:
 * @badge: The instance.
 *
 * Gets the priority of @badge.
 *
 * Returns: The priority of @badge.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
int purple_badge_get_priority(PurpleBadge *badge);

/**
 * purple_badge_get_icon_name:
 * @badge: The instance.
 *
 * Gets the icon-name from @badge.
 *
 * Returns: The icon-name.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_icon_name(PurpleBadge *badge);

/**
 * purple_badge_get_mnemonic:
 * @badge: The instance.
 *
 * Gets the mnemonic from @badge.
 *
 * Returns: The mnemonic.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_mnemonic(PurpleBadge *badge);

/**
 * purple_badge_get_tooltip_text:
 * @badge: The instance.
 *
 * Gets the tooltip text from @badge.
 *
 * Returns: (nullable): The tooltip text.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_tooltip_text(PurpleBadge *badge);

/**
 * purple_badge_set_tooltip_text:
 * @badge: The instance.
 * @tooltip_text: (nullable): The new tooltip text.
 *
 * Sets the tooltip text of @badge.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_badge_set_tooltip_text(PurpleBadge *badge, const char *tooltip_text);

/**
 * purple_badge_get_description:
 * @badge: The instance.
 *
 * Gets the description of @badge.
 *
 * This can be used by user interfaces to tell the user what this badge
 * represents.
 *
 * Returns: (nullable): The description of @badge.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_description(PurpleBadge *badge);

/**
 * purple_badge_set_description:
 * @badge: The instance.
 * @description: (nullable): The new description.
 *
 * Sets the description of @badge to @description.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_badge_set_description(PurpleBadge *badge, const char *description);

/**
 * purple_badge_get_link_text:
 * @badge: The instance.
 *
 * Gets the text that should be displayed when displaying the link for @badge.
 *
 * Returns: (nullable): The link text.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_link_text(PurpleBadge *badge);

/**
 * purple_badge_set_link_text:
 * @badge: The instance.
 * @link_text: (nullable): The new link text.
 *
 * Sets the link text of @badge to @link_text.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_badge_set_link_text(PurpleBadge *badge, const char *link_text);

/**
 * purple_badge_get_link_uri:
 * @badge: The instance.
 *
 * Gets the link URI of @badge.
 *
 * Returns: (nullable): The URI.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_badge_get_link_uri(PurpleBadge *badge);

/**
 * purple_badge_set_link_uri:
 * @badge: The instance.
 * @link_uri: (nullable): The new link URI.
 *
 * Sets the link URI of @badge to @link_uri.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_badge_set_link_uri(PurpleBadge *badge, const char *link_uri);

/**
 * purple_badge_compare:
 * @badge1: (nullable): The first badge.
 * @badge2: (nullable): The second badge.
 *
 * Gets which badge has higher priority.
 *
 * If @badge1 has a higher priority then an integer less than 0 will be
 * returned. If their priorities are equal, 0 will be returned. If @badge2 has
 * a higher priority, an integer greater than 0 will be returned.
 *
 * Returns: An integer less than 0 if @badge1 has higher priorty, 0 if they
 *          have equal priorities, and an integer greater than 0 if @badge2 has
 *          a higher priority.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
int purple_badge_compare(PurpleBadge *badge1, PurpleBadge *badge2);

/**
 * purple_badge_equal:
 * @badge1: (nullable): The first badge.
 * @badge2: (nullable): The second badge.
 *
 * Checks if @badge1 and @badge2 have the same id.
 *
 * Returns: %TRUE if the badges have the same id.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_badge_equal(PurpleBadge *badge1, PurpleBadge *badge2);

G_END_DECLS

#endif /* PURPLE_BADGE_H */
