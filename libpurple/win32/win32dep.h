/*
 * purple
 *
 * File: win32dep.h
 *
 * Copyright (C) 2002-2003, Herman Bloggs <hermanator12002@yahoo.com>
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
 *
 */
#ifndef PURPLE_WIN32_WIN32DEP_H
#define PURPLE_WIN32_WIN32DEP_H

#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#include <process.h>
#include "libc_interface.h"

#include "purpleversion.h"

G_BEGIN_DECLS

/* rpcndr.h defines small as char, causing problems, so we need to undefine it */
#undef small

/*
 *  PROTOS
 */

/**
 ** win32dep.c
 **/

/* Determine Purple paths */
PURPLE_AVAILABLE_IN_3_0
const char *wpurple_bin_dir(void);
PURPLE_AVAILABLE_IN_ALL
const char *wpurple_data_dir(void);
PURPLE_AVAILABLE_IN_ALL
const char *wpurple_lib_dir(const char *subdir);
PURPLE_AVAILABLE_IN_ALL
const char *wpurple_locale_dir(void);
PURPLE_AVAILABLE_IN_3_0
const char *wpurple_sysconf_dir(void);

/*
 * The following API is private.
 */

/* init / cleanup */
void wpurple_init(void);
void wpurple_cleanup(void);

G_END_DECLS

#endif /* PURPLE_WIN32_WIN32DEP_H */
