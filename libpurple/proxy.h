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

#ifndef PURPLE_PROXY_H
#define PURPLE_PROXY_H

#include <glib.h>
#include <gio/gio.h>

#include "purpleaccount.h"
#include "purpleproxyinfo.h"
#include "purpleversion.h"

G_BEGIN_DECLS

/**************************************************************************/
/* Global Proxy API                                                       */
/**************************************************************************/

/**
 * purple_global_proxy_get_info:
 *
 * Returns purple's global proxy information.
 *
 * Returns: (transfer none): The global proxy information.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleProxyInfo *purple_global_proxy_get_info(void);

/**
 * purple_global_proxy_set_info:
 * @info:     The proxy information.
 *
 * Set purple's global proxy information.
 *
 * Since: 2.6.0
 */
PURPLE_AVAILABLE_IN_2_6
void purple_global_proxy_set_info(PurpleProxyInfo *info);

/**************************************************************************/
/* Proxy API                                                              */
/**************************************************************************/

/**
 * purple_proxy_init:
 *
 * Initializes the proxy subsystem.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_proxy_init(void);

/**
 * purple_proxy_uninit:
 *
 * Uninitializes the proxy subsystem.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
void purple_proxy_uninit(void);

/**
 * purple_proxy_get_setup:
 * @account: The account for which the configuration is needed.
 *
 * Returns configuration of a proxy.
 *
 * Returns: (transfer none): The configuration of a proxy.
 *
 * Since: 2.0.0
 */
PURPLE_AVAILABLE_IN_ALL
PurpleProxyInfo *purple_proxy_get_setup(PurpleAccount *account);

/**
 * purple_proxy_get_proxy_resolver:
 * @account: The account for which to get the proxy resolver.
 * @error: Return location for a GError, or NULL.
 *
 * Returns a #GProxyResolver capable of resolving which proxy
 * to use for this account, if any. This object can be given to a
 * #GSocketClient for automatic proxy handling or can be used
 * directly if desired.
 *
 * Returns: (transfer full): NULL if there was an error with the
 *         account's (or system) proxy settings, or a reference to
 *         a #GProxyResolver on success.
 *
 * Since: 3.0.0
 */
PURPLE_AVAILABLE_IN_3_0
GProxyResolver *purple_proxy_get_proxy_resolver(PurpleAccount *account,
		GError **error);

G_END_DECLS

#endif /* PURPLE_PROXY_H */
