/**
 * Copyright (C) 2006 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
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
 */

#define PLUGIN_STATIC_NAME	GntLastlog

#include "internal.h"
#include <purple.h>

#include <gnt.h>
#include <gnttextview.h>
#include <gntwindow.h>

#include <gntconv.h>
#include <gntplugin.h>

static PurpleCmdId cmd;

static gboolean
window_kpress_cb(GntWidget *wid, const char *key, GntTextView *view)
{
	if (key[0] == 27)
	{
		if (purple_strequal(key, GNT_KEY_DOWN)) {
			gnt_text_view_scroll(view, 1);
		} else if (purple_strequal(key, GNT_KEY_UP)) {
			gnt_text_view_scroll(view, -1);
		} else if (purple_strequal(key, GNT_KEY_PGDOWN)) {
			int height = 0;
			gnt_widget_get_size(wid, NULL, &height);
			gnt_text_view_scroll(view, height - 2);
		} else if (purple_strequal(key, GNT_KEY_PGUP)) {
			int height = 0;
			gnt_widget_get_size(wid, NULL, &height);
			gnt_text_view_scroll(view, -(height - 2));
		} else {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

static PurpleCmdRet
lastlog_cb(PurpleConversation *conv, const char *cmd, char **args, char **error, gpointer null)
{
	FinchConv *ggconv = FINCH_CONV(conv);
	char **strings = g_strsplit(gnt_text_view_get_text(GNT_TEXT_VIEW(ggconv->tv)),
	                            "\n", 0);
	GntWidget *win, *tv;
	int i, j;

	win = gnt_window_new();
	gnt_box_set_title(GNT_BOX(win), _("Lastlog"));

	tv = gnt_text_view_new();
	gnt_box_add_widget(GNT_BOX(win), tv);

	gnt_widget_show(win);

	for (i = 0; strings[i]; i++) {
		if (strstr(strings[i], args[0]) != NULL) {
			char **finds = g_strsplit(strings[i], args[0], 0);
			for (j = 0; finds[j]; j++) {
				if (j)
					gnt_text_view_append_text_with_flags(GNT_TEXT_VIEW(tv), args[0], GNT_TEXT_FLAG_BOLD);
				gnt_text_view_append_text_with_flags(GNT_TEXT_VIEW(tv), finds[j], GNT_TEXT_FLAG_NORMAL);
			}
			g_strfreev(finds);
			gnt_text_view_append_text_with_flags(GNT_TEXT_VIEW(tv), "\n", GNT_TEXT_FLAG_NORMAL);
		}
	}

	g_signal_connect(G_OBJECT(win), "key_pressed", G_CALLBACK(window_kpress_cb), tv);
	g_strfreev(strings);
	return PURPLE_CMD_RET_OK;
}

static FinchPluginInfo *
plugin_query(GError **error)
{
	const gchar * const authors[] = {
		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>",
		NULL
	};

	return finch_plugin_info_new(
		"id",           "gntlastlog",
		"name",         N_("GntLastlog"),
		"version",      DISPLAY_VERSION,
		"category",     N_("Utility"),
		"summary",      N_("Lastlog plugin."),
		"description",  N_("Lastlog plugin."),
		"authors",      authors,
		"website",      PURPLE_WEBSITE,
		"abi-version",  PURPLE_ABI_VERSION,
		NULL
	);
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
	cmd = purple_cmd_register("lastlog", "s", PURPLE_CMD_P_DEFAULT,
			PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_IM, NULL,
			/* Translators: The "backlog" here refers to the the conversation buffer/history. */
			lastlog_cb, _("lastlog: Searches for a substring in the backlog."), NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
	purple_cmd_unregister(cmd);
	return TRUE;
}

PURPLE_PLUGIN_INIT(PLUGIN_STATIC_NAME, plugin_query, plugin_load, plugin_unload);
