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

#include "pidginconversation.h"
#include "pidgindisplayitem.h"

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

	GListStore *base_model;
	GListModel *selection_model;

	GMenuModel *conversation_menu;
	PidginDisplayItem *conversations_item;

	GListStore *conversation_model;
};

G_DEFINE_FINAL_TYPE(PidginDisplayWindow, pidgin_display_window,
                    GTK_TYPE_APPLICATION_WINDOW)

static GtkWidget *default_window = NULL;

/******************************************************************************
 * Helpers
 *****************************************************************************/
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

static gboolean
pidgin_display_window_find_conversation_by_id(gconstpointer a,
                                              G_GNUC_UNUSED gconstpointer b,
                                              gpointer user_data)
{
	PidginDisplayItem *item_a = PIDGIN_DISPLAY_ITEM((gpointer)a);
	const char *a_id = pidgin_display_item_get_id(item_a);
	const char *id = user_data;

	return purple_strequal(a_id, id);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
pidgin_display_window_leave_conversation_cb(GObject *source,
                                            GAsyncResult *result,
                                            gpointer data)
{
	PurpleConversation *conversation = data;
	GError *error = NULL;
	gboolean left = FALSE;

	left = purple_protocol_conversation_leave_conversation_finish(PURPLE_PROTOCOL_CONVERSATION(source),
	                                                              result, &error);
	if(error != NULL) {
		g_warning("failed to leave conversation: '%s'", error->message);
		g_clear_error(&error);
	} else if(left) {
		PurpleConversationManager *manager = NULL;

		manager = purple_conversation_manager_get_default();

		purple_conversation_manager_unregister(manager, conversation);
	} else {
		g_warning("failed to leave conversation for unknown reasons");
	}

	g_clear_object(&conversation);
}

/******************************************************************************
 * Conversation Actions
 *****************************************************************************/
static void
pidgin_display_window_leave_conversation(G_GNUC_UNUSED GSimpleAction *simple,
                                         GVariant *parameter,
                                         G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleAccountManager *account_manager = NULL;
	PurpleConversation *conversation = NULL;
	PurpleConversationManager *conversation_manager = NULL;
	PurpleProtocol *protocol = NULL;
	GStrv parts = NULL;
	const char *id = NULL;

	if(!g_variant_is_of_type(parameter, G_VARIANT_TYPE_STRING)) {
		g_warning("parameter is of type %s, expected %s",
		          g_variant_get_type_string(parameter),
		          (char *)G_VARIANT_TYPE_STRING);

		return;
	}

	id = g_variant_get_string(parameter, NULL);
	parts = g_strsplit(id, "-", 2);
	if(g_strv_length(parts) != 2) {
		g_warning("unexpected id format '%s'", id);

		g_strfreev(parts);

		return;
	}

	account_manager = purple_account_manager_get_default();
	account = purple_account_manager_find_by_id(account_manager, parts[0]);
	if(!PURPLE_IS_ACCOUNT(account)) {
		g_warning("failed to find account with id '%s'", parts[0]);

		g_strfreev(parts);

		return;
	}

	conversation_manager = purple_conversation_manager_get_default();
	conversation = purple_conversation_manager_find_with_id(conversation_manager,
	                                                        account, parts[1]);
	if(!PURPLE_IS_CONVERSATION(conversation)) {
		g_warning("failed to find conversation with id '%s'", parts[1]);

		g_strfreev(parts);

		return;
	}

	/* We're done with parts now so free it. */
	g_strfreev(parts);

	protocol = purple_account_get_protocol(account);
	if(PURPLE_IS_PROTOCOL_CONVERSATION(protocol)) {
		PurpleProtocolConversation *protocol_conversation = NULL;

		protocol_conversation = PURPLE_PROTOCOL_CONVERSATION(protocol);

		purple_protocol_conversation_leave_conversation_async(protocol_conversation,
		                                                      conversation,
		                                                      NULL,
		                                                      pidgin_display_window_leave_conversation_cb,
		                                                      g_object_ref(conversation));
	}
}

static GActionEntry conversation_entries[] = {
	{
		.name = "leave",
		.activate = pidgin_display_window_leave_conversation,
		.parameter_type  = "s",
	}
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
	GtkSingleSelection *selection = GTK_SINGLE_SELECTION(self);
	GtkTreeListRow *row = NULL;
	GtkWidget *widget = NULL;

	row = gtk_single_selection_get_selected_item(selection);

	item = gtk_tree_list_row_get_item(row);

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

static void
pidgin_display_window_conversation_present_cb(PurpleConversation *conversation,
                                              gpointer data)
{
	pidgin_display_window_select(data, conversation);
}

static gboolean
pidgin_display_window_show_item_menu_cb(G_GNUC_UNUSED GObject *source,
                                        PidginDisplayItem *item,
                                        G_GNUC_UNUSED gpointer data)
{
	GMenuModel *model = NULL;

	if(!PIDGIN_IS_DISPLAY_ITEM(item)) {
		return FALSE;
	}

	model = pidgin_display_item_get_menu(item);

	return G_IS_MENU_MODEL(model);
}

static char *
pidgin_display_window_show_item_actions_cb(G_GNUC_UNUSED GObject *source,
                                           gboolean item_selected,
                                           gboolean item_contains_pointer,
                                           gboolean menu_button_active,
                                           G_GNUC_UNUSED gpointer data)
{
	if(item_selected || item_contains_pointer || menu_button_active) {
		return g_strdup("actions");
	}

	return g_strdup("notifications");
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
	GSimpleActionGroup *conversation_actions = NULL;
	GPluginManager *plugin_manager = NULL;
	gpointer settings_backend = NULL;

	gtk_widget_init_template(GTK_WIDGET(window));

	/* Add a reference to the selection model as we use it internally and with
	 * out it we get some weird call backs being called when it's nulled out
	 * during destruction.
	 */
	g_object_ref(window->selection_model);

	/* Setup the tree list model. */
	tree_model = gtk_tree_list_model_new(G_LIST_MODEL(window->base_model),
	                                     FALSE, TRUE,
	                                     (GtkTreeListModelCreateModelFunc)pidgin_display_window_create_model,
	                                     window, NULL);

	/* Set the model of the selection to the tree model. */
	gtk_single_selection_set_model(GTK_SINGLE_SELECTION(window->selection_model),
	                               G_LIST_MODEL(tree_model));
	g_clear_object(&tree_model);

	/* Set the application and add all of our actions. */
	gtk_window_set_application(GTK_WINDOW(window),
	                           GTK_APPLICATION(g_application_get_default()));

	conversation_actions = g_simple_action_group_new();
	g_action_map_add_action_entries(G_ACTION_MAP(conversation_actions),
	                                conversation_entries,
	                                G_N_ELEMENTS(conversation_entries),
	                                window);
	gtk_widget_insert_action_group(GTK_WIDGET(window), "conversation",
	                               G_ACTION_GROUP(conversation_actions));
	g_clear_object(&conversation_actions);

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
	    "/im/pidgin/Pidgin3/display-window.ui"
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
	                                     conversation_menu);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     conversations_item);
	gtk_widget_class_bind_template_child(widget_class, PidginDisplayWindow,
	                                     conversation_model);

	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_display_window_key_pressed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_display_window_selected_item_changed_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_display_window_show_item_menu_cb);
	gtk_widget_class_bind_template_callback(widget_class,
	                                        pidgin_display_window_show_item_actions_cb);
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
		const char *account_id = NULL;
		char *id = NULL;
		gboolean item_exists = FALSE;

		GtkWidget *parent = gtk_widget_get_parent(pidgin_conversation);

		if(GTK_IS_WIDGET(parent)) {
			g_object_ref(pidgin_conversation);
			gtk_widget_unparent(pidgin_conversation);
		}

		account_id = purple_account_get_id(account);
		id = g_strdup_printf("%s-%s", account_id, conversation_id);
		item_exists =
			g_list_store_find_with_equal_func_full(window->conversation_model,
			                                       NULL,
			                                       pidgin_display_window_find_conversation_by_id,
			                                       id,
			                                       NULL);
		if (!item_exists) {
			PurpleProtocol *protocol = NULL;
			GMenu *menu = NULL;
			const char *icon_name = NULL;

			menu = purple_menu_copy(window->conversation_menu);
			purple_menu_populate_dynamic_targets(menu, "id", id, NULL);

			item = pidgin_display_item_new(pidgin_conversation, id);
			pidgin_display_item_set_menu(item, G_MENU_MODEL(menu));
			g_clear_object(&menu);
			g_object_set_data(G_OBJECT(item), "conversation", purple_conversation);

			protocol = purple_account_get_protocol(account);
			icon_name = purple_protocol_get_icon_name(protocol);
			if(!purple_strempty(icon_name)) {
				pidgin_display_item_set_icon_name(item, icon_name);
			}

			g_object_bind_property(purple_conversation, "title-for-display",
			                       item, "title",
			                       G_BINDING_SYNC_CREATE);

			g_signal_connect_object(purple_conversation, "present",
			                        G_CALLBACK(pidgin_display_window_conversation_present_cb),
			                        window, G_CONNECT_DEFAULT);

			g_list_store_append(window->conversation_model, item);
			g_clear_object(&item);
		}
		g_free(id);
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
		g_signal_handlers_disconnect_by_func(conversation,
		                                     G_CALLBACK(pidgin_display_window_conversation_present_cb),
		                                     window);

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
		/* Figure out where the conversations_item is because we need to add to
		 * that position.
		 */
		guint conversations_position = 0;
		guint real_position = 0;

		g_list_store_find(window->base_model, window->conversations_item,
		                  &conversations_position);

		/* Since this is a nested model, we need to increment by one to get to
		 * the correct item.
		 */
		real_position = conversations_position + position + 1;

		gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(window->selection_model),
		                                  real_position);
	}
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
