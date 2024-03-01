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
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PURPLE_CHANNEL_JOIN_DETAILS_H
#define PURPLE_CHANNEL_JOIN_DETAILS_H

#include <glib.h>
#include <glib-object.h>

#include "purpleversion.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_CHANNEL_JOIN_DETAILS (purple_channel_join_details_get_type())

PURPLE_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PurpleChannelJoinDetails, purple_channel_join_details, PURPLE, CHANNEL_JOIN_DETAILS, GObject)

/**
 * PurpleChannelJoinDetails:
 *
 * The details that are necessary for a [class@Protocol] to join a channel. A
 * user interface will ask a protocol for a [class@ChannelJoinDetails] and then
 * fill it out and pass it back to the protocol to actually join a channel.
 *
 * Since: 3.0
 */

/**
 * purple_channel_join_details_new:
 * @nickname_supported: Whether or not nicknames are supported.
 * @password_supported: Whether or not passwords are supported.
 *
 * Creates a new [class@ChannelJoinDetails].
 *
 * Returns: (transfer full): The new instance.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleChannelJoinDetails *purple_channel_join_details_new(gboolean nickname_supported, gboolean password_supported);

/**
 * purple_channel_join_details_get_name:
 * @details: The instance.
 *
 * Gets the name of the channel.
 *
 * Returns: (nullable): The name of the channel.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_channel_join_details_get_name(PurpleChannelJoinDetails *details);

/**
 * purple_channel_join_details_set_name:
 * @details: The instance.
 * @name: (nullable): The new name.
 *
 * Sets the name of the channel to @name.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_channel_join_details_set_name(PurpleChannelJoinDetails *details, const char *name);

/**
 * purple_channel_join_details_get_nickname:
 * @details: The instance.
 *
 * Gets the nickname of the user in the channel.
 *
 * Returns: (nullable): The user's channel-specific nickname to use.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_channel_join_details_get_nickname(PurpleChannelJoinDetails *details);

/**
 * purple_channel_join_details_set_nickname:
 * @details: The instance.
 * @nickname: (nullable): The new nickname.
 *
 * Sets the channel-specific nickname for the user.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_channel_join_details_set_nickname(PurpleChannelJoinDetails *details, const char *nickname);

/**
 * purple_channel_join_details_get_nickname_supported:
 * @details: The instance.
 *
 * Gets whether or not channel-specific nicknames are supported.
 *
 * Returns: %TRUE if channel-specific nicknames are supported otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_channel_join_details_get_nickname_supported(PurpleChannelJoinDetails *details);

/**
 * purple_channel_join_details_get_password:
 * @details: The instance.
 *
 * Gets the password the user specified to join this channel.
 *
 * Returns: (nullable): The password to join this channel.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
const char *purple_channel_join_details_get_password(PurpleChannelJoinDetails *details);

/**
 * purple_channel_join_details_set_password:
 * @details: The instance.
 * @password: (nullable): The new password.
 *
 * Sets the password to use to join this channel.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_channel_join_details_set_password(PurpleChannelJoinDetails *details, const char *password);

/**
 * purple_channel_join_details_get_password_supported:
 * @details: The instance.
 *
 * Gets whether or not the protocols supports channel passwords.
 *
 * Returns: %TRUE if channel passwords are supported, otherwise %FALSE.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
gboolean purple_channel_join_details_get_password_supported(PurpleChannelJoinDetails *details);

/**
 * purple_channel_join_details_merge:
 * @source: The instance to copy from.
 * @destination: The instance to copy to.
 *
 * Copies the values of [property@PurpleChannelJoinDetails:name],
 * [property@PurpleChannelJoinDetails:nickname], and
 * [property@PurpleChannelJoinDetails:password] from @source to @destination.
 *
 * This function is intended to be used in the join channel dialog when the
 * user switches accounts and the user interface needs to ask the protocol for
 * its default [class@ChannelJoinDetails] so that any input from the user can
 * be maintained.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_channel_join_details_merge(PurpleChannelJoinDetails *source, PurpleChannelJoinDetails *destination);

G_END_DECLS

#endif /* PURPLE_CHANNEL_JOIN_DETAILS_H */
