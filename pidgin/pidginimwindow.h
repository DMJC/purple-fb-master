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

#ifndef PIDGIN_IM_WINDOW_H
#define PIDGIN_IM_WINDOW_H

#include <glib.h>

#include <gtk/gtk.h>

#include <adwaita.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginIMWindow:
 *
 * A window used to start a new direct message.
 *
 * Since: 3.0.0
 */

#define PIDGIN_TYPE_IM_WINDOW (pidgin_im_window_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginIMWindow, pidgin_im_window,
                     PIDGIN, IM_WINDOW, AdwMessageDialog)

/**
 * pidgin_im_window_new:
 *
 * Creates a new #PidginIMWindow instance.
 *
 * Returns: (transfer full): The new #PidginIMWindow instance.
 */
GtkWidget *pidgin_im_window_new(void);

G_END_DECLS

#endif /* PIDGIN_IM_WINDOW_H */
