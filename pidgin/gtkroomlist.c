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

#include "internal.h"
#include "pidgin.h"
#include "gtkutils.h"
#include "pidginaccountchooser.h"
#include "pidginstock.h"
#include "pidgintooltip.h"

#include <purple.h>

#include "gtkroomlist.h"

#define PIDGIN_TYPE_ROOMLIST_DIALOG (pidgin_roomlist_dialog_get_type())
G_DECLARE_FINAL_TYPE(PidginRoomlistDialog, pidgin_roomlist_dialog, PIDGIN,
                     ROOMLIST_DIALOG, GtkDialog)

struct _PidginRoomlistDialog {
	GtkDialog parent;

	GtkWidget *account_widget;
	GtkWidget *progress;
	GtkWidget *sw;

	GtkWidget *stop_button;
	GtkWidget *list_button;
	GtkWidget *add_button;
	GtkWidget *join_button;
	GtkWidget *close_button;

	PurpleAccount *account;
	PurpleRoomlist *roomlist;

	gboolean pg_needs_pulse;
	guint pg_update_to;
};

G_DEFINE_TYPE(PidginRoomlistDialog, pidgin_roomlist_dialog, GTK_TYPE_DIALOG)

typedef struct {
	PidginRoomlistDialog *dialog;
	GtkTreeStore *model;
	GtkWidget *tree;
	GHashTable *cats; /* Meow. */
	gint num_rooms, total_rooms;
	GtkWidget *tipwindow;
	GdkRectangle tip_rect;
	PangoLayout *tip_layout;
	PangoLayout *tip_name_layout;
	int tip_height;
	int tip_width;
	int tip_name_height;
	int tip_name_width;
} PidginRoomlist;

enum {
	NAME_COLUMN = 0,
	ROOM_COLUMN,
	NUM_OF_COLUMNS,
};

static GList *roomlists = NULL;

static gint delete_win_cb(GtkWidget *w, GdkEventAny *e, gpointer d)
{
	PidginRoomlistDialog *dialog = PIDGIN_ROOMLIST_DIALOG(w);

	if (dialog->roomlist && purple_roomlist_get_in_progress(dialog->roomlist))
		purple_roomlist_cancel_get_list(dialog->roomlist);

	if (dialog->pg_update_to > 0)
		g_source_remove(dialog->pg_update_to);

	if (dialog->roomlist) {
		PidginRoomlist *rl = purple_roomlist_get_ui_data(dialog->roomlist);

		if (dialog->pg_update_to > 0)
			/* yes, that's right, unref it twice. */
			g_object_unref(dialog->roomlist);

		if (rl)
			rl->dialog = NULL;
		g_object_unref(dialog->roomlist);
	}

	dialog->progress = NULL;

	return FALSE;
}

static void
dialog_select_account_cb(GtkWidget *chooser, PidginRoomlistDialog *dialog)
{
	PurpleAccount *account = pidgin_account_chooser_get_selected(chooser);
	gboolean change = (account != dialog->account);
	dialog->account = account;

	if (change && dialog->roomlist) {
		PidginRoomlist *rl = purple_roomlist_get_ui_data(dialog->roomlist);
		if (rl->tree) {
			gtk_widget_destroy(rl->tree);
			rl->tree = NULL;
		}
		g_object_unref(dialog->roomlist);
		dialog->roomlist = NULL;
	}
}

