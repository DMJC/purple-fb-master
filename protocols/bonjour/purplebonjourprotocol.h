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
#ifndef PURPLE_BONJOUR_PROTOCOL_H
#define PURPLE_BONJOUR_PROTOCOL_H

#include <glib.h>
#include <glib-object.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

G_BEGIN_DECLS

#define PURPLE_BONJOUR_TYPE_PROTOCOL (purple_bonjour_protocol_get_type())

G_DECLARE_FINAL_TYPE(PurpleBonjourProtocol, purple_bonjour_protocol,
                     PURPLE_BONJOUR, PROTOCOL, PurpleProtocol)

/**
 * purple_bonjour_protocol_register: (skip)
 * @plugin: The GTypeModule
 *
 * Registers the dynamic type using @plugin.
 *
 * Since: 3.0
 */
G_GNUC_INTERNAL void purple_bonjour_protocol_register(GPluginNativePlugin *plugin);

G_GNUC_INTERNAL PurpleProtocol *purple_bonjour_protocol_new(void);

G_END_DECLS

#endif /* PURPLE_BONJOUR_PROTOCOL_H */
