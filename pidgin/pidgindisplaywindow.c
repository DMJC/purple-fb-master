/*
 * Pidgin - Internet Messenger
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <glib/gi18n-lib.h>

#include <adwaita.h>

#include <gplugin.h>
#include <gplugin-gtk.h>

#include "pidgindisplaywindow.h"

#include "gtkdialogs.h"
#include "gtkutils.h"
#include "pidginconversation.h"
#include "pidgindisplayitem.h"
#include "pidgininvitedialog.h"

enum {
	SIG_CONVERSATION_SWITCHED,
	N_SIGNALS,
};
static guint signals[N_SIGNALS] = {0, };

struct _PidginDisplayWindow {
	GtkApplicationWindow parent;

	GtkWidget *view;
	GtkWidget *bin;

	GtkWidget *plugin_list;

	GListModel *base_model;
	GListModel *selection_model;

	GListStore *conversation_model;
};

G_DEFINE_FINAL_TYPE(PidginDisplayWindow, pidgin_display_window,
                    GTK_TYPE_APPLICATION_WINDOW)

static GtkWidget *default_window = NULL;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
pidgin_display_window_actions_set_enabled(GActionMap *map,
                                          const gchar **actions,
                                          gboolean enabled)
{
	for(int i = 0; actions[i] != NULL; i++) {
		GAction *action = NULL;
		const gchar *name = actions[i];

		action = g_action_map_lookup_action(map, name);
		if(action != NULL) {
			g_simple_action_set_enabled(G_SIMPLE_ACTION(action), enabled);
		}
	}
}

static GListModel *
pidgin_display_window_create_model(GObject *item,
                                   G_GNUC_UNUSED gpointer data)
{
	GListModel *model = NULL;

	model = pidgin_display_item_get_children(PIDGIN_DISPLAY_ITEM(item));
	if(model != NULL) {
		return g_object_ref(model);
	}

	return NULL;
}

static gboolean
pidgin_display_window_find_conversation(gconstpointer a, gconstpointer b) {
	PidginDisplayItem *item_a = PIDGIN_DISPLAY_ITEM((gpointer)a);
	PidginDisplayItem *item_b = PIDGIN_DISPLAY_ITEM((gpointer)b);
	PurpleConversation *conversation_a = NULL;
	PurpleConversation *conversation_b = NULL;

	conversation_a = g_object_get_data(G_OBJECT(item_a), "conversation");
	conversation_b = g_object_get_data(G_OBJECT(item_b), "conversation");

	return (conversation_a == conversation_b);
}

/******************************************************************************
 * Actions
 *****************************************************************************/
static void
pidgin_display_window_close_conversation(G_GNUC_UNUSED GSimpleAction *simple,
                                         G_GNUC_UNUSED GVariant *parameter,
                                         gpointer data)
{
	PidginDisplayWindow *window = data;
	PurpleConversation *selected = NULL;

	selected = pidgin_display_window_get_selected(window);
	if(PURPLE_IS_CONVERSATION(selected)) {
		GtkWidget *conversation = NULL;

		conversation = pidgin_conversation_from_purple_conversation(selected);
		if(PIDGIN_IS_CONVERSATION(conversation)) {
			pidgin_conversation_close(PIDGIN_CONVERSATION(conversation));
			g_clear_object(&conversation);
		}

		pidgin_display_window_remove(window, selected);
	}
}

static void
pidgin_display_window_get_info(G_GNUC_UNUSED GSimpleAction *simple,
                               G_GNUC_UNUSED GVariant *parameter,
                               gpointer data)
{
	PidginDisplayWindow *window = data;
	PurpleConversation *selected = NULL;

	selected = pidgin_display_window_get_selected(window);
	if(PURPLE_IS_CONVERSATION(selected)) {
		if(PURPLE_IS_IM_CONVERSATION(selected)) {
			PurpleConnection *connection = NULL;

			connection = purple_conversation_get_connection(selected);
			pidgin_retrieve_user_info(connection,
			                          purple_conversation_get_name(selected));
		}
	}
}

