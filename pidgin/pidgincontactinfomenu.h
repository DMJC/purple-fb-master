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

#ifndef PIDGIN_CONTACT_INFO_MENU_H
#define PIDGIN_CONTACT_INFO_MENU_H

#include <glib.h>

#include <gtk/gtk.h>

#include <purple.h>

G_BEGIN_DECLS

/**
 * pidgin_contact_info_menu_popup:
 * @info: The [class@Purple.ContactInfo] that the menu should use.
 * @account: The [class@Purple.Account] that @info is for.
 * @parent: The parent [class@Gtk.Widget] that the menu should use.
 * @x: The x coordinate to point to.
 * @y: The y coordinate to point to.
 *
 * Creates a [class@Gtk.PopoverMenu] for @info and displays it at the @x and @y
 * coordinates instead of @parent.
 *
 * Since: 3.0.0
 */
void pidgin_contact_info_menu_popup(PurpleContactInfo *info, PurpleAccount *account, GtkWidget *parent, gdouble x, gdouble y);

G_END_DECLS

#endif /* PIDGIN_CONTACT_INFO_MENU_H */