static void list_button_cb(GtkButton *button, PidginRoomlistDialog *dialog)
{
	PurpleConnection *gc;
	PidginRoomlist *rl;

	gc = purple_account_get_connection(dialog->account);
	if (!gc)
		return;

	if (dialog->roomlist != NULL) {
		rl = purple_roomlist_get_ui_data(dialog->roomlist);
		gtk_widget_destroy(rl->tree);
		g_object_unref(dialog->roomlist);
	}

	dialog->roomlist = purple_roomlist_get_list(gc);
	if (!dialog->roomlist)
		return;
	g_object_ref(dialog->roomlist);
	rl = purple_roomlist_get_ui_data(dialog->roomlist);
	rl->dialog = dialog;

	gtk_widget_set_sensitive(dialog->account_widget, FALSE);

	gtk_container_add(GTK_CONTAINER(dialog->sw), rl->tree);

	/* some protocols (not bundled with libpurple) finish getting their
	 * room list immediately */
	if(purple_roomlist_get_in_progress(dialog->roomlist)) {
		gtk_widget_set_sensitive(dialog->stop_button, TRUE);
		gtk_widget_set_sensitive(dialog->list_button, FALSE);
	} else {
		gtk_widget_set_sensitive(dialog->stop_button, FALSE);
		gtk_widget_set_sensitive(dialog->list_button, TRUE);
	}
	gtk_widget_set_sensitive(dialog->add_button, FALSE);
	gtk_widget_set_sensitive(dialog->join_button, FALSE);
}

static void stop_button_cb(GtkButton *button, PidginRoomlistDialog *dialog)
{
	purple_roomlist_cancel_get_list(dialog->roomlist);

	gtk_widget_set_sensitive(dialog->account_widget, TRUE);

	gtk_widget_set_sensitive(dialog->stop_button, FALSE);
	gtk_widget_set_sensitive(dialog->list_button, TRUE);
	gtk_widget_set_sensitive(dialog->add_button, FALSE);
	gtk_widget_set_sensitive(dialog->join_button, FALSE);
}

struct _menu_cb_info {
	PurpleRoomlist *list;
	PurpleRoomlistRoom *room;
};

static void
selection_changed_cb(GtkTreeSelection *selection, PidginRoomlist *grl) {
	GtkTreeIter iter;
	GValue val;
	PurpleRoomlistRoom *room;
	static struct _menu_cb_info *info;
	PidginRoomlistDialog *dialog = grl->dialog;

	if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		val.g_type = 0;
		gtk_tree_model_get_value(GTK_TREE_MODEL(grl->model), &iter, ROOM_COLUMN, &val);
		room = g_value_get_pointer(&val);
		if (!room || !(purple_roomlist_room_get_room_type(room) & PURPLE_ROOMLIST_ROOMTYPE_ROOM)) {
			gtk_widget_set_sensitive(dialog->join_button, FALSE);
			gtk_widget_set_sensitive(dialog->add_button, FALSE);
			return;
		}

		info = g_new0(struct _menu_cb_info, 1);
		info->list = dialog->roomlist;
		info->room = room;

		g_object_set_data_full(G_OBJECT(dialog->join_button), "room-info",
							   info, g_free);
		g_object_set_data(G_OBJECT(dialog->add_button), "room-info", info);

		gtk_widget_set_sensitive(dialog->add_button, TRUE);
		gtk_widget_set_sensitive(dialog->join_button, TRUE);
	} else {
		gtk_widget_set_sensitive(dialog->add_button, FALSE);
		gtk_widget_set_sensitive(dialog->join_button, FALSE);
	}
}

static void do_add_room_cb(GtkWidget *w, struct _menu_cb_info *info)
{
	char *name;
	PurpleAccount *account = purple_roomlist_get_account(info->list);
	PurpleConnection *gc = purple_account_get_connection(account);
	PurpleProtocol *protocol = NULL;

	if(gc != NULL)
		protocol = purple_connection_get_protocol(gc);

	if(protocol != NULL && PURPLE_PROTOCOL_IMPLEMENTS(protocol, ROOMLIST, room_serialize))
		name = purple_protocol_roomlist_iface_room_serialize(protocol, info->room);
	else
		name = g_strdup(purple_roomlist_room_get_name(info->room));

	purple_blist_request_add_chat(account, NULL, NULL, name);

	g_free(name);
}

static void add_room_to_blist_cb(GtkButton *button, PidginRoomlistDialog *dialog)
{
	PurpleRoomlist *rl = dialog->roomlist;
	PidginRoomlist *grl = purple_roomlist_get_ui_data(rl);
	struct _menu_cb_info *info = g_object_get_data(G_OBJECT(button), "room-info");

	if(info != NULL)
		do_add_room_cb(grl->tree, info);
}