static void
pidgin_display_window_send_file(G_GNUC_UNUSED GSimpleAction *simple,
                                G_GNUC_UNUSED GVariant *parameter,
                                gpointer data)
{
	PidginDisplayWindow *window = data;
	PurpleConversation *selected = NULL;

	selected = pidgin_display_window_get_selected(window);
	if(PURPLE_IS_IM_CONVERSATION(selected)) {
		PurpleConnection *connection = NULL;

		connection = purple_conversation_get_connection(selected);
		purple_serv_send_file(connection,
		                      purple_conversation_get_name(selected),
		                      NULL);
	}
}

static GActionEntry win_entries[] = {
	{
		.name = "close",
		.activate = pidgin_display_window_close_conversation
	}, {
		.name = "get-info",
		.activate = pidgin_display_window_get_info
	}, {
		.name = "send-file",
		.activate = pidgin_display_window_send_file
	}
};

/*<private>
 * pidgin_display_window_conversation_actions:
 *
 * A list of action names that are only valid if a conversation is selected.
 */
static const gchar *pidgin_display_window_conversation_actions[] = {
	"alias",
	"close",
	"get-info",
	NULL
};

static const gchar *pidgin_display_window_im_conversation_actions[] = {
	"send-file",
	NULL
};

static const gchar *pidgin_display_window_chat_conversation_actions[] = {
	"invite",
	NULL
};

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
pidgin_display_window_key_pressed_cb(G_GNUC_UNUSED GtkEventControllerKey *controller,
                                     guint keyval,
                                     G_GNUC_UNUSED guint keycode,
                                     GdkModifierType state,
                                     gpointer data)
{
	PidginDisplayWindow *window = data;

	if (state & GDK_CONTROL_MASK) {
		switch (keyval) {
			case GDK_KEY_Page_Down:
			case GDK_KEY_KP_Page_Down:
			case ']':
				pidgin_display_window_select_next(window);
				return TRUE;
				break;
			case GDK_KEY_Home:
				pidgin_display_window_select_first(window);
				return TRUE;
				break;
			case GDK_KEY_End:
				pidgin_display_window_select_last(window);
				return TRUE;
				break;
			case GDK_KEY_Page_Up:
			case GDK_KEY_KP_Page_Up:
			case '[':
				pidgin_display_window_select_previous(window);
				return TRUE;
				break;
		}
	} else if (state & GDK_ALT_MASK) {
		if ('1' <= keyval && keyval <= '9') {
			guint switchto = keyval - '1';
			pidgin_display_window_select_nth(window, switchto);

			return TRUE;
		}
	}

	return FALSE;
}

static void
pidgin_display_window_selected_item_changed_cb(GObject *self,
                                               G_GNUC_UNUSED GParamSpec *pspec,
                                               gpointer data)
{
	PidginDisplayItem *item = NULL;
	PidginDisplayWindow *window = data;
	PurpleConversation *conversation = NULL;
	GtkSingleSelection *selection = GTK_SINGLE_SELECTION(self);
	GtkTreeListRow *row = NULL;
	GtkWidget *widget = NULL;
	gboolean is_conversation = FALSE;
	gboolean is_im_conversation = FALSE;
	gboolean is_chat_conversation = FALSE;

	row = gtk_single_selection_get_selected_item(selection);

	item = gtk_tree_list_row_get_item(row);

	/* Toggle whether actions should be enabled or disabled. */
	conversation = g_object_get_data(G_OBJECT(item), "conversation");
	if(PURPLE_IS_CONVERSATION(conversation)) {
		is_conversation = PURPLE_IS_CONVERSATION(conversation);
		is_im_conversation = PURPLE_IS_IM_CONVERSATION(conversation);
		is_chat_conversation = PURPLE_IS_CHAT_CONVERSATION(conversation);
	}

	pidgin_display_window_actions_set_enabled(G_ACTION_MAP(window),
	                                          pidgin_display_window_conversation_actions,
	                                          is_conversation);
	pidgin_display_window_actions_set_enabled(G_ACTION_MAP(window),
	                                          pidgin_display_window_im_conversation_actions,
	                                          is_im_conversation);
	pidgin_display_window_actions_set_enabled(G_ACTION_MAP(window),
	                                          pidgin_display_window_chat_conversation_actions,
	                                          is_chat_conversation);

	widget = pidgin_display_item_get_widget(item);
	if(GTK_IS_WIDGET(widget)) {
		adw_bin_set_child(ADW_BIN(window->bin), widget);
	}
}

