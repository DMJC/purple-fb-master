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

#include "purpleircv3formatting.h"

#define IRCV3_FORMAT_BOLD (0x02)
#define IRCV3_FORMAT_COLOR (0x03)
#define IRCV3_FORMAT_HEX_COLOR (0x04)
#define IRCV3_FORMAT_ITALIC (0x1d)
#define IRCV3_FORMAT_MONOSPACE (0x11)
#define IRCV3_FORMAT_RESET (0x0f)
#define IRCV3_FORMAT_REVERSE (0x16)
#define IRCV3_FORMAT_STRIKETHROUGH (0x1e)
#define IRCV3_FORMAT_UNDERLINE (0x1f)

/******************************************************************************
 * Helpers
 *****************************************************************************/
static inline gboolean
purple_ircv3_formatting_is_hex_color(const char *text) {
	for(int i = 0; i < 6; i++) {
		if(text[i] == '\0') {
			return FALSE;
		}

		if(!g_ascii_isxdigit(text[i])) {
			return FALSE;
		}
	}

	return TRUE;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
char *
purple_ircv3_formatting_strip(const char *text) {
	GString *str = NULL;

	/* We don't use purple_strempty here because if we're passed an empty
	 * string, we should return a newly allocated empty string.
	 */
	if(text == NULL) {
		return NULL;
	}

	str = g_string_new("");

	for(int i = 0; text[i] != '\0'; i++) {
		switch(text[i]) {
		case IRCV3_FORMAT_BOLD:
		case IRCV3_FORMAT_ITALIC:
		case IRCV3_FORMAT_MONOSPACE:
		case IRCV3_FORMAT_RESET:
		case IRCV3_FORMAT_REVERSE:
		case IRCV3_FORMAT_STRIKETHROUGH:
		case IRCV3_FORMAT_UNDERLINE:
			continue;
			break;
		case IRCV3_FORMAT_COLOR:
			if(g_ascii_isdigit(text[i + 1])) {
				i += 1;

				if(g_ascii_isdigit(text[i + 1])) {
					i += 1;
				}

				if(text[i + 1] == ',' && g_ascii_isdigit(text[i + 2])) {
					i += 2;

					if(g_ascii_isdigit(text[i + 1])) {
						i += 1;
					}
				}
			}

			break;
		case IRCV3_FORMAT_HEX_COLOR:
			if(purple_ircv3_formatting_is_hex_color(&text[i + 1])) {
				i += 6;
			}

			if(text[i + 1] == ',' &&
			   purple_ircv3_formatting_is_hex_color(&text[i + 2]))
			{
				i += 7;
			}

			break;
		default:
			g_string_append_c(str, text[i]);
			break;
		}

	}

	return g_string_free(str, FALSE);
}