static void do_join_cb(GtkWidget *w, struct _menu_cb_info *info)
{
	purple_roomlist_room_join(info->list, info->room);
}

static void join_button_cb(GtkButton *button, PidginRoomlistDialog *dialog)
{
	PurpleRoomlist *rl = dialog->roomlist;
	PidginRoomlist *grl = purple_roomlist_get_ui_data(rl);
	struct _menu_cb_info *info = g_object_get_data(G_OBJECT(button), "room-info");

	if(info != NULL)
		do_join_cb(grl->tree, info);
}

static void row_activated_cb(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *arg2,
                      PurpleRoomlist *list)
{
	PidginRoomlist *grl = purple_roomlist_get_ui_data(list);
	GtkTreeIter iter;
	PurpleRoomlistRoom *room;
	GValue val;
	struct _menu_cb_info info;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(grl->model), &iter, path);
	val.g_type = 0;
	gtk_tree_model_get_value(GTK_TREE_MODEL(grl->model), &iter, ROOM_COLUMN, &val);
	room = g_value_get_pointer(&val);
	if (!room || !(purple_roomlist_room_get_room_type(room) & PURPLE_ROOMLIST_ROOMTYPE_ROOM))
		return;

	info.list = list;
	info.room = room;

	do_join_cb(GTK_WIDGET(tv), &info);
}

static gboolean room_click_cb(GtkWidget *tv, GdkEventButton *event, PurpleRoomlist *list)
{
	GtkTreePath *path;
	PidginRoomlist *grl = purple_roomlist_get_ui_data(list);
	GValue val;
	PurpleRoomlistRoom *room;
	GtkTreeIter iter;
	GtkWidget *menu;
	static struct _menu_cb_info info; /* XXX? */

	if (!gdk_event_triggers_context_menu((GdkEvent *)event))
		return FALSE;

	/* Here we figure out which room was clicked */
	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tv), event->x, event->y, &path, NULL, NULL, NULL))
		return FALSE;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(grl->model), &iter, path);
	gtk_tree_path_free(path);
	val.g_type = 0;
	gtk_tree_model_get_value (GTK_TREE_MODEL(grl->model), &iter, ROOM_COLUMN, &val);
	room = g_value_get_pointer(&val);

	if (!room || !(purple_roomlist_room_get_room_type(room) & PURPLE_ROOMLIST_ROOMTYPE_ROOM))
		return FALSE;

	info.list = list;
	info.room = room;

	menu = gtk_menu_new();
	pidgin_new_menu_item(menu, _("_Join"), PIDGIN_STOCK_CHAT,
                        G_CALLBACK(do_join_cb), &info);
	pidgin_new_menu_item(menu, _("_Add"), GTK_STOCK_ADD,
                        G_CALLBACK(do_add_room_cb), &info);

	gtk_widget_show_all(menu);
	gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

	return FALSE;
}

static void row_expanded_cb(GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, gpointer user_data)
{
	PurpleRoomlist *list = user_data;
	PurpleRoomlistRoom *category;
	GValue val;

	val.g_type = 0;
	gtk_tree_model_get_value(gtk_tree_view_get_model(treeview), arg1, ROOM_COLUMN, &val);
	category = g_value_get_pointer(&val);

	if (!purple_roomlist_room_get_expanded_once(category)) {
		purple_roomlist_expand_category(list, category);
		purple_roomlist_room_set_expanded_once(category, TRUE);
	}
}

#define SMALL_SPACE 6
#define TOOLTIP_BORDER 12