static void
pidgin_display_window_conversation_registered_cb(G_GNUC_UNUSED PurpleConversationManager *manager,
                                                 PurpleConversation *conversation,
                                                 gpointer data)
{
	PidginDisplayWindow *window = data;

	pidgin_display_window_add(window, conversation);
}

static void
pidgin_display_window_conversation_unregistered_cb(G_GNUC_UNUSED PurpleConversationManager *manager,
                                                   PurpleConversation *conversation,
                                                   gpointer data)
{
	PidginDisplayWindow *window = data;

	pidgin_display_window_remove(window, conversation);
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_display_window_finalize(GObject *obj) {
	PidginDisplayWindow *window = PIDGIN_DISPLAY_WINDOW(obj);

	g_clear_object(&window->conversation_model);
	g_clear_object(&window->selection_model);

	G_OBJECT_CLASS(pidgin_display_window_parent_class)->finalize(obj);
}

static void
pidgin_display_window_init(PidginDisplayWindow *window) {
	GtkEventController *key = NULL;
	GtkTreeListModel *tree_model = NULL;
	GPluginManager *plugin_manager = NULL;
	gpointer settings_backend = NULL;

	gtk_widget_init_template(GTK_WIDGET(window));

	/* Add a reference to the selection model as we use it internally and with
	 * out it we get some weird call backs being called when it's nulled out
	 * during destruction.
	 */
	g_object_ref(window->selection_model);

	/* Setup the tree list model. */
	tree_model = gtk_tree_list_model_new(window->base_model, FALSE, TRUE,
	                                     (GtkTreeListModelCreateModelFunc)pidgin_display_window_create_model,
	                                     window, NULL);

	/* Set the model of the selection to the tree model. */
	gtk_single_selection_set_model(GTK_SINGLE_SELECTION(window->selection_model),
	                               G_LIST_MODEL(tree_model));
	g_clear_object(&tree_model);

	/* Set the application and add all of our actions. */
	gtk_window_set_application(GTK_WINDOW(window),
	                           GTK_APPLICATION(g_application_get_default()));

	g_action_map_add_action_entries(G_ACTION_MAP(window), win_entries,
	                                G_N_ELEMENTS(win_entries), window);

	/* Add a key controller. */
	key = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(key, GTK_PHASE_CAPTURE);
	g_signal_connect(G_OBJECT(key), "key-pressed",
	                 G_CALLBACK(pidgin_display_window_key_pressed_cb),
	                 window);
	gtk_widget_add_controller(GTK_WIDGET(window), key);

	/* Set up the plugin list. */
	plugin_manager = gplugin_manager_get_default();
	gplugin_gtk_view_set_manager(GPLUGIN_GTK_VIEW(window->plugin_list),
	                             plugin_manager);

	settings_backend = purple_core_get_settings_backend();
	gplugin_gtk_view_set_settings_backend(GPLUGIN_GTK_VIEW(window->plugin_list),
	                                      settings_backend);
}

static void
pidgin_display_window_class_init(PidginDisplayWindowClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	obj_class->finalize = pidgin_display_window_finalize;

	/**
	 * PidginDisplayWindow::conversation-switched:
	 * @window: The conversation window.
	 * @new_conv: The now active conversation.
	 *
	 * Emitted when a window switched from one conversation to another.
	 */
	signals[SIG_CONVERSATION_SWITCHED] = g_signal_new_class_handler(
		"conversation-switched",
		G_OBJECT_CLASS_TYPE(obj_class),
		G_SIGNAL_RUN_LAST,
		NULL,
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		PURPLE_TYPE_CONVERSATION
	);

	gtk_widget_class_set_template_from_resource(
	    widget_class,
	    "/im/pidgin/Pidgin3/Display/window.ui"
	);

	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     view);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     bin);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     plugin_list);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     base_model);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     selection_model);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     conversation_model);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_display_window_key_pressed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_display_window_selected_item_changed_cb);
}

