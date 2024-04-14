/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
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

#if !defined(PIDGIN_GLOBAL_HEADER_INSIDE) && !defined(PIDGIN_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PIDGIN_AVATAR_H
#define PIDGIN_AVATAR_H

#include <glib.h>

#include <gtk/gtk.h>

#include <purple.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginAvatar:
 *
 * #PidginAvatar is a widget that displays avatars for contacts or
 * conversations.
 *
 * Since: 3.0
 */

#define PIDGIN_TYPE_AVATAR (pidgin_avatar_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginAvatar, pidgin_avatar, PIDGIN, AVATAR, GtkBox)

/**
 * pidgin_avatar_new:
 *
 * Creates a new #PidginAvatar instance.
 *
 * Returns: (transfer full): The new #PidginAvatar instance.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkWidget *pidgin_avatar_new(void);

/**
 * pidgin_avatar_set_animate:
 * @avatar: The #PidginAvatar instance.
 * @animate: Whether or not to animate the avatar.
 *
 * Starts or stops animation of @avatar.  If avatar is displaying a
 * non-animated image, changing this will do nothing unless a new animated
 * image is set.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_avatar_set_animate(PidginAvatar *avatar, gboolean animate);

/**
 * pidgin_avatar_get_animate:
 * @avatar: The #PidginAvatar instance.
 *
 * Gets whether or not @avatar should be animated.
 *
 * Returns: Whether or not @avatar should be animated.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
gboolean pidgin_avatar_get_animate(PidginAvatar *avatar);

/**
 * pidgin_avatar_set_contact_info:
 * @avatar: The #PidginAvatar instance.
 * @info: (nullable): A contact info or %NULL to unset.
 *
 * Sets or unsets the [class@Purple.ContactInfo] that @avatar should use for
 * display.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_avatar_set_contact_info(PidginAvatar *avatar, PurpleContactInfo *info);

/**
 * pidgin_avatar_get_contact_info:
 * @avatar: The #PidginAvatar instance.
 *
 * Gets the [class@Purple.ContactInfo] that @avatar is using for display.
 *
 * Returns: (transfer none): The contact info if set, otherwise %NULL.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
PurpleContactInfo *pidgin_avatar_get_contact_info(PidginAvatar *avatar);

/**
 * pidgin_avatar_set_conversation:
 * @avatar: The #PidginAvatar instance.
 * @conversation: (nullable): The conversation to set or %NULL to unset.
 *
 * Sets or unsets the conversation that @avatar uses to find the
 * [class@Purple.Avatar] to display.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_avatar_set_conversation(PidginAvatar *avatar, PurpleConversation *conversation);

/**
 * pidgin_avatar_get_conversation:
 * @avatar: The #PidginAvatar instance.
 *
 * Gets the #PurpleConversation that @avatar is using for display.
 *
 * Returns: (transfer none): The conversation that @avatar is using to
 *          find the [class@Purple.Avatar] to display.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
PurpleConversation *pidgin_avatar_get_conversation(PidginAvatar *avatar);

G_END_DECLS

#endif /* PIDGIN_AVATAR_H */