static gboolean
pidgin_roomlist_paint_tooltip(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	PurpleRoomlist *list = user_data;
	PidginRoomlist *grl = purple_roomlist_get_ui_data(list);
	int current_height, max_width;
	int max_text_width;
	GtkTextDirection dir = gtk_widget_get_direction(GTK_WIDGET(grl->tree));
	GtkStyleContext *context;

	context = gtk_widget_get_style_context(grl->tipwindow);
	gtk_style_context_add_class(context, GTK_STYLE_CLASS_TOOLTIP);

	max_text_width = MAX(grl->tip_width, grl->tip_name_width);
	max_width = TOOLTIP_BORDER + SMALL_SPACE + max_text_width + TOOLTIP_BORDER;

	current_height = 12;

	if (dir == GTK_TEXT_DIR_RTL) {
		gtk_render_layout(context, cr,
		                  max_width - (TOOLTIP_BORDER + SMALL_SPACE) - PANGO_PIXELS(600000),
		                  current_height,
		                  grl->tip_name_layout);
	} else {
		gtk_render_layout(context, cr,
		                  TOOLTIP_BORDER + SMALL_SPACE,
		                  current_height,
		                  grl->tip_name_layout);
	}
	if (dir != GTK_TEXT_DIR_RTL) {
		gtk_render_layout(context, cr,
		                  TOOLTIP_BORDER + SMALL_SPACE,
		                  current_height + grl->tip_name_height,
		                  grl->tip_layout);
	} else {
		gtk_render_layout(context, cr,
		                  max_width - (TOOLTIP_BORDER + SMALL_SPACE) - PANGO_PIXELS(600000),
		                  current_height + grl->tip_name_height,
		                  grl->tip_layout);
	}

	return FALSE;
}

static gboolean pidgin_roomlist_create_tip(PurpleRoomlist *list, GtkTreePath *path)
{
	PidginRoomlist *grl = purple_roomlist_get_ui_data(list);
	PurpleRoomlistRoom *room;
	GtkTreeIter iter;
	GValue val;
	gchar *name, *tmp, *node_name;
	GString *tooltip_text = NULL;
	GList *l, *k;
	gint j;
	gboolean first = TRUE;

#if 0
	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tv), grl->tip_rect.x, grl->tip_rect.y + (grl->tip_rect.height/2),
		&path, NULL, NULL, NULL))
		return FALSE;
#endif
	gtk_tree_model_get_iter(GTK_TREE_MODEL(grl->model), &iter, path);

	val.g_type = 0;
	gtk_tree_model_get_value(GTK_TREE_MODEL(grl->model), &iter, ROOM_COLUMN, &val);
	room = g_value_get_pointer(&val);

	if (!room || !(purple_roomlist_room_get_room_type(room) & PURPLE_ROOMLIST_ROOMTYPE_ROOM))
		return FALSE;

	tooltip_text = g_string_new("");
	gtk_tree_model_get(GTK_TREE_MODEL(grl->model), &iter, NAME_COLUMN, &name, -1);

	for (j = NUM_OF_COLUMNS,
				l = purple_roomlist_room_get_fields(room),
				k = purple_roomlist_get_fields(list);
			l && k; j++, l = l->next, k = k->next)
	{
		PurpleRoomlistField *f = k->data;
		gchar *label;
		if (purple_roomlist_field_get_hidden(f))
			continue;
		label = g_markup_escape_text(purple_roomlist_field_get_label(f), -1);
		switch (purple_roomlist_field_get_field_type(f)) {
			case PURPLE_ROOMLIST_FIELD_BOOL:
				g_string_append_printf(tooltip_text, "%s<b>%s:</b> %s", first ? "" : "\n", label, l->data ? "True" : "False");
				break;
			case PURPLE_ROOMLIST_FIELD_INT:
				g_string_append_printf(tooltip_text, "%s<b>%s:</b> %d", first ? "" : "\n", label, GPOINTER_TO_INT(l->data));
				break;
			case PURPLE_ROOMLIST_FIELD_STRING:
				tmp = g_markup_escape_text((char *)l->data, -1);
				g_string_append_printf(tooltip_text, "%s<b>%s:</b> %s", first ? "" : "\n", label, tmp);
				g_free(tmp);
				break;
		}
		first = FALSE;
		g_free(label);
	}

	grl->tip_layout = gtk_widget_create_pango_layout(grl->tipwindow, NULL);
	grl->tip_name_layout = gtk_widget_create_pango_layout(grl->tipwindow, NULL);

	tmp = g_markup_escape_text(name, -1);
	g_free(name);
	node_name = g_strdup_printf("<span size='x-large' weight='bold'>%s</span>", tmp);
	g_free(tmp);

	pango_layout_set_markup(grl->tip_layout, tooltip_text->str, -1);
	pango_layout_set_wrap(grl->tip_layout, PANGO_WRAP_WORD);
	pango_layout_set_width(grl->tip_layout, 600000);

	pango_layout_get_size (grl->tip_layout, &grl->tip_width, &grl->tip_height);
	grl->tip_width = PANGO_PIXELS(grl->tip_width);
	grl->tip_height = PANGO_PIXELS(grl->tip_height);

	pango_layout_set_markup(grl->tip_name_layout, node_name, -1);
	pango_layout_set_wrap(grl->tip_name_layout, PANGO_WRAP_WORD);
	pango_layout_set_width(grl->tip_name_layout, 600000);

	pango_layout_get_size (grl->tip_name_layout, &grl->tip_name_width, &grl->tip_name_height);
	grl->tip_name_width = PANGO_PIXELS(grl->tip_name_width) + SMALL_SPACE;
	grl->tip_name_height = MAX(PANGO_PIXELS(grl->tip_name_height), SMALL_SPACE);

	g_free(node_name);
	g_string_free(tooltip_text, TRUE);

	return TRUE;
}

