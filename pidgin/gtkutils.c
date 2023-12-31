/* pidgin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include <purpleconfig.h>

#include <glib/gi18n-lib.h>

#include <purple.h>

#ifdef _WIN32
#  undef small
#  include <shellapi.h>
#endif /*_WIN32*/

#include "gtkrequest.h"
#include "gtkutils.h"

/******************************************************************************
 * Code
 *****************************************************************************/

void
pidgin_retrieve_user_info(PurpleConnection *conn, const char *name) {
	PurpleNotifyUserInfo *info = NULL;
	PurpleProtocol *protocol = NULL;

	protocol = purple_connection_get_protocol(conn);
	if(!PURPLE_IS_PROTOCOL_SERVER(protocol)) {
		return;
	}

	purple_protocol_server_get_info(PURPLE_PROTOCOL_SERVER(protocol), conn,
	                                name);

	info = purple_notify_user_info_new();
	purple_notify_user_info_add_pair_plaintext(info, _("Information"),
	                                           _("Retrieving..."));
	purple_notify_userinfo(conn, name, info, NULL, NULL);
	purple_notify_user_info_destroy(info);
}

void pidgin_retrieve_user_info_in_chat(PurpleConnection *conn, const char *name, int chat)
{
	char *who = NULL;
	PurpleProtocol *protocol = NULL;

	if (chat < 0) {
		pidgin_retrieve_user_info(conn, name);
		return;
	}

	protocol = purple_connection_get_protocol(conn);
	if (protocol != NULL)
		who = purple_protocol_chat_get_user_real_name(PURPLE_PROTOCOL_CHAT(protocol), conn, chat, name);

	pidgin_retrieve_user_info(conn, who ? who : name);
	g_free(who);
}

void
pidgin_set_accessible_label(GtkWidget *w, GtkLabel *l)
{
	GtkAccessible *acc, *label;

	acc = GTK_ACCESSIBLE(w);
	label = GTK_ACCESSIBLE(l);

	/* Make sure mnemonics work */
	gtk_label_set_mnemonic_widget(l, w);

	/* Create the labeled-by relation */
	gtk_accessible_update_relation(acc, GTK_ACCESSIBLE_RELATION_LABELLED_BY,
	                               label, NULL, -1);
}

GtkWidget *
pidgin_add_widget_to_vbox(GtkBox *vbox, const char *widget_label, GtkSizeGroup *sg, GtkWidget *widget, gboolean expand, GtkWidget **p_label)
{
	GtkWidget *hbox;
	GtkWidget *label = NULL;

	if (widget_label) {
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
		gtk_box_append(vbox, hbox);

		label = gtk_label_new_with_mnemonic(widget_label);
		if (sg) {
			gtk_label_set_xalign(GTK_LABEL(label), 0);
			gtk_size_group_add_widget(sg, label);
		}
		gtk_box_append(GTK_BOX(hbox), label);

		gtk_widget_set_hexpand(widget, expand);
		gtk_box_append(GTK_BOX(hbox), widget);
	} else {
		gtk_widget_set_vexpand(widget, expand);
		gtk_box_append(vbox, widget);

		hbox = GTK_WIDGET(vbox);
	}

	if (label) {
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);
		pidgin_set_accessible_label(widget, GTK_LABEL(label));
	}

	if (p_label)
		(*p_label) = label;
	return hbox;
}

gboolean pidgin_auto_parent_window(GtkWidget *widget)
{
	/* This finds the currently active window and makes that the parent window. */
	GList *windows = NULL;
	GtkWindow *parent = NULL;
	gpointer parent_from;
	PurpleNotifyType notify_type;

	parent_from = g_object_get_data(G_OBJECT(widget), "pidgin-parent-from");
	if (purple_request_is_valid_ui_handle(parent_from, NULL)) {
		
		gtk_window_set_transient_for(GTK_WINDOW(widget),
			gtk_window_get_transient_for(
				pidgin_request_get_dialog_window(parent_from)));
		return TRUE;
	}
	if (purple_notify_is_valid_ui_handle(parent_from, &notify_type) &&
		notify_type == PURPLE_NOTIFY_MESSAGE)
	{
		gtk_window_set_transient_for(GTK_WINDOW(widget),
			gtk_window_get_transient_for(GTK_WINDOW(parent_from)));
		return TRUE;
	}

	windows = gtk_window_list_toplevels();
	while (windows) {
		GtkWindow *window = GTK_WINDOW(windows->data);
		windows = g_list_delete_link(windows, windows);

		if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window),
			"pidgin-window-is-closing")))
		{
			parent = gtk_window_get_transient_for(window);
			break;
		}

		if (GTK_WIDGET(window) == widget ||
				!gtk_widget_get_visible(GTK_WIDGET(window))) {
			continue;
		}

		if (gtk_window_is_active(window)) {
			parent = window;
			break;
		}
	}
	g_clear_list(&windows, NULL);
	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(widget), parent);
		return TRUE;
	}
	return FALSE;
}