/******************************************************************************
 * API
 *****************************************************************************/
GtkWidget *
pidgin_display_window_get_default(void) {
	if(!GTK_IS_WIDGET(default_window)) {
		PurpleConversationManager *manager = NULL;

		default_window = pidgin_display_window_new();
		g_object_add_weak_pointer(G_OBJECT(default_window),
		                          (gpointer)&default_window);

		manager = purple_conversation_manager_get_default();
		g_signal_connect_object(manager, "registered",
		                        G_CALLBACK(pidgin_display_window_conversation_registered_cb),
		                        default_window, 0);
		g_signal_connect_object(manager, "unregistered",
		                        G_CALLBACK(pidgin_display_window_conversation_unregistered_cb),
		                        default_window, 0);
	}

	return default_window;
}

GtkWidget *
pidgin_display_window_new(void) {
	return g_object_new(
		PIDGIN_TYPE_DISPLAY_WINDOW,
		"show-menubar", TRUE,
		NULL);
}

void
pidgin_display_window_add(PidginDisplayWindow *window,
                          PurpleConversation *purple_conversation)
{
	GtkWidget *pidgin_conversation = NULL;
	const char *conversation_id = NULL;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));
	g_return_if_fail(PURPLE_IS_CONVERSATION(purple_conversation));

	conversation_id = purple_conversation_get_id(purple_conversation);
	g_return_if_fail(conversation_id != NULL);

	pidgin_conversation = pidgin_conversation_from_purple_conversation(purple_conversation);

	if(!PIDGIN_IS_CONVERSATION(pidgin_conversation)) {
		pidgin_conversation = pidgin_conversation_new(purple_conversation);
	}

	if(PIDGIN_IS_CONVERSATION(pidgin_conversation)) {
		PidginDisplayItem *item = NULL;
		PurpleAccount *account = purple_conversation_get_account(purple_conversation);
		PurpleContactInfo *info = PURPLE_CONTACT_INFO(account);
		const char *account_id = NULL;
		char *id = NULL;

		GtkWidget *parent = gtk_widget_get_parent(pidgin_conversation);

		if(GTK_IS_WIDGET(parent)) {
			g_object_ref(pidgin_conversation);
			gtk_widget_unparent(pidgin_conversation);
		}

		account_id = purple_contact_info_get_id(info);
		id = g_strdup_printf("%s-%s", account_id, conversation_id);
		item = pidgin_display_item_new(pidgin_conversation, id);
		g_free(id);
		g_object_set_data(G_OBJECT(item), "conversation", purple_conversation);

		g_object_bind_property(purple_conversation, "title",
		                       item, "title",
		                       G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

		g_list_store_append(window->conversation_model, item);
		g_clear_object(&item);

		if(GTK_IS_WIDGET(parent)) {
			g_object_unref(pidgin_conversation);
		}
	}
}

void
pidgin_display_window_remove(PidginDisplayWindow *window,
                             PurpleConversation *conversation)
{
	PidginDisplayItem *item = NULL;
	guint position = 0;
	gboolean found = FALSE;
	gchar *id = NULL;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));

	/* Create a wrapper item for our find function. */
	id = g_uuid_string_random();
	item = g_object_new(PIDGIN_TYPE_DISPLAY_ITEM, "id", id, NULL);
	g_free(id);
	g_object_set_data(G_OBJECT(item), "conversation", conversation);

	found = g_list_store_find_with_equal_func(window->conversation_model,
	                                          item,
	                                          pidgin_display_window_find_conversation,
	                                          &position);

	g_clear_object(&item);

	if(found) {
		g_list_store_remove(window->conversation_model, position);
	}
}

guint
pidgin_display_window_get_count(G_GNUC_UNUSED PidginDisplayWindow *window) {
	/* TODO: This is only used by the gestures plugin and that will probably
	 * need some rewriting and different api for a mixed content window list
	 * this is now.
	 */

	return 0;
}