static gboolean
pidgin_roomlist_create_tooltip(GtkWidget *widget, GtkTreePath *path,
		gpointer data, int *w, int *h)
{
	PurpleRoomlist *list = data;
	PidginRoomlist *grl = purple_roomlist_get_ui_data(list);
	grl->tipwindow = widget;
	if (!pidgin_roomlist_create_tip(data, path))
		return FALSE;
	if (w)
		*w = TOOLTIP_BORDER + SMALL_SPACE +
			MAX(grl->tip_width, grl->tip_name_width) + TOOLTIP_BORDER;
	if (h)
		*h = TOOLTIP_BORDER + grl->tip_height + grl->tip_name_height
			+ TOOLTIP_BORDER;
	return TRUE;
}

static gboolean account_filter_func(PurpleAccount *account)
{
	PurpleConnection *conn = purple_account_get_connection(account);
	PurpleProtocol *protocol = NULL;

	if (conn && PURPLE_CONNECTION_IS_CONNECTED(conn))
		protocol = purple_connection_get_protocol(conn);

	return (protocol && PURPLE_PROTOCOL_IMPLEMENTS(protocol, ROOMLIST, get_list));
}

gboolean
pidgin_roomlist_is_showable()
{
	GList *c;
	PurpleConnection *gc;

	for (c = purple_connections_get_all(); c != NULL; c = c->next) {
		gc = c->data;

		if (account_filter_func(purple_connection_get_account(gc)))
			return TRUE;
	}

	return FALSE;
}

static void
pidgin_roomlist_dialog_class_init(PidginRoomlistDialogClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(
	        widget_class, "/im/pidgin/Pidgin/Roomlist/roomlist.ui");

	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     account_widget);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     add_button);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     close_button);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     join_button);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     list_button);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     progress);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     stop_button);
	gtk_widget_class_bind_template_child(widget_class, PidginRoomlistDialog,
	                                     sw);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        add_room_to_blist_cb);
	gtk_widget_class_bind_template_callback(widget_class, delete_win_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        dialog_select_account_cb);
	gtk_widget_class_bind_template_callback(widget_class, join_button_cb);
	gtk_widget_class_bind_template_callback(widget_class, list_button_cb);
	gtk_widget_class_bind_template_callback(widget_class, stop_button_cb);
}

