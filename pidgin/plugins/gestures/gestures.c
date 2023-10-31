/*
 * Mouse gestures plugin for Purple
 *
 * Copyright (C) 2003 Christian Hammond.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include <glib/gi18n-lib.h>

#include <purple.h>

#include <pidgin.h>

#include "gstroke.h"

#define GESTURES_PLUGIN_ID "gtk-x11-gestures"
#define SETTINGS_SCHEMA_ID "im.pidgin.Pidgin.plugin.Gestures"

static void
stroke_close(GtkWidget *widget, void *data)
{
	PurpleConversation *conv;
	PidginConversation *gtkconv;

	conv = (PurpleConversation *)data;

	/* Double-check */
	if (!PIDGIN_IS_PIDGIN_CONVERSATION(conv))
		return;

	gtkconv = PIDGIN_CONVERSATION(conv);

	gstroke_cleanup(gtkconv->history);
	g_object_unref(conv);
}

static void
stroke_prev_tab(GtkWidget *widget, void *data)
{
	PurpleConversation *conv;
	PidginConversation *gtkconv;
	GtkWidget *win;

	conv = (PurpleConversation *)data;
	gtkconv = PIDGIN_CONVERSATION(conv);
	win = gtk_widget_get_toplevel(gtkconv->tab_cont);

	pidgin_display_window_select_previous(PIDGIN_DISPLAY_WINDOW(win));
}

static void
stroke_next_tab(GtkWidget *widget, void *data)
{
	PurpleConversation *conv;
	PidginConversation *gtkconv;
	GtkWidget *win;

	conv = (PurpleConversation *)data;
	gtkconv = PIDGIN_CONVERSATION(conv);
	win = gtk_widget_get_toplevel(gtkconv->tab_cont);

	pidgin_display_window_select_next(PIDGIN_DISPLAY_WINDOW(win));
}

static void
stroke_new_win(GtkWidget *widget, void *data)
{
	GtkWidget *new_win, *old_win;
	PurpleConversation *conv;

	conv    = (PurpleConversation *)data;
	old_win = gtk_widget_get_parent(PIDGIN_CONVERSATION(conv)->tab_cont);

	if(pidgin_display_window_get_count(PIDGIN_DISPLAY_WINDOW(old_win)) <= 1) {
		return;
	}

	new_win = pidgin_display_window_new();

	pidgin_display_window_remove(PIDGIN_DISPLAY_WINDOW(old_win),
	                                  conv);
	pidgin_display_window_add(PIDGIN_DISPLAY_WINDOW(new_win), conv);

	gtk_widget_set_visible(new_win, TRUE);
}

static void
attach_signals(PurpleConversation *conv)
{
	PidginConversation *gtkconv;

	gtkconv = PIDGIN_CONVERSATION(conv);

	gstroke_enable(gtkconv->history);
	gstroke_signal_connect(gtkconv->history, "14789",  stroke_close,    conv);
	gstroke_signal_connect(gtkconv->history, "1456",   stroke_close,    conv);
	gstroke_signal_connect(gtkconv->history, "1489",   stroke_close,    conv);
	gstroke_signal_connect(gtkconv->history, "74123",  stroke_next_tab, conv);
	gstroke_signal_connect(gtkconv->history, "7456",   stroke_next_tab, conv);
	gstroke_signal_connect(gtkconv->history, "96321",  stroke_prev_tab, conv);
	gstroke_signal_connect(gtkconv->history, "9654",   stroke_prev_tab, conv);
	gstroke_signal_connect(gtkconv->history, "25852",  stroke_new_win,  conv);
}

static void
new_conv_cb(PurpleConversation *conv)
{
	if (PIDGIN_IS_PIDGIN_CONVERSATION(conv))
		attach_signals(conv);
}

static void
settings_change_cb(GSettings *settings, char *key, G_GNUC_UNUSED gpointer data)
{
	if(purple_strequal(key, "button")) {
		gstroke_set_mouse_button(g_settings_get_enum(settings, key));
	} else if(purple_strequal(key, "visual")) {
		gstroke_set_draw_strokes(g_settings_get_boolean(settings, key));
	} else {
		g_warning("Got gestures settings change for unknown key: %s", key);
	}
}

static GPluginPluginInfo *
gestures_query(GError **error)
{
	const gchar * const authors[] = {
		"Christian Hammond <chipx86@gnupdate.org>",
		NULL
	};

	return pidgin_plugin_info_new(
		"id",                   GESTURES_PLUGIN_ID,
		"name",                 N_("Mouse Gestures"),
		"version",              DISPLAY_VERSION,
		"category",             N_("User interface"),
		"summary",              N_("Provides support for mouse gestures"),
		"description",          N_("Allows support for mouse gestures in "
		                           "conversation windows. Drag the middle "
		                           "mouse button to perform certain "
		                           "actions:\n"
		                           " • Drag down and then to the right to "
		                           "close a conversation.\n"
		                           " • Drag up and then to the left to "
		                           "switch to the previous conversation.\n"
		                           " • Drag up and then to the right to "
		                           "switch to the next conversation."),
		"authors",              authors,
		"website",              PURPLE_WEBSITE,
		"abi-version",          PURPLE_ABI_VERSION,
		"settings-schema",      SETTINGS_SCHEMA_ID,
		NULL
	);
}

static gboolean
gestures_load(GPluginPlugin *plugin, GError **error)
{
	GSettings *settings = NULL;
	PurpleConversation *conv;
	PurpleConversationManager *manager;
	GList *list;

	settings = g_settings_new_with_backend(SETTINGS_SCHEMA_ID,
	                                       purple_core_get_settings_backend());

	g_signal_connect(settings, "changed",
	                 G_CALLBACK(settings_change_cb), NULL);
	gstroke_set_mouse_button(g_settings_get_enum(settings, "button"));
	gstroke_set_draw_strokes(g_settings_get_boolean(settings, "visual"));
	g_object_unref(settings);

	manager = purple_conversation_manager_get_default();
	list = purple_conversation_manager_get_all(manager);
	while(list != NULL) {
		conv = PURPLE_CONVERSATION(list->data);

		if(!PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
			list = g_list_delete_link(list, list);

			continue;
		}

		attach_signals(conv);

		list = g_list_delete_link(list, list);
	}

	purple_signal_connect(purple_conversations_get_handle(),
						"conversation-created",
						plugin, G_CALLBACK(new_conv_cb), NULL);

	return TRUE;
}

static gboolean
gestures_unload(G_GNUC_UNUSED GPluginPlugin *plugin,
                G_GNUC_UNUSED gboolean shutdown, G_GNUC_UNUSED GError **error)
{
	PurpleConversation *conv;
	PurpleConversationManager *manager;
	PidginConversation *gtkconv;
	GList *list;

	manager = purple_conversation_manager_get_default();
	list = purple_conversation_manager_get_all(manager);

	while(list != NULL) {
		conv = PURPLE_CONVERSATION(list->data);

		if (!PIDGIN_IS_PIDGIN_CONVERSATION(conv)) {
			list = g_list_delete_link(list, list);

			continue;
		}

		gtkconv = PIDGIN_CONVERSATION(conv);

		gstroke_cleanup(gtkconv->history);
		gstroke_disable(gtkconv->history);

		list = g_list_delete_link(list, list);
	}

	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(gestures)
