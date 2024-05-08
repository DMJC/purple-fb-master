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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301 USA
 */
#include <glib/gi18n-lib.h>

#include "pidginnotifiable.h"

/******************************************************************************
 * GInterface implementation
 *****************************************************************************/
G_DEFINE_INTERFACE(PidginNotifiable, pidgin_notifiable, G_TYPE_INVALID)

static void
pidgin_notifiable_default_init(PidginNotifiableInterface *iface) {
	GParamSpec *pspec = NULL;

	/**
	 * PidginNotifiable:needs-attention:
	 *
	 * Whether or not this needs attention.
	 *
	 * Since: 3.0
	 */
	pspec = g_param_spec_boolean(
		"needs-attention", "needs-attention",
		"Whether or not this notifiable needs attention.",
		FALSE,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	g_object_interface_install_property(iface, pspec);

	/**
	 * PidginNotifiable:notification-count:
	 *
	 * The number of notifications.
	 *
	 * Since: 3.0
	 */
	pspec = g_param_spec_uint(
		"notification-count", "notifications-count",
		"The number of notifications.",
		0, G_MAXUINT, 0,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	g_object_interface_install_property(iface, pspec);
}

/******************************************************************************
 * Public API
 *****************************************************************************/
gboolean
pidgin_notifiable_get_needs_attention(PidginNotifiable *notifiable) {
	gboolean ret = FALSE;

	g_return_val_if_fail(PIDGIN_IS_NOTIFIABLE(notifiable), FALSE);

	g_object_get(G_OBJECT(notifiable), "needs-attention", &ret, NULL);

	return ret;
}

guint
pidgin_notifiable_get_notification_count(PidginNotifiable *notifiable) {
	guint ret = 0;

	g_return_val_if_fail(PIDGIN_IS_NOTIFIABLE(notifiable), 0);

	g_object_get(G_OBJECT(notifiable), "notification-count", &ret, NULL);

	return ret;
}