static void
pidgin_roomlist_dialog_init(PidginRoomlistDialog *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));

	pidgin_account_chooser_set_filter_func(
	        PIDGIN_ACCOUNT_CHOOSER(self->account_widget),
	        account_filter_func);
}

static PidginRoomlistDialog *
pidgin_roomlist_dialog_new_with_account(PurpleAccount *account)
{
	PidginRoomlistDialog *dialog;

	dialog = g_object_new(PIDGIN_TYPE_ROOMLIST_DIALOG, NULL);
	dialog->account = account;

	if (!account) {
		/* This is normally NULL, and we normally don't care what the
		 * first selected item is */
		dialog->account = pidgin_account_chooser_get_selected(
		        dialog->account_widget);
	} else {
		pidgin_account_chooser_set_selected(dialog->account_widget,
		                                    account);
	}

	/* show the dialog window and return the dialog */
	gtk_widget_show(GTK_WIDGET(dialog));

	return dialog;
}

void pidgin_roomlist_dialog_show_with_account(PurpleAccount *account)
{
	PidginRoomlistDialog *dialog = pidgin_roomlist_dialog_new_with_account(account);

	if (!dialog)
		return;

	list_button_cb(GTK_BUTTON(dialog->list_button), dialog);
}

void pidgin_roomlist_dialog_show(void)
{
	pidgin_roomlist_dialog_new_with_account(NULL);
}

static void pidgin_roomlist_new(PurpleRoomlist *list)
{
	PidginRoomlist *rl = g_new0(PidginRoomlist, 1);

	purple_roomlist_set_ui_data(list, rl);

	rl->cats = g_hash_table_new_full(NULL, NULL, NULL, (GDestroyNotify)gtk_tree_row_reference_free);

	roomlists = g_list_append(roomlists, list);
}

static void int_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer,
                                   GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	gchar buf[16];
	int myint;

	gtk_tree_model_get(model, iter, GPOINTER_TO_INT(user_data), &myint, -1);

	if (myint)
		g_snprintf(buf, sizeof(buf), "%d", myint);
	else
		buf[0] = '\0';

	g_object_set(renderer, "text", buf, NULL);
}

/* this sorts backwards on purpose, so that clicking name sorts a-z, while clicking users sorts
   infinity-0. you can still click again to reverse it on any of them. */
static gint int_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
	int c, d;

	c = d = 0;

	gtk_tree_model_get(model, a, GPOINTER_TO_INT(user_data), &c, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(user_data), &d, -1);

	if (c == d)
		return 0;
	else if (c > d)
		return -1;
	else
		return 1;
}

static gboolean
_search_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
	gboolean result;
	gchar *name, *fold, *fkey;

	gtk_tree_model_get(model, iter, column, &name, -1);
	fold = g_utf8_casefold(name, -1);
	fkey = g_utf8_casefold(key, -1);

	result = (g_strstr_len(fold, strlen(fold), fkey) == NULL);

	g_free(fold);
	g_free(fkey);
	g_free(name);

	return result;
}

