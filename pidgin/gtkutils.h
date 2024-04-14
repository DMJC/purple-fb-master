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

#ifndef _PIDGINUTILS_H_
#define _PIDGINUTILS_H_

#include <gtk/gtk.h>

#include <purple.h>

#include "pidginversion.h"


G_BEGIN_DECLS

/**
 * pidgin_set_accessible_label:
 * @w: The widget that we want to name.
 * @l: A GtkLabel that we want to use as the ATK name for the widget.
 *
 * Sets an ATK name for a given widget.  Also sets the labelled-by
 * and label-for ATK relationships.
 *
 * Since: 2.0
 */
PIDGIN_AVAILABLE_IN_ALL
void pidgin_set_accessible_label(GtkWidget *w, GtkLabel *l);

/**
 * pidgin_auto_parent_window:
 * @window:    The window to make transient.
 *
 * Automatically make a window transient to a suitable parent window.
 *
 * Returns: Whether the window was made transient or not.
 *
 * Since: 2.4
 */
PIDGIN_AVAILABLE_IN_2_4
gboolean pidgin_auto_parent_window(GtkWidget *window);

G_END_DECLS

#endif /* _PIDGINUTILS_H_ */

