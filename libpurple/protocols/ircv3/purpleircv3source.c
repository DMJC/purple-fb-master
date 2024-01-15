/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <purple.h>

#include "purpleircv3source.h"

/******************************************************************************
 * Public API
 *****************************************************************************/
void
purple_ircv3_source_parse(const char *source, char **nick, char **user,
                          char **host)
{
	GRegex *regex = NULL;
	GMatchInfo *info = NULL;
	gboolean matched = FALSE;

	g_return_if_fail(!purple_strempty(source));
	g_return_if_fail(nick != NULL || user != NULL || host != NULL);

	/* If we find any \r \n or spaces in our source, it's invalid so we just
	 * bail immediately.
	 */
	matched = g_regex_match_simple("[\r\n ]", source, G_REGEX_DEFAULT,
	                               G_REGEX_MATCH_DEFAULT);
	if(matched) {
		return;
	}

	regex = g_regex_new("^(?P<nick>[^ \r\n!]+)(?:!(?P<user>[^@]+)(?:@(?P<host>[^!@]+))?)?$",
	                    G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);

	matched = g_regex_match(regex, source, G_REGEX_MATCH_DEFAULT, &info);
	if(!matched) {
		if(nick != NULL) {
			*nick = g_strdup(source);
		}

		g_clear_pointer(&info, g_match_info_unref);
		g_clear_pointer(&regex, g_regex_unref);

		return;
	}

	if(nick != NULL) {
		*nick = g_match_info_fetch_named(info, "nick");
	}

	if(user != NULL) {
		char *tmp = g_match_info_fetch_named(info, "user");

		if(!purple_strempty(tmp)) {
			*user = tmp;
		} else {
			g_free(tmp);
		}
	}

	if(host != NULL) {
		char *tmp = g_match_info_fetch_named(info, "host");

		if(!purple_strempty(tmp)) {
			*host = tmp;
		} else {
			g_free(tmp);
		}
	}

	g_clear_pointer(&info, g_match_info_unref);
	g_clear_pointer(&regex, g_regex_unref);
}