static void pidgin_roomlist_set_fields(PurpleRoomlist *list, GList *fields)
{
	PidginRoomlist *grl = purple_roomlist_get_ui_data(list);
	gint columns = NUM_OF_COLUMNS;
	int j;
	GtkTreeStore *model;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GList *l;
	GType *types;

	g_return_if_fail(grl != NULL);

	columns += g_list_length(fields);
	types = g_new(GType, columns);

	types[NAME_COLUMN] = G_TYPE_STRING;
	types[ROOM_COLUMN] = G_TYPE_POINTER;

	for (j = NUM_OF_COLUMNS, l = fields; l; l = l->next, j++) {
		PurpleRoomlistField *f = l->data;

		switch (purple_roomlist_field_get_field_type(f)) {
		case PURPLE_ROOMLIST_FIELD_BOOL:
			types[j] = G_TYPE_BOOLEAN;
			break;
		case PURPLE_ROOMLIST_FIELD_INT:
			types[j] = G_TYPE_INT;
			break;
		case PURPLE_ROOMLIST_FIELD_STRING:
			types[j] = G_TYPE_STRING;
			break;
		}
	}

	model = gtk_tree_store_newv(columns, types);
	g_free(types);

	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	g_signal_connect(G_OBJECT(selection), "changed",
					 G_CALLBACK(selection_changed_cb), grl);

	g_object_unref(model);

	grl->model = model;
	grl->tree = tree;
	gtk_widget_show(grl->tree);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
				"text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column),
	                                GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_column_set_sort_column_id(GTK_TREE_VIEW_COLUMN(column), NAME_COLUMN);
	gtk_tree_view_column_set_reorderable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	for (j = NUM_OF_COLUMNS, l = fields; l; l = l->next, j++) {
		PurpleRoomlistField *f = l->data;

		if (purple_roomlist_field_get_hidden(f))
			continue;

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				purple_roomlist_field_get_label(f), renderer,
				"text", j, NULL);
		gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column),
		                                GTK_TREE_VIEW_COLUMN_GROW_ONLY);
		gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
		gtk_tree_view_column_set_sort_column_id(GTK_TREE_VIEW_COLUMN(column), j);
		gtk_tree_view_column_set_reorderable(GTK_TREE_VIEW_COLUMN(column), TRUE);
		if (purple_roomlist_field_get_field_type(f) == PURPLE_ROOMLIST_FIELD_INT) {
			gtk_tree_view_column_set_cell_data_func(column, renderer, int_cell_data_func,
			                                        GINT_TO_POINTER(j), NULL);
			gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), j, int_sort_func,
			                                GINT_TO_POINTER(j), NULL);
		}
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	}

	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(room_click_cb), list);
	g_signal_connect(G_OBJECT(tree), "row-expanded", G_CALLBACK(row_expanded_cb), list);
	g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(row_activated_cb), list);
#if 0 /* uncomment this when the tooltips are slightly less annoying and more well behaved */
	g_signal_connect(G_OBJECT(tree), "motion-notify-event", G_CALLBACK(row_motion_cb), list);
	g_signal_connect(G_OBJECT(tree), "leave-notify-event", G_CALLBACK(row_leave_cb), list);
#endif
	pidgin_tooltip_setup_for_treeview(tree, list,
		pidgin_roomlist_create_tooltip,
		pidgin_roomlist_paint_tooltip);

	/* Enable CTRL+F searching */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), NAME_COLUMN);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(tree), _search_func, NULL, NULL);

}

static gboolean pidgin_progress_bar_pulse(gpointer data)
{
	PurpleRoomlist *list = data;
	PidginRoomlist *rl = purple_roomlist_get_ui_data(list);

	if (!rl || !rl->dialog || !rl->dialog->pg_needs_pulse) {
		if (rl && rl->dialog)
			rl->dialog->pg_update_to = 0;
		g_object_unref(list);
		return FALSE;
	}

	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(rl->dialog->progress));
	rl->dialog->pg_needs_pulse = FALSE;
	return TRUE;
}

