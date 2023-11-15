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

#include <errno.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gdk/gdkkeysyms.h>

#include <purple.h>

#ifdef _WIN32
#  undef small
#  include <shellapi.h>
#endif /*_WIN32*/

#include "gtkdialogs.h"
#include "gtkrequest.h"
#include "gtkutils.h"
#include "pidginaccountchooser.h"
#include "pidgincore.h"

/******************************************************************************
 * Enums
 *****************************************************************************/

enum {
	AOP_ICON_COLUMN,
	AOP_NAME_COLUMN,
	AOP_DATA_COLUMN,
	AOP_COLUMN_COUNT
};

enum {
	COMPLETION_DISPLAYED_COLUMN,  /* displayed completion value */
	COMPLETION_BUDDY_COLUMN,      /* buddy name */
	COMPLETION_NORMALIZED_COLUMN, /* UTF-8 normalized & casefolded buddy name */
	COMPLETION_COMPARISON_COLUMN, /* UTF-8 normalized & casefolded value for comparison */
	COMPLETION_ACCOUNT_COLUMN,    /* account */
	COMPLETION_COLUMN_COUNT
};

/******************************************************************************
 * Structs
 *****************************************************************************/

typedef struct
{
	GtkWidget *entry;
	GtkWidget *chooser;

	PidginFilterBuddyCompletionEntryFunc filter_func;
	gpointer filter_func_user_data;

	GtkListStore *store;
} PidginCompletionData;

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

static gboolean
buddyname_completion_match_func(GtkEntryCompletion *completion,
                                const char *key, GtkTreeIter *iter,
                                G_GNUC_UNUSED gpointer user_data)
{
	GtkTreeModel *model;
	GValue val1;
	GValue val2;
	const char *tmp;

	model = gtk_entry_completion_get_model(completion);

	val1.g_type = 0;
	gtk_tree_model_get_value(model, iter, COMPLETION_NORMALIZED_COLUMN, &val1);
	tmp = g_value_get_string(&val1);
	if (tmp != NULL && g_str_has_prefix(tmp, key)) {
		g_value_unset(&val1);
		return TRUE;
	}
	g_value_unset(&val1);

	val2.g_type = 0;
	gtk_tree_model_get_value(model, iter, COMPLETION_COMPARISON_COLUMN, &val2);
	tmp = g_value_get_string(&val2);
	if (tmp != NULL && g_str_has_prefix(tmp, key)) {
		g_value_unset(&val2);
		return TRUE;
	}
	g_value_unset(&val2);

	return FALSE;
}

static gboolean
buddyname_completion_match_selected_cb(G_GNUC_UNUSED GtkEntryCompletion *completion,
                                       GtkTreeModel *model, GtkTreeIter *iter,
                                       PidginCompletionData *data)
{
	GValue val;
	PurpleAccount *account = NULL;

	val.g_type = 0;
	gtk_tree_model_get_value(model, iter, COMPLETION_BUDDY_COLUMN, &val);
	gtk_editable_set_text(GTK_EDITABLE(data->entry), g_value_get_string(&val));
	g_value_unset(&val);

	gtk_tree_model_get_value(model, iter, COMPLETION_ACCOUNT_COLUMN, &val);
	account = g_value_get_pointer(&val);
	g_value_unset(&val);

	if(!PURPLE_IS_ACCOUNT(account)) {
		return TRUE;
	}

	if(PIDGIN_IS_ACCOUNT_CHOOSER(data->chooser)) {
		pidgin_account_chooser_set_selected(PIDGIN_ACCOUNT_CHOOSER(data->chooser),
		                                    account);
	}

	return TRUE;
}

