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

#ifndef PURPLE_IRCV3_CAPABILITIES_H
#define PURPLE_IRCV3_CAPABILITIES_H

#include <glib.h>
#include <glib-object.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

G_BEGIN_DECLS

#define PURPLE_IRCV3_TYPE_CAPABILITIES (purple_ircv3_capabilities_get_type())
G_DECLARE_FINAL_TYPE(PurpleIRCv3Capabilities, purple_ircv3_capabilities,
                     PURPLE_IRCV3, CAPABILITIES, GObject)

#include "purpleircv3connection.h"

G_GNUC_INTERNAL void purple_ircv3_capabilities_register(GPluginNativePlugin *plugin);

G_GNUC_INTERNAL PurpleIRCv3Capabilities *purple_ircv3_capabilities_new(PurpleIRCv3Connection *connection);

G_GNUC_INTERNAL gboolean purple_ircv3_capabilities_message_handler(GHashTable *tags, const char *source, const char *command, guint n_params, GStrv params, GError **error, gpointer data);

/**
 * purple_ircv3_capabilities_get_connection:
 * @capabilities: The instance.
 *
 * Gets the PurpleIRCv3Connection object that @capabilities was created with.
 *
 * Returns: (transfer none): The connection instance.
 *
 * Since: 3.0.0
 */
PurpleIRCv3Connection *purple_ircv3_capabilities_get_connection(PurpleIRCv3Capabilities *capabilities);

/**
 * purple_ircv3_capabilities_request:
 * @capabilities: The instance.
 * @capability: The capabilities to request.
 *
 * This method will send `CAP REQ @capability` to the server. Listen to the
 * `::ack` and `::nak` signals which will contain the contents of @capability
 * that was passed in here.
 *
 * Since: 3.0.0
 */
void purple_ircv3_capabilities_request(PurpleIRCv3Capabilities *capabilities, const char *capability);

/**
 * purple_ircv3_capabilities_lookup:
 * @capabilities: The instance.
 * @name: The name of the capability to look for.
 * @found: (out) (nullable): A return address for a boolean on whether the
 *         capability was advertised or not.
 *
 * Gets the value that the @name capability provided if it was advertised. To
 * determine if the capability was advertised use the @found parameter.
 *
 * Returns: The value of the capability named @name.
 *
 * Since: 3.0.0
 */
const char *purple_ircv3_capabilities_lookup(PurpleIRCv3Capabilities *capabilities, const char *name, gboolean *found);

G_END_DECLS

#endif /* PURPLE_IRCV3_CAPABILITIES_H */