static void pidgin_roomlist_add_room(PurpleRoomlist *list, PurpleRoomlistRoom *room)
{
	PidginRoomlist *rl = purple_roomlist_get_ui_data(list);
	GtkTreeRowReference *rr, *parentrr = NULL;
	GtkTreePath *path;
	GtkTreeIter iter, parent, child;
	GList *l, *k;
	int j;
	gboolean append = TRUE;

	rl->total_rooms++;
	if (purple_roomlist_room_get_room_type(room) == PURPLE_ROOMLIST_ROOMTYPE_ROOM)
		rl->num_rooms++;

	if (rl->dialog) {
		if (rl->dialog->pg_update_to == 0) {
			g_object_ref(list);
			rl->dialog->pg_update_to = g_timeout_add(100, pidgin_progress_bar_pulse, list);
			gtk_progress_bar_pulse(GTK_PROGRESS_BAR(rl->dialog->progress));
		} else
			rl->dialog->pg_needs_pulse = TRUE;
	}

	if (purple_roomlist_room_get_parent(room)) {
		parentrr = g_hash_table_lookup(rl->cats, purple_roomlist_room_get_parent(room));
		path = gtk_tree_row_reference_get_path(parentrr);
		if (path) {
			PurpleRoomlistRoom *tmproom = NULL;

			gtk_tree_model_get_iter(GTK_TREE_MODEL(rl->model), &parent, path);
			gtk_tree_path_free(path);

			if (gtk_tree_model_iter_children(GTK_TREE_MODEL(rl->model), &child, &parent)) {
				gtk_tree_model_get(GTK_TREE_MODEL(rl->model), &child, ROOM_COLUMN, &tmproom, -1);
				if (!tmproom)
					append = FALSE;
			}
		}
	}

	if (append)
		gtk_tree_store_append(rl->model, &iter, (parentrr ? &parent : NULL));
	else
		iter = child;

	if (purple_roomlist_room_get_room_type(room) & PURPLE_ROOMLIST_ROOMTYPE_CATEGORY)
		gtk_tree_store_append(rl->model, &child, &iter);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(rl->model), &iter);

	if (purple_roomlist_room_get_room_type(room) & PURPLE_ROOMLIST_ROOMTYPE_CATEGORY) {
		rr = gtk_tree_row_reference_new(GTK_TREE_MODEL(rl->model), path);
		g_hash_table_insert(rl->cats, room, rr);
	}

	gtk_tree_path_free(path);

	gtk_tree_store_set(rl->model, &iter, NAME_COLUMN, purple_roomlist_room_get_name(room), -1);
	gtk_tree_store_set(rl->model, &iter, ROOM_COLUMN, room, -1);

	for (j = NUM_OF_COLUMNS,
				l = purple_roomlist_room_get_fields(room),
				k = purple_roomlist_get_fields(list);
			l && k; j++, l = l->next, k = k->next)
	{
		PurpleRoomlistField *f = k->data;
		if (purple_roomlist_field_get_hidden(f))
			continue;
		gtk_tree_store_set(rl->model, &iter, j, l->data, -1);
	}
}

static void pidgin_roomlist_in_progress(PurpleRoomlist *list, gboolean in_progress)
{
	PidginRoomlist *rl = purple_roomlist_get_ui_data(list);

	if (!rl || !rl->dialog)
		return;

	if (in_progress) {
		if (rl->dialog->account_widget)
			gtk_widget_set_sensitive(rl->dialog->account_widget, FALSE);
		gtk_widget_set_sensitive(rl->dialog->stop_button, TRUE);
		gtk_widget_set_sensitive(rl->dialog->list_button, FALSE);
	} else {
		rl->dialog->pg_needs_pulse = FALSE;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(rl->dialog->progress), 0.0);
		if (rl->dialog->account_widget)
			gtk_widget_set_sensitive(rl->dialog->account_widget, TRUE);
		gtk_widget_set_sensitive(rl->dialog->stop_button, FALSE);
		gtk_widget_set_sensitive(rl->dialog->list_button, TRUE);
	}
}

static void pidgin_roomlist_destroy(PurpleRoomlist *list)
{
	PidginRoomlist *rl = purple_roomlist_get_ui_data(list);

	roomlists = g_list_remove(roomlists, list);

	g_return_if_fail(rl != NULL);

	g_hash_table_destroy(rl->cats);
	g_free(rl);
	purple_roomlist_set_ui_data(list, NULL);
}

static PurpleRoomlistUiOps ops = {
	pidgin_roomlist_dialog_show_with_account,
	pidgin_roomlist_new,
	pidgin_roomlist_set_fields,
	pidgin_roomlist_add_room,
	pidgin_roomlist_in_progress,
	pidgin_roomlist_destroy,
	NULL,
	NULL,
	NULL,
	NULL
};


void pidgin_roomlist_init(void)
{
	purple_roomlist_set_ui_ops(&ops);
}
