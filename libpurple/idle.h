/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

#if !defined(PURPLE_GLOBAL_HEADER_INSIDE) && !defined(PURPLE_COMPILATION)
# error "only <purple.h> may be included directly"
#endif

#ifndef PURPLE_IDLE_H
#define PURPLE_IDLE_H

#include <time.h>
#include <glib-object.h>

#include "purpleidleui.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**************************************************************************/
/* Idle API                                                               */
/**************************************************************************/

/**
 * purple_idle_touch:
 *
 * Touch our idle tracker.  This signifies that the user is
 * 'active'.  The conversation code calls this when the
 * user sends an IM, for example.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_idle_touch(void);

/**
 * purple_idle_set:
 *
 * Fake our idle time by setting the time at which our
 * accounts purportedly became idle.  This is used by
 * the I'dle Mak'er plugin.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_idle_set(time_t time);

/**************************************************************************/
/* Idle Subsystem                                                         */
/**************************************************************************/

/**
 * purple_idle_set_ui:
 * @ui: (transfer full): An instance of [iface@IdleUi].
 *
 * Sets the user interface idle reporter.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
void purple_idle_set_ui(PurpleIdleUi *ui);

/**
 * purple_idle_get_ui:
 *
 * Gets the current idle reporter.
 *
 * Returns: (transfer none): The [iface@IdleUi] that is currently in use or
 *          %NULL if no idle reporter is available.
 *
 * Since: 3.0
 */
PURPLE_AVAILABLE_IN_3_0
PurpleIdleUi *purple_idle_get_ui(void);

/**
 * purple_idle_init:
 *
 * Initializes the idle system.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_idle_init(void);

/**
 * purple_idle_uninit:
 *
 * Uninitializes the idle system.
 *
 * Since: 2.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_idle_uninit(void);

G_END_DECLS

#endif /* PURPLE_IDLE_H */
