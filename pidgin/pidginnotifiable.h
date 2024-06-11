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

#ifndef PIDGIN_NOTIFIABLE_H
#define PIDGIN_NOTIFIABLE_H

#include <glib.h>
#include <glib-object.h>

#include <purple.h>

#include "pidginversion.h"

G_BEGIN_DECLS

/**
 * PidginNotifiable:
 *
 * An interface for notifiable items.
 *
 * This should be implemented by anything that wants to show attention or a
 * badge in a [class@DisplayWindow].
 *
 * The [property@Notifiable:needs-attention] property is used to determine if
 * a badge and/or other steps should be taken to signify that the user needs to
 * look at this item.
 *
 * The [property@Notifiable:notification-count] determines how many
 * notifications there are which can be used in a badge or other elements.
 *
 * Since: 3.0
 */

#define PIDGIN_TYPE_NOTIFIABLE (pidgin_notifiable_get_type())

PIDGIN_AVAILABLE_IN_3_0
G_DECLARE_INTERFACE(PidginNotifiable, pidgin_notifiable, PIDGIN, NOTIFIABLE,
                    GObject)

struct _PidginNotifiableInterface {
    /*< private >*/
    GTypeInterface parent;

    /*< private >*/
    gpointer reserved[4];
};

/**
 * pidgin_notifiable_get_needs_attention:
 * @notifiable: The instance.
 *
 * Gets whether or not this notifiable needs attention.
 *
 * Returns: %TRUE if attention is needed, otherwise %FALSE.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
gboolean pidgin_notifiable_get_needs_attention(PidginNotifiable *notifiable);

/**
 * pidgin_notifiable_get_notification_count:
 * @notifiable: The instance.
 *
 * Gets the number of notifications.
 *
 * It's up the the consumer to decide if the exact number or an estimate, like
 * `10+` is displayed.
 *
 * Returns: The number of notifications.
 *
 * Since: 3.0
 */
PIDGIN_AVAILABLE_IN_3_0
guint pidgin_notifiable_get_notification_count(PidginNotifiable *notifiable);

G_END_DECLS

#endif /* PIDGIN_NOTIFIABLE_H */
