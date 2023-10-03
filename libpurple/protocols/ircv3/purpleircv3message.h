/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
#if !defined(PURPLE_IRCV3_GLOBAL_HEADER_INSIDE) && \
    !defined(PURPLE_IRCV3_COMPILATION)
# error "only <libpurple/protocols/ircv3.h> may be included directly"
#endif

#ifndef PURPLE_IRCV3_MESSAGE_H
#define PURPLE_IRCV3_MESSAGE_H

#include <glib.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

G_BEGIN_DECLS

#define PURPLE_IRCV3_TYPE_MESSAGE (purple_ircv3_message_get_type())
G_DECLARE_FINAL_TYPE(PurpleIRCv3Message, purple_ircv3_message, PURPLE_IRCV3,
                     MESSAGE, GObject)

/**
 * purple_ircv3_message_new:
 * @command: (not nullable): The command of the message.
 *
 * Creates a new message with @command.
 *
 * Returns: (transfer full): The new message.
 *
 * Since: 3.0.0
 */
PurpleIRCv3Message *purple_ircv3_message_new(const char *command);

/**
 * purple_ircv3_message_get_command:
 * @message: The instance.
 *
 * Gets the command of the message.
 *
 * Returns: The command of the message.
 *
 * Since: 3.0.0
 */
const char *purple_ircv3_message_get_command(PurpleIRCv3Message *message);

/**
 * purple_ircv3_message_set_command:
 * @message: The instance.
 * @command: The new command.
 *
 * Sets the command for @message to @command.
 *
 * Since: 3.0.0
 */
void purple_ircv3_message_set_command(PurpleIRCv3Message *message, const char *command);

/**
 * purple_ircv3_message_get_params:
 * @message: The instance.
 *
 * Gets the parameters from @message.
 *
 * Returns: (transfer none) (nullable): The parameters from @message.
 *
 * Since: 3.0.0
 */
GStrv purple_ircv3_message_get_params(PurpleIRCv3Message *message);

/**
 * purple_ircv3_message_set_params:
 * @message: The instance.
 * @params: (transfer none) (nullable): The new parameters.
 *
 * Sets the parameters of @message to @params.
 *
 * Since: 3.0.0
 */
void purple_ircv3_message_set_params(PurpleIRCv3Message *message, GStrv params);

/**
 * purple_ircv3_message_get_source:
 * @message: The instance.
 *
 * Gets the the source of @message.
 *
 * Returns: (nullable): The source of @message.
 *
 * Since: 3.0.0
 */
const char *purple_ircv3_message_get_source(PurpleIRCv3Message *message);

/**
 * purple_ircv3_message_set_source:
 * @message: The instance.
 * @source: (nullable): The new source.
 *
 * Sets the source of @message to @source.
 *
 * Since: 3.0.0
 */
void purple_ircv3_message_set_source(PurpleIRCv3Message *message, const char *source);

/**
 * purple_ircv3_message_get_tags:
 * @message: The instance.
 *
 * Gets the tags from @message.
 *
 * Returns: (transfer none) (element-type utf8 utf8) (nullable): The tags from
 *          @message.
 *
 * Since: 3.0.0
 */
GHashTable *purple_ircv3_message_get_tags(PurpleIRCv3Message *message);

/**
 * purple_ircv3_message_set_tags:
 * @message: The instance.
 * @tags: (transfer none) (element-type utf8 utf8) (nullable): The new tags.
 *
 * Sets the tags of @message to @tags.
 *
 * Since: 3.0.0
 */
void purple_ircv3_message_set_tags(PurpleIRCv3Message *message, GHashTable *tags);

G_END_DECLS

#endif /* PURPLE_IRCV3_MESSAGE_H */