static void
add_buddyname_autocomplete_entry(GtkListStore *store, const char *buddy_alias, const char *contact_alias,
								  const PurpleAccount *account, const char *buddyname)
{
	GtkTreeIter iter;
	gboolean completion_added = FALSE;
	gchar *normalized_buddyname;
	gchar *tmp;

	tmp = g_utf8_normalize(buddyname, -1, G_NORMALIZE_DEFAULT);
	normalized_buddyname = g_utf8_casefold(tmp, -1);
	g_free(tmp);

	/* There's no sense listing things like: 'xxx "xxx"'
	   when the name and buddy alias match. */
	if (buddy_alias && !purple_strequal(buddy_alias, buddyname)) {
		char *completion_entry = g_strdup_printf("%s \"%s\"", buddyname, buddy_alias);
		char *tmp2 = g_utf8_normalize(buddy_alias, -1, G_NORMALIZE_DEFAULT);

		tmp = g_utf8_casefold(tmp2, -1);
		g_free(tmp2);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				COMPLETION_DISPLAYED_COLUMN, completion_entry,
				COMPLETION_BUDDY_COLUMN, buddyname,
				COMPLETION_NORMALIZED_COLUMN, normalized_buddyname,
				COMPLETION_COMPARISON_COLUMN, tmp,
				COMPLETION_ACCOUNT_COLUMN, account,
				-1);
		g_free(completion_entry);
		g_free(tmp);
		completion_added = TRUE;
	}

	/* There's no sense listing things like: 'xxx "xxx"'
	   when the name and contact alias match. */
	if (contact_alias && !purple_strequal(contact_alias, buddyname)) {
		/* We don't want duplicates when the contact and buddy alias match. */
		if (!purple_strequal(contact_alias, buddy_alias)) {
			char *completion_entry = g_strdup_printf("%s \"%s\"",
							buddyname, contact_alias);
			char *tmp2 = g_utf8_normalize(contact_alias, -1, G_NORMALIZE_DEFAULT);

			tmp = g_utf8_casefold(tmp2, -1);
			g_free(tmp2);

			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					COMPLETION_DISPLAYED_COLUMN, completion_entry,
					COMPLETION_BUDDY_COLUMN, buddyname,
					COMPLETION_NORMALIZED_COLUMN, normalized_buddyname,
					COMPLETION_COMPARISON_COLUMN, tmp,
					COMPLETION_ACCOUNT_COLUMN, account,
					-1);
			g_free(completion_entry);
			g_free(tmp);
			completion_added = TRUE;
		}
	}

	if (completion_added == FALSE) {
		/* Add the buddy's name. */
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				COMPLETION_DISPLAYED_COLUMN, buddyname,
				COMPLETION_BUDDY_COLUMN, buddyname,
				COMPLETION_NORMALIZED_COLUMN, normalized_buddyname,
				COMPLETION_COMPARISON_COLUMN, NULL,
				COMPLETION_ACCOUNT_COLUMN, account,
				-1);
	}

	g_free(normalized_buddyname);
}

static void
add_completion_list(PidginCompletionData *data)
{
	PurpleBlistNode *gnode, *cnode, *bnode;
	PidginFilterBuddyCompletionEntryFunc filter_func = data->filter_func;
	gpointer user_data = data->filter_func_user_data;
	gchar *alias;

	gtk_list_store_clear(data->store);

	for (gnode = purple_blist_get_default_root(); gnode != NULL;
	     gnode = gnode->next) {
		if (!PURPLE_IS_GROUP(gnode))
			continue;

		for (cnode = gnode->child; cnode != NULL; cnode = cnode->next)
		{
			if (!PURPLE_IS_META_CONTACT(cnode))
				continue;

			g_object_get(cnode, "alias", &alias, NULL);

			for (bnode = cnode->child; bnode != NULL; bnode = bnode->next)
			{
				PidginBuddyCompletionEntry entry;
				entry.is_buddy = TRUE;
				entry.buddy = (PurpleBuddy *) bnode;

				if (filter_func(&entry, user_data)) {
					add_buddyname_autocomplete_entry(data->store,
														alias,
														purple_buddy_get_contact_alias(entry.buddy),
														purple_buddy_get_account(entry.buddy),
														purple_buddy_get_name(entry.buddy)
													 );
				}
			}

			g_free(alias);
		}
	}
}

