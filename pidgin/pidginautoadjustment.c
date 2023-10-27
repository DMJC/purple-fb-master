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

#include <gtk/gtk.h>

#include "pidginautoadjustment.h"

struct _PidginAutoAdjustment {
	GtkAdjustment parent;

	gboolean auto_scroll;
};

G_DEFINE_TYPE(PidginAutoAdjustment, pidgin_auto_adjustment,
              GTK_TYPE_ADJUSTMENT)

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
pidgin_auto_adjustment_scroll(gpointer data) {
	GtkAdjustment *adjustment = data;
	gdouble upper = 0.0;
	gdouble pagesize = 0.0;

	upper = gtk_adjustment_get_upper(adjustment);
	pagesize = gtk_adjustment_get_page_size(adjustment);

	gtk_adjustment_set_value(adjustment, upper - pagesize);

	return G_SOURCE_REMOVE;
}

static void
pidgin_auto_adjustment_changed(GtkAdjustment *gtk_adjustment) {
	PidginAutoAdjustment *adjustment = PIDGIN_AUTO_ADJUSTMENT(gtk_adjustment);

	if(adjustment->auto_scroll) {
		/* If we set the value here, we interrupt the updates to the other
		 * values of the adjustment which breaks the adjustment.
		 *
		 * The specific behavior we've seen is that the upper property doesn't
		 * get updated when we call set value here. Which means you can't even
		 * scroll down anymore in a scrolled window because you're already at
		 * the upper bound. However, if you scroll up, even if there's no where
		 * to scroll, it will update the adjustment's properties and you can
		 * then scroll down.
		 *
		 * By using a timeout to set the value during the next main loop
		 * iteration we avoid this problem.
		 */
		g_timeout_add(0, pidgin_auto_adjustment_scroll, adjustment);
	}
}

static void
pidgin_auto_adjustment_value_changed(GtkAdjustment *gtk_adjustment) {
	PidginAutoAdjustment *adjustment = PIDGIN_AUTO_ADJUSTMENT(gtk_adjustment);
	gdouble current = 0.0;
	gdouble upper = 0.0;
	gdouble pagesize = 0.0;

	current = gtk_adjustment_get_value(gtk_adjustment);
	upper = gtk_adjustment_get_upper(gtk_adjustment);
	pagesize = gtk_adjustment_get_page_size(gtk_adjustment);

	adjustment->auto_scroll = (current + pagesize >= upper);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_auto_adjustment_init(PidginAutoAdjustment *adjustment) {
	adjustment->auto_scroll = TRUE;
}

static void
pidgin_auto_adjustment_class_init(PidginAutoAdjustmentClass *klass) {
	GtkAdjustmentClass *adjustment_class = GTK_ADJUSTMENT_CLASS(klass);

	adjustment_class->changed = pidgin_auto_adjustment_changed;
	adjustment_class->value_changed = pidgin_auto_adjustment_value_changed;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GtkAdjustment *
pidgin_auto_adjustment_new(void) {
	return g_object_new(PIDGIN_TYPE_AUTO_ADJUSTMENT, NULL);
}

void
pidgin_auto_adjustment_decrement(PidginAutoAdjustment *adjustment) {
	gdouble value = 0.0;

	g_return_if_fail(PIDGIN_IS_AUTO_ADJUSTMENT(adjustment));

	value = gtk_adjustment_get_value(GTK_ADJUSTMENT(adjustment));
	value -= gtk_adjustment_get_page_increment(GTK_ADJUSTMENT(adjustment));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), value);
}

void
pidgin_auto_adjustment_increment(PidginAutoAdjustment *adjustment) {
	gdouble value = 0.0;

	g_return_if_fail(PIDGIN_IS_AUTO_ADJUSTMENT(adjustment));

	value = gtk_adjustment_get_value(GTK_ADJUSTMENT(adjustment));
	value += gtk_adjustment_get_page_increment(GTK_ADJUSTMENT(adjustment));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), value);
}
