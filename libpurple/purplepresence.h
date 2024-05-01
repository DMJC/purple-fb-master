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

#ifndef PURPLE_PRESENCE_H
#define PURPLE_PRESENCE_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

/**
 * PurplePresence:
 *
 * A PurplePresence is like a collection of PurpleStatuses (plus some other
 * random info).  For any buddy, or for any one of your accounts, or for any
 * person with which you're chatting, you may know various amounts of
 * information.  This information is all contained in one PurplePresence.  If
 * one of your buddies is away and idle, then the presence contains the
 * #PurpleStatus for their awayness, and it contains their current idle time.
 * #PurplePresence's are never saved to disk.  The information they contain is
 * only relevant for the current Purple session.
 *
 * Note: When a presence is destroyed with the last g_object_unref(), all
 * statuses added to this list will be destroyed along with the presence.
 *
 * Since: 2.0
 */

typedef struct _PurplePresence PurplePresence;

/**
 * PurplePresencePrimitive:
 * @PURPLE_PRESENCE_PRIMITIVE_OFFLINE: The presence is offline or otherwise
 *                                     unknown.
 * @PURPLE_PRESENCE_PRIMITIVE_AVAILABLE: The presence is online or available.
 * @PURPLE_PRESENCE_PRIMITIVE_IDLE: The presence is online but is idle. This
 *                                  state is typically set by the client
 *                                  directly and not the user.
 * @PURPLE_PRESENCE_PRIMITIVE_INVISIBLE: The presence is online, but not
 *                                       visible to others.
 * @PURPLE_PRESENCE_PRIMITIVE_AWAY: The presence is online, but the user is
 *                                  away from their device.
 * @PURPLE_PRESENCE_PRIMITIVE_DO_NOT_DISTURB: Similar to
 *                                            @PURPLE_PRESENCE_PRIMITIVE_AWAY,
 *                                            but typically means the user has
 *                                            notifications and sounds
 *                                            disabled.
 * @PURPLE_PRESENCE_PRIMITIVE_STREAMING: The presence is online but is
 *                                       streaming.
 *
 * An enum that is used to determine the type of a [class@Purple.Presence].
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_TYPE_IN_3_0
typedef enum {
	PURPLE_PRESENCE_PRIMITIVE_OFFLINE,
	PURPLE_PRESENCE_PRIMITIVE_AVAILABLE,
	PURPLE_PRESENCE_PRIMITIVE_IDLE,
	PURPLE_PRESENCE_PRIMITIVE_INVISIBLE,
	PURPLE_PRESENCE_PRIMITIVE_AWAY,
	PURPLE_PRESENCE_PRIMITIVE_DO_NOT_DISTURB,
	PURPLE_PRESENCE_PRIMITIVE_STREAMING,
} PurplePresencePrimitive;

G_BEGIN_DECLS

#define PURPLE_TYPE_PRESENCE purple_presence_get_type()

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurplePresence, purple_presence, PURPLE, PRESENCE,
                     GObject)

/**
 * purple_presence_new:
 *
 * Creates a new presence instance.
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
PurplePresence *purple_presence_new(void);

/**
 * purple_presence_set_idle:
 * @presence: The #PurplePresence instance.
 * @idle: The idle state.
 *
 * Sets the idle state and time of @presence.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_presence_set_idle(PurplePresence *presence, gboolean idle);

/**
 * purple_presence_set_idle_time:
 * @presence: The instance.
 * @idle_time: (transfer none) (nullable): The time when the presence went
 *             idle, or %NULL to clear the idle state.
 *
 * Sets the time that @presence went idle to @idle_time.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_presence_set_idle_time(PurplePresence *presence, GDateTime *idle_time);

/**
 * purple_presence_set_login_time:
 * @presence: The #PurplePresence instance.
 * @login_time: (transfer none): The login time.
 *
 * Sets the login time on a presence.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_presence_set_login_time(PurplePresence *presence, GDateTime *login_time);

/**
 * purple_presence_is_available:
 * @presence: The #PurplePresence instance.
 *
 * Gets whether or not @presence is available.
 *
 * Available presences are online and possibly invisible, but not away or idle.
 *
 * Returns: %TRUE if the presence is available, or %FALSE otherwise.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_presence_is_available(PurplePresence *presence);

/**
 * purple_presence_is_online:
 * @presence: The #PurplePresence instance.
 *
 * Gets whether or not @presence is online.
 *
 * Returns: %TRUE if the presence is online, or %FALSE otherwise.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_presence_is_online(PurplePresence *presence);

/**
 * purple_presence_is_idle:
 * @presence: The #PurplePresence instance.
 *
 * Gets whether or not @presence is idle.
 *
 * Returns: %TRUE if the presence is idle, or %FALSE otherwise.  If the
 *          presence is offline (purple_presence_is_online() returns %FALSE)
 *          then %FALSE is returned.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gboolean purple_presence_is_idle(PurplePresence *presence);

/**
 * purple_presence_get_idle_time:
 * @presence: The #PurplePresence instance.
 *
 * Gets the idle time of @presence. This can be %NULL if the protocol doesn't
 * support idle times or if the presence isn't in an idle state.
 *
 * Returns: (nullable): The idle time of @presence or %NULL.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
GDateTime *purple_presence_get_idle_time(PurplePresence *presence);

/**
 * purple_presence_get_login_time:
 * @presence: The #PurplePresence instance.
 *
 * Gets the login time of @presence.
 *
 * Returns: (transfer none): The login time of @presence.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
GDateTime *purple_presence_get_login_time(PurplePresence *presence);

/**
 * purple_presence_compare:
 * @presence1: The first presence.
 * @presence2: The second presence.
 *
 * Compares the presences for availability.
 *
 * Returns: -1 if @presence1 is more available than @presence2.
 *           0 if @presence1 is equal to @presence2.
 *           1 if @presence1 is less available than @presence2.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
gint purple_presence_compare(PurplePresence *presence1, PurplePresence *presence2);

/**
 * purple_presence_get_primitive:
 * @presence: The instance.
 *
 * Gets the [enum@Purple.PresencePrimitive] for @presence.
 *
 * Returns: The current primitive.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurplePresencePrimitive purple_presence_get_primitive(PurplePresence *presence);

/**
 * purple_presence_set_primitive:
 * @presence: The instance.
 * @primitive: The new primitive.
 *
 * Sets the [enum@Purple.PresencePrimitive] for @presence to @primitive.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_presence_set_primitive(PurplePresence *presence, PurplePresencePrimitive primitive);

/**
 * purple_presence_get_message:
 * @presence: The instance.
 *
 * Gets the status message for @presence if one is set.
 *
 * Returns: (nullable): The status message of @presence.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_presence_get_message(PurplePresence *presence);

/**
 * purple_presence_set_message:
 * @presence: The instance.
 * @message: (nullable): The new message.
 *
 * Sets the status message of @presence to @message.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_presence_set_message(PurplePresence *presence, const char *message);

/**
 * purple_presence_get_emoji:
 * @presence: The instance.
 *
 * Gets the current emoji, sometimes referred to as a mood, of @presence.
 *
 * Returns: The current emoji or %NULL if none is set.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_presence_get_emoji(PurplePresence *presence);

/**
 * purple_presence_set_emoji:
 * @presence: The instance.
 * @emoji: (nullable): The new emoji to set.
 *
 * Sets the current emoji, sometimes referred to as a mood, of @presence to
 * @emoji.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_presence_set_emoji(PurplePresence *presence, const char *emoji);

/**
 * purple_presence_get_mobile:
 * @presence: The instance.
 *
 * Gets whether or not @presence is on a mobile device.
 *
 * Returns: %TRUE if @presence is on a mobile device, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_presence_get_mobile(PurplePresence *presence);

/**
 * purple_presence_set_mobile:
 * @presence: The instance.
 * @mobile: The new mobile status.
 *
 * Sets whether or not @presence is on a mobile device.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_presence_set_mobile(PurplePresence *presence, gboolean mobile);

/**
 * purple_presence_get_notifications_disabled:
 * @presence: The instance.
 *
 * Gets whether or not @presence has notifications disabled.
 *
 * Returns: %TRUE if @presence has notifications disabled, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_presence_get_notifications_disabled(PurplePresence *presence);

/**
 * purple_presence_set_notifications_disabled:
 * @presence: The instance.
 * @notifications_disabled: Whether notifications are disabled or not.
 *
 * Sets whether or not @presence has notifications disabled.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_presence_set_notifications_disabled(PurplePresence *presence, gboolean notifications_disabled);

/**
 * purple_presence_primitive_to_string:
 * @primitive: The [enum@Purple.PresencePrimitive] value.
 *
 * Gets a string representation for @primitive that is suitable for display to
 * users.
 *
 * Returns: The string representation.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_presence_primitive_to_string(PurplePresencePrimitive primitive);

G_END_DECLS

#endif /* PURPLE_PRESENCE_H */
