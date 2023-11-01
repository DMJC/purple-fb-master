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

#ifndef PIDGIN_AUTO_ADJUSTMENT_H
#define PIDGIN_AUTO_ADJUSTMENT_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginAutoAdjustment:
 *
 * This is a simple subclass of [class@Gtk.Adjustment] that has helpers for
 * keyboard navigation as well as the ability to automatically scroll to the
 * max when new items are added if the widget was already scrolled all the
 * way to the bottom.
 *
 * Since: 3.0.0
 */

#define PIDGIN_TYPE_AUTO_ADJUSTMENT (pidgin_auto_adjustment_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_FINAL_TYPE(PidginAutoAdjustment, pidgin_auto_adjustment, PIDGIN,
                     AUTO_ADJUSTMENT, GtkAdjustment)

/**
 * pidgin_auto_adjustment_new:
 *
 * Creates a new #PidginAutoAdjustment.
 *
 * Returns: (transfer full): The new #PidginAutoAdjustment instance.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
GtkAdjustment *pidgin_auto_adjustment_new(void);

/**
 * pidgin_auto_adjustment_decrement:
 * @auto_scroller: The #PidginAutoAdjustment instance.
 *
 * Decrements the value of @auto_scroller by a page increment.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_auto_adjustment_decrement(PidginAutoAdjustment *auto_scroller);

/**
 * pidgin_auto_adjustment_increment:
 * @auto_scroller: The #PidginAutoAdjustment instance.
 *
 * Increments the value of @auto_scroller by a page increment.
 *
 * Since: 3.0.0
 */
PIDGIN_AVAILABLE_IN_3_0
void pidgin_auto_adjustment_increment(PidginAutoAdjustment *auto_scroller);

G_END_DECLS

#endif /* PIDGIN_AUTO_ADJUSTMENT_H */