static void
repopulate_autocomplete(G_GNUC_UNUSED gpointer something, gpointer data)
{
	add_completion_list(data);
}

static void
autocomplete_account_added_cb(G_GNUC_UNUSED PurpleAccountManager *manager,
                              G_GNUC_UNUSED PurpleAccount *account,
                              gpointer data)
{
	add_completion_list(data);
}

static void
autocomplete_account_removed_cb(G_GNUC_UNUSED PurpleAccountManager *manager,
                                G_GNUC_UNUSED PurpleAccount *account,
                                gpointer data)
{
	add_completion_list(data);
}

static void
buddyname_autocomplete_destroyed_cb(GtkWidget *widget, gpointer data)
{
	PurpleAccountManager *manager = purple_account_manager_get_default();

	purple_signals_disconnect_by_handle(widget);

	g_signal_handlers_disconnect_by_func(manager,
	                                     autocomplete_account_added_cb, data);
	g_signal_handlers_disconnect_by_func(manager,
	                                     autocomplete_account_removed_cb,
	                                     data);

	g_free(data);
}

void
pidgin_setup_screenname_autocomplete(
        GtkWidget *entry, GtkWidget *chooser,
        PidginFilterBuddyCompletionEntryFunc filter_func, gpointer user_data)
{
	PidginCompletionData *data;
	PurpleAccountManager *manager = NULL;

	/*
	 * Store the displayed completion value, the buddy name, the UTF-8
	 * normalized & casefolded buddy name, the UTF-8 normalized &
	 * casefolded value for comparison, and the account.
	 */
	GtkListStore *store;

	GtkEntryCompletion *completion;

	data = g_new0(PidginCompletionData, 1);
	store = gtk_list_store_new(COMPLETION_COLUMN_COUNT, G_TYPE_STRING,
	                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                           G_TYPE_POINTER);

	data->entry = entry;
	data->chooser = chooser;
	if (filter_func == NULL) {
		data->filter_func = pidgin_screenname_autocomplete_default_filter;
		data->filter_func_user_data = NULL;
	} else {
		data->filter_func = filter_func;
		data->filter_func_user_data = user_data;
	}
	data->store = store;

	add_completion_list(data);

	/* Sort the completion list by buddy name */
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),
	                                     COMPLETION_BUDDY_COLUMN,
	                                     GTK_SORT_ASCENDING);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_match_func(completion, buddyname_completion_match_func, NULL, NULL);

	g_signal_connect(G_OBJECT(completion), "match-selected",
		G_CALLBACK(buddyname_completion_match_selected_cb), data);

	gtk_entry_set_completion(GTK_ENTRY(entry), completion);
	g_object_unref(completion);

	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_entry_completion_set_text_column(completion, COMPLETION_DISPLAYED_COLUMN);

	purple_signal_connect(purple_connections_get_handle(), "signed-on", entry,
						G_CALLBACK(repopulate_autocomplete), data);
	purple_signal_connect(purple_connections_get_handle(), "signed-off", entry,
						G_CALLBACK(repopulate_autocomplete), data);

	manager = purple_account_manager_get_default();
	g_signal_connect(manager, "added",
	                 G_CALLBACK(autocomplete_account_added_cb), data);
	g_signal_connect(manager, "removed",
	                 G_CALLBACK(autocomplete_account_removed_cb), data);

	g_signal_connect(G_OBJECT(entry), "destroy", G_CALLBACK(buddyname_autocomplete_destroyed_cb), data);
}

gboolean
pidgin_screenname_autocomplete_default_filter(const PidginBuddyCompletionEntry *completion_entry, gpointer all_accounts) {
	gboolean all = GPOINTER_TO_INT(all_accounts);

	return all || purple_account_is_connected(purple_buddy_get_account(completion_entry->buddy));
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
