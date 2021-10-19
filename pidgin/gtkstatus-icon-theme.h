/* pidgin
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#if !defined(PIDGIN_GLOBAL_HEADER_INSIDE) && !defined(PIDGIN_COMPILATION)
# error "only <pidgin.h> may be included directly"
#endif

#ifndef PIDGIN_STATUS_ICON_THEME_H
#define PIDGIN_STATUS_ICON_THEME_H

#include <glib-object.h>
#include "gtkicon-theme.h"

#define PIDGIN_TYPE_STATUS_ICON_THEME  pidgin_status_icon_theme_get_type()

/**************************************************************************/
/* Pidgin Status Icon Theme API                                           */
/**************************************************************************/
G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(PidginStatusIconTheme, pidgin_status_icon_theme, PIDGIN,
		STATUS_ICON_THEME, PidginIconTheme)

G_END_DECLS

#endif /* PIDGIN_STATUS_ICON_THEME_H */
