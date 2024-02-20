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

#include "purpleversion.h"
#include "purpleversionconsts.h"

const guint purple_major_version = PURPLE_MAJOR_VERSION;
const guint purple_minor_version = PURPLE_MINOR_VERSION;
const guint purple_micro_version = PURPLE_MICRO_VERSION;

const char *purple_version_check(guint required_major, guint required_minor, guint required_micro)
{
	if (required_major > PURPLE_MAJOR_VERSION)
		return "libpurple version too old (major mismatch)";
	if (required_major < PURPLE_MAJOR_VERSION)
		return "libpurple version too new (major mismatch)";
	if (required_minor > PURPLE_MINOR_VERSION)
		return "libpurple version too old (minor mismatch)";
	if ((required_minor == PURPLE_MINOR_VERSION) && (required_micro > PURPLE_MICRO_VERSION))
		return "libpurple version too old (micro mismatch)";
	return NULL;
}