PurpleConversation *
pidgin_display_window_get_selected(PidginDisplayWindow *window) {
	GtkSingleSelection *selection = NULL;
	GtkTreeListRow *tree_row = NULL;
	GObject *selected = NULL;

	g_return_val_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window), NULL);

	selection = GTK_SINGLE_SELECTION(window->selection_model);
	tree_row = gtk_single_selection_get_selected_item(selection);
	selected = gtk_tree_list_row_get_item(tree_row);

	if(PIDGIN_IS_DISPLAY_ITEM(selected)) {
		return g_object_get_data(selected, "conversation");
	}

	return NULL;
}

void
pidgin_display_window_select(PidginDisplayWindow *window,
                             PurpleConversation *conversation)
{
	/* TODO: This is used by the unity and gestures plugins, but I'm really not
	 * sure how to make this work yet without some hard-coding or something, so
	 * I'm opting to stub it out for now.
	 */

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));
	g_return_if_fail(PURPLE_IS_CONVERSATION(conversation));
}

void
pidgin_display_window_select_previous(PidginDisplayWindow *window) {
	GtkSingleSelection *selection = NULL;
	guint position = 0;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));

	selection = GTK_SINGLE_SELECTION(window->selection_model);
	position = gtk_single_selection_get_selected(selection);
	if(position == 0) {
		position = g_list_model_get_n_items(G_LIST_MODEL(selection)) - 1;
	} else {
		position = position - 1;
	}

	gtk_single_selection_set_selected(selection, position);
}

void
pidgin_display_window_select_next(PidginDisplayWindow *window) {
	GtkSingleSelection *selection = NULL;
	guint position = 0;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));

	selection = GTK_SINGLE_SELECTION(window->selection_model);
	position = gtk_single_selection_get_selected(selection);
	if(position + 1 >= g_list_model_get_n_items(G_LIST_MODEL(selection))) {
		position = 0;
	} else {
		position = position + 1;
	}

	gtk_single_selection_set_selected(selection, position);
}

void
pidgin_display_window_select_first(PidginDisplayWindow *window) {
	GtkSingleSelection *selection = NULL;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));

	selection = GTK_SINGLE_SELECTION(window->selection_model);

	/* The selection has autoselect set to true, which won't do anything if
	 * this is an invalid value.
	 */
	gtk_single_selection_set_selected(selection, 0);
}

void
pidgin_display_window_select_last(PidginDisplayWindow *window) {
	GtkSingleSelection *selection = NULL;
	guint n_items = 0;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));

	selection = GTK_SINGLE_SELECTION(window->selection_model);
	n_items = g_list_model_get_n_items(G_LIST_MODEL(selection));

	/* The selection has autoselect set to true, which won't do anything if
	 * this is an invalid value.
	 */
	gtk_single_selection_set_selected(selection, n_items - 1);
}

void
pidgin_display_window_select_nth(PidginDisplayWindow *window,
                                 guint nth)
{
	GtkSingleSelection *selection = NULL;
	guint n_items = 0;

	g_return_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window));

	selection = GTK_SINGLE_SELECTION(window->selection_model);

	/* The selection has autoselect set to true, but this isn't bound checking
	 * or something on the children models, so we verify before setting.
	 */
	n_items = g_list_model_get_n_items(G_LIST_MODEL(selection));
	if(nth < n_items) {
		gtk_single_selection_set_selected(selection, nth);
	}
}

gboolean
pidgin_display_window_conversation_is_selected(PidginDisplayWindow *window,
                                               PurpleConversation *conversation)
{
	GtkSingleSelection *selection = NULL;
	GObject *selected = NULL;

	g_return_val_if_fail(PIDGIN_IS_DISPLAY_WINDOW(window), FALSE);
	g_return_val_if_fail(PURPLE_IS_CONVERSATION(conversation), FALSE);

	selection = GTK_SINGLE_SELECTION(window->selection_model);
	selected = gtk_single_selection_get_selected_item(selection);

	if(PIDGIN_IS_DISPLAY_ITEM(selected)) {
		PurpleConversation *selected_conversation = NULL;

		selected_conversation = g_object_get_data(G_OBJECT(selected),
		                                          "conversation");
		if(selected_conversation != NULL) {
			return (selected_conversation == conversation);
		}
	}

	return FALSE;
}
