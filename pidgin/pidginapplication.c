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

#include <purpleconfig.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gplugin.h>
#include <purple.h>

#include <adwaita.h>

#include "pidginapplication.h"

#include "gtkroomlist.h"
#include "pidginabout.h"
#include "pidginaccounteditor.h"
#include "pidginaccountmanager.h"
#include "pidginaccountsdisabledmenu.h"
#include "pidginaccountsenabledmenu.h"
#include "pidginchanneljoindialog.h"
#include "pidgincore.h"
#include "pidgindebug.h"
#include "pidgindisplaywindow.h"
#include "pidginpluginsdialog.h"
#include "pidginpluginsmenu.h"
#include "pidginprefs.h"
#include "pidginui.h"

struct _PidginApplication {
	AdwApplication parent;

	GHashTable *action_groups;
};

/******************************************************************************
 * Globals
 *****************************************************************************/
static gchar *opt_config_dir_arg = NULL;
static gboolean opt_debug = FALSE;
static gboolean opt_nologin = FALSE;

static GOptionEntry option_entries[] = {
	{
		"config", 'c', 0, G_OPTION_ARG_FILENAME, &opt_config_dir_arg,
		N_("use DIR for config files"), N_("DIR")
	}, {
		"debug", 'd', 0, G_OPTION_ARG_NONE, &opt_debug,
		N_("print debugging messages to stdout"), NULL
	}, {
		"nologin", 'n', 0, G_OPTION_ARG_NONE, &opt_nologin,
		N_("don't automatically login"), NULL
	},
	{
		"version", 'v', 0, G_OPTION_ARG_NONE, NULL,
		N_("display the current version and exit"), NULL
	}, {
		NULL
	}
};

G_DEFINE_FINAL_TYPE(PidginApplication, pidgin_application,
                    ADW_TYPE_APPLICATION)

/******************************************************************************
 * Helpers
 *****************************************************************************/

/*
 * pidgin_application_get_account_manager:
 *
 * The Pidgin account manager can get opened from multiple actions, so this
 * helper manages the singleton.
 *
 * Since: 3.0
 */
static GtkWidget *
pidgin_application_get_account_manager(void) {
	static GtkWidget *manager = NULL;

	if(!PIDGIN_IS_ACCOUNT_MANAGER(manager)) {
		manager = pidgin_account_manager_new();
		g_object_add_weak_pointer(G_OBJECT(manager), (gpointer)&manager);
	}

	return manager;
}

/*
 * pidgin_application_present_transient_window:
 * @application: The application instance.
 * @window: The [class@Gtk.Window] to present.
 *
 * Presents a window and makes sure its transient parent is set to the currently
 * active window of @application.
 *
 * Since: 3.0
 */
static void
pidgin_application_present_transient_window(PidginApplication *application,
                                            GtkWindow *window)
{
	GtkWindow *active_window = NULL;

	g_return_if_fail(PIDGIN_IS_APPLICATION(application));
	g_return_if_fail(GTK_IS_WINDOW(window));

	active_window = pidgin_application_get_active_window(application);

	gtk_window_set_transient_for(window, active_window);

	gtk_window_present_with_time(window, GDK_CURRENT_TIME);
}

static void
pidgin_application_plugin_state_changed(G_GNUC_UNUSED GPluginManager *manager,
                                        G_GNUC_UNUSED GPluginPlugin *plugin,
                                        G_GNUC_UNUSED gpointer data)
{
	purple_plugins_save_loaded(PIDGIN_PREFS_ROOT "/plugins/loaded");
}

static void
pidgin_application_init_plugins(void) {
	GPluginManager *manager = gplugin_manager_get_default();

	gplugin_manager_append_paths_from_environment(manager,
	                                              "PIDGIN_PLUGIN_PATH");

	if(g_getenv("PURPLE_PLUGINS_SKIP")) {
		g_message("PURPLE_PLUGINS_SKIP environment variable set, skipping "
		          "normal Pidgin plugin paths");
	} else {
		gchar *path = g_build_filename(purple_data_dir(), "plugins", NULL);

		if(!g_file_test(path, G_FILE_TEST_IS_DIR)) {
			g_mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);
		}

		gplugin_manager_append_path(manager, path);
		g_free(path);

		gplugin_manager_append_path(manager, PIDGIN_LIBDIR);
	}

	g_signal_connect(manager, "loaded-plugin",
	                 G_CALLBACK(pidgin_application_plugin_state_changed), NULL);
	g_signal_connect(manager, "load-plugin-failed",
	                 G_CALLBACK(pidgin_application_plugin_state_changed), NULL);
	g_signal_connect(manager, "unloaded-plugin",
	                 G_CALLBACK(pidgin_application_plugin_state_changed), NULL);
	g_signal_connect(manager, "unload-plugin-failed",
	                 G_CALLBACK(pidgin_application_plugin_state_changed), NULL);

	purple_plugins_refresh();
}

static void
pidgin_application_populate_dynamic_menus(PidginApplication *application) {
	GMenu *target = NULL;
	GMenuModel *model = NULL;

	/* Link the AccountsDisabledMenu into its proper location. */
	model = pidgin_accounts_disabled_menu_new();
	target = gtk_application_get_menu_by_id(GTK_APPLICATION(application),
	                                        "disabled-accounts");
	g_menu_append_section(target, NULL, model);

	/* Link the AccountsEnabledMenu into its proper location. */
	model = pidgin_accounts_enabled_menu_new();
	target = gtk_application_get_menu_by_id(GTK_APPLICATION(application),
	                                        "enabled-accounts");
	g_menu_append_section(target, NULL, model);

	/* Link the PluginsMenu into its proper location. */
	model = pidgin_plugins_menu_new();
	target = gtk_application_get_menu_by_id(GTK_APPLICATION(application),
	                                        "plugins-menu");
	g_menu_append_section(target, NULL, model);
}

/******************************************************************************
 * Actions
 *****************************************************************************/
/*< private >
 * pidgin_application_online_actions:
 *
 * This list keeps track of which actions should only be enabled while online.
 */
static const gchar *pidgin_application_online_actions[] = {
};

/**
 * pidgin_application_channel_actions: (skip)
 *
 * This list keeps track of which actions should only be enabled if a protocol
 * supporting channels is connected.
 */
static const gchar *pidgin_application_channel_actions[] = {
	"join-channel",
};

/*< private >
 * pidgin_application_room_list_actions:
 *
 * This list keeps track of which actions should only be enabled if an online
 * account supports room lists.
 */
static const gchar *pidgin_application_room_list_actions[] = {
	"room-list",
};

/**
 * pidgin_action_group_actions_set_enable: (skip)
 * @group: The #PidginActionGroup instance.
 * @actions: The action names.
 * @n_actions: The number of @actions.
 * @enabled: Whether or not to enable the actions.
 *
 * Sets the enabled property of the named actions to @enabled.
 */
static void
pidgin_application_actions_set_enabled(PidginApplication *application,
                                       const gchar *const *actions,
                                       guint n_actions,
                                       gboolean enabled)
{
	guint i = 0;

	for(i = 0; i < n_actions; i++) {
		GAction *action = NULL;
		const gchar *name = actions[i];

		action = g_action_map_lookup_action(G_ACTION_MAP(application), name);

		if(action != NULL) {
			g_simple_action_set_enabled(G_SIMPLE_ACTION(action), enabled);
		} else {
			g_warning("Failed to find action named %s", name);
		}
	}
}

static void
pidgin_application_about(G_GNUC_UNUSED GSimpleAction *simple,
                         G_GNUC_UNUSED GVariant *parameter, gpointer data)
{
	PidginApplication *application = data;
	static GtkWidget *about = NULL;

	if(!GTK_IS_WIDGET(about)) {
		about = pidgin_about_dialog_new();
		g_object_add_weak_pointer(G_OBJECT(about), (gpointer)&about);
	}

	pidgin_application_present_transient_window(application, GTK_WINDOW(about));
}

static void
pidgin_application_accounts(G_GNUC_UNUSED GSimpleAction *simple,
                            G_GNUC_UNUSED GVariant *parameter, gpointer data)
{
	PidginApplication *application = data;
	GtkWidget *manager = pidgin_application_get_account_manager();

	pidgin_account_manager_show_overview(PIDGIN_ACCOUNT_MANAGER(manager));

	pidgin_application_present_transient_window(application,
	                                            GTK_WINDOW(manager));
}

static void
pidgin_application_connect_account(G_GNUC_UNUSED GSimpleAction *simple,
                                   GVariant *parameter,
                                   G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleAccountManager *manager = NULL;
	const gchar *id = NULL;

	id = g_variant_get_string(parameter, NULL);

	manager = purple_account_manager_get_default();

	account = purple_account_manager_find_by_id(manager, id);
	if(PURPLE_IS_ACCOUNT(account)) {
		purple_account_connect(account);
		g_clear_object(&account);
	}
}

static void
pidgin_application_debug(G_GNUC_UNUSED GSimpleAction *simple,
                         G_GNUC_UNUSED GVariant *parameter,
                         G_GNUC_UNUSED gpointer data)
{
	gboolean old = purple_prefs_get_bool(PIDGIN_PREFS_ROOT "/debug/enabled");
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/debug/enabled", !old);
}


static void
pidgin_application_disable_account(G_GNUC_UNUSED GSimpleAction *simple,
                                   GVariant *parameter,
                                   G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleAccountManager *manager = NULL;
	const gchar *id = NULL;

	id = g_variant_get_string(parameter, NULL);

	manager = purple_account_manager_get_default();

	account = purple_account_manager_find_by_id(manager, id);
	if(PURPLE_IS_ACCOUNT(account)) {
		if(purple_account_get_enabled(account)) {
			purple_account_set_enabled(account, FALSE);
		}

		g_clear_object(&account);
	}
}

static void
pidgin_application_donate(G_GNUC_UNUSED GSimpleAction *simple,
                          G_GNUC_UNUSED GVariant *parameter,
                          G_GNUC_UNUSED gpointer data)
{
	purple_notify_uri(NULL, "https://www.imfreedom.org/donate/");
}

static void
pidgin_application_edit_account(G_GNUC_UNUSED GSimpleAction *simple,
                                GVariant *parameter, gpointer data)
{
	PidginApplication *application = data;
	PurpleAccount *account = NULL;
	PurpleAccountManager *manager = NULL;
	const gchar *id = NULL;

	if(!g_variant_is_of_type(parameter, G_VARIANT_TYPE_STRING)) {
		g_warning("parameter is of type %s, expected %s",
		          g_variant_get_type_string(parameter),
		          (char *)G_VARIANT_TYPE_STRING);

		return;
	}

	id = g_variant_get_string(parameter, NULL);

	manager = purple_account_manager_get_default();

	account = purple_account_manager_find_by_id(manager, id);
	if(PURPLE_IS_ACCOUNT(account)) {
		GtkWidget *account_manager = pidgin_application_get_account_manager();

		pidgin_account_manager_edit_account(PIDGIN_ACCOUNT_MANAGER(account_manager),
		                                    account);

		pidgin_application_present_transient_window(application,
		                                            GTK_WINDOW(account_manager));

		g_clear_object(&account);
	}
}

static void
pidgin_application_enable_account(G_GNUC_UNUSED GSimpleAction *simple,
                                  GVariant *parameter,
                                  G_GNUC_UNUSED gpointer data)
{
	PurpleAccount *account = NULL;
	PurpleAccountManager *manager = NULL;
	const gchar *id = NULL;

	id = g_variant_get_string(parameter, NULL);

	manager = purple_account_manager_get_default();

	account = purple_account_manager_find_by_id(manager, id);
	if(PURPLE_IS_ACCOUNT(account)) {
		if(!purple_account_get_enabled(account)) {
			purple_account_set_enabled(account, TRUE);
		}

		g_clear_object(&account);
	}
}

static void
pidgin_application_file_transfers(G_GNUC_UNUSED GSimpleAction *simple,
                                  G_GNUC_UNUSED GVariant *parameter,
                                  G_GNUC_UNUSED gpointer data)
{
	g_warning("sorry not yet ported...");
}

static void
pidgin_application_join_channel(G_GNUC_UNUSED GSimpleAction *simple,
                                G_GNUC_UNUSED GVariant *parameter,
                                gpointer data)
{
	PidginApplication *application = data;
	static GtkWidget *dialog = NULL;

	if(!GTK_IS_WIDGET(dialog)) {
		dialog = pidgin_channel_join_dialog_new();
		g_object_add_weak_pointer(G_OBJECT(dialog), (gpointer)&dialog);
	}

	pidgin_application_present_transient_window(application,
	                                            GTK_WINDOW(dialog));
}

static void
pidgin_application_online_help(G_GNUC_UNUSED GSimpleAction *simple,
                               G_GNUC_UNUSED GVariant *parameter,
                               G_GNUC_UNUSED gpointer data)
{
	purple_notify_uri(NULL, PURPLE_WEBSITE "help");
}

static void
pidgin_application_plugins(G_GNUC_UNUSED GSimpleAction *simple,
                           G_GNUC_UNUSED GVariant *parameter, gpointer data)
{
	PidginApplication *application = data;
	static GtkWidget *dialog = NULL;

	if(!GTK_IS_WIDGET(dialog)) {
		dialog = pidgin_plugins_dialog_new();
		g_object_add_weak_pointer(G_OBJECT(dialog), (gpointer)&dialog);
	}

	pidgin_application_present_transient_window(application,
	                                            GTK_WINDOW(dialog));
}

static void
pidgin_application_preferences(G_GNUC_UNUSED GSimpleAction *simple,
                               G_GNUC_UNUSED GVariant *parameter,
                               gpointer data)
{
	PidginApplication *application = data;
	static GtkWidget *preferences = NULL;

	if(!GTK_IS_WIDGET(preferences)) {
		preferences = g_object_new(PIDGIN_TYPE_PREFS_WINDOW, NULL);
		g_object_add_weak_pointer(G_OBJECT(preferences), (gpointer)&preferences);
	}

	pidgin_application_present_transient_window(application,
	                                            GTK_WINDOW(preferences));

}

static void
pidgin_application_quit(G_GNUC_UNUSED GSimpleAction *simple,
                        G_GNUC_UNUSED GVariant *parameter,
                        G_GNUC_UNUSED gpointer data)
{
	GPluginManager *manager = NULL;

	/* Remove the signal handlers for plugin state changing so we don't try to
	 * update preferences.
	 */
	manager = gplugin_manager_get_default();
	g_signal_handlers_disconnect_by_func(manager,
	                                     pidgin_application_plugin_state_changed,
	                                     NULL);

	purple_core_quit();
}

static void
pidgin_application_room_list(G_GNUC_UNUSED GSimpleAction *simple,
                             G_GNUC_UNUSED GVariant *parameter,
                             G_GNUC_UNUSED gpointer data)
{
	pidgin_roomlist_dialog_show();
}

static GActionEntry app_entries[] = {
	{
		.name = "about",
		.activate = pidgin_application_about,
	}, {
		.name = "connect-account",
		.activate = pidgin_application_connect_account,
		.parameter_type = "s",
	}, {
		.name = "debug",
		.activate = pidgin_application_debug,
	}, {
		.name = "disable-account",
		.activate = pidgin_application_disable_account,
		.parameter_type = "s",
	}, {
		.name = "donate",
		.activate = pidgin_application_donate,
	}, {
		.name = "edit-account",
		.activate = pidgin_application_edit_account,
		.parameter_type = "s",
	}, {
		.name = "enable-account",
		.activate = pidgin_application_enable_account,
		.parameter_type = "s",
	}, {
		.name = "file-transfers",
		.activate = pidgin_application_file_transfers,
	}, {
		.name = "join-channel",
		.activate = pidgin_application_join_channel,
	}, {
		.name = "manage-accounts",
		.activate = pidgin_application_accounts,
	}, {
		.name = "manage-plugins",
		.activate = pidgin_application_plugins,
	}, {
		.name = "online-help",
		.activate = pidgin_application_online_help,
	}, {
		.name = "preferences",
		.activate = pidgin_application_preferences,
	}, {
		.name = "quit",
		.activate = pidgin_application_quit,
	}, {
		.name = "room-list",
		.activate = pidgin_application_room_list,
	}
};

/******************************************************************************
 * Shortcuts
 *****************************************************************************/
static inline void
pidgin_application_add_shortcut(PidginApplication *application,
                                const char *action,
                                const char *accelerator)
{
	const char *accels[] = {accelerator, NULL};

	gtk_application_set_accels_for_action(GTK_APPLICATION(application),
	                                      action, accels);
}

static void
pidgin_application_add_shortcuts(PidginApplication *application) {
	pidgin_application_add_shortcut(application, "app.file-transfers",
	                                "<Primary>T");
	pidgin_application_add_shortcut(application, "app.get-user-info",
	                                "<Primary>I");
	pidgin_application_add_shortcut(application, "app.manage-accounts",
	                                "<Primary>S");
	pidgin_application_add_shortcut(application, "app.manage-plugins",
	                                "<Primary>U");
	pidgin_application_add_shortcut(application, "app.new-message",
	                                "<Primary>M");
	pidgin_application_add_shortcut(application, "app.preferences",
	                                "<Primary>comma");
	pidgin_application_add_shortcut(application, "app.quit", "<Primary>Q");
}

/******************************************************************************
 * Purple Signal Callbacks
 *****************************************************************************/
static void
pidgin_application_connected_cb(G_GNUC_UNUSED PurpleAccountManager *manager,
                                PurpleAccount *account,
                                gpointer data)
{
	PidginApplication *application = data;
	PurpleProtocol *protocol = NULL;
	gboolean should_enable_channel = FALSE;
	gboolean should_enable_room_list = FALSE;
	guint n_actions = 0;

	n_actions = G_N_ELEMENTS(pidgin_application_online_actions);
	pidgin_application_actions_set_enabled(PIDGIN_APPLICATION(data),
	                                       pidgin_application_online_actions,
	                                       n_actions,
	                                       TRUE);

	/* Now figure out what menus items should be enabled. */
	protocol = purple_account_get_protocol(account);

	/* We assume that the current state is correct, so we don't bother changing
	 * state unless the newly connected account implements the channel virtual
	 * functions, which would cause a state change.
	 */
	should_enable_channel = PURPLE_PROTOCOL_IMPLEMENTS(protocol, CONVERSATION,
	                                                   get_channel_join_details);
	if(should_enable_channel) {
		n_actions = G_N_ELEMENTS(pidgin_application_channel_actions);
		pidgin_application_actions_set_enabled(application,
		                                       pidgin_application_channel_actions,
		                                       n_actions,
		                                       TRUE);
	}

	/* likewise, for the room list, we only care about enabling in this
	 * handler.
	 */
	should_enable_room_list = PURPLE_PROTOCOL_IMPLEMENTS(protocol, ROOMLIST,
	                                                     get_list);
	if(should_enable_room_list) {
		n_actions = G_N_ELEMENTS(pidgin_application_room_list_actions);
		pidgin_application_actions_set_enabled(application,
		                                       pidgin_application_room_list_actions,
		                                       n_actions,
		                                       TRUE);
	}
}

static void
pidgin_application_disconnected_cb(PurpleAccountManager *manager,
                                   G_GNUC_UNUSED PurpleAccount *account,
                                   gpointer data)
{
	PidginApplication *application = data;
	GList *connected = NULL;
	gboolean should_disable_actions = TRUE;
	gboolean should_disable_channel = TRUE;
	gboolean should_disable_room_list = TRUE;
	guint n_actions = 0;

	connected = purple_account_manager_get_connected(manager);

	while(connected != NULL) {
		PurpleAccount *account = connected->data;
		PurpleProtocol *protocol = NULL;

		/* We have at least one account connected so we're online. */
		should_disable_actions = FALSE;

		protocol = purple_account_get_protocol(account);

		/* Check if the protocol implements joining channels. */
		if(PURPLE_PROTOCOL_IMPLEMENTS(protocol, CONVERSATION,
		                              get_channel_join_details))
		{
			should_disable_channel = FALSE;
		}

		/* Check if the protocol implements the room list interface. */
		if(PURPLE_PROTOCOL_IMPLEMENTS(protocol, ROOMLIST, get_list)) {
			should_disable_room_list = FALSE;
		}

		/* If we can't disable anything we can exit the loop early. */
		if(!should_disable_channel && !should_disable_room_list) {
			g_clear_list(&connected, NULL);

			break;
		}

		connected = g_list_delete_link(connected, connected);
	}

	if(should_disable_actions) {
		n_actions = G_N_ELEMENTS(pidgin_application_online_actions);
		pidgin_application_actions_set_enabled(PIDGIN_APPLICATION(data),
		                                       pidgin_application_online_actions,
		                                       n_actions,
		                                       FALSE);
	}

	if(should_disable_channel) {
		n_actions = G_N_ELEMENTS(pidgin_application_channel_actions);
		pidgin_application_actions_set_enabled(application,
		                                       pidgin_application_channel_actions,
		                                       n_actions,
		                                       FALSE);
	}

	if(should_disable_room_list) {
		n_actions = G_N_ELEMENTS(pidgin_application_room_list_actions);
		pidgin_application_actions_set_enabled(application,
		                                       pidgin_application_room_list_actions,
		                                       n_actions,
		                                       FALSE);
	}
}

static void
pidgin_application_error_reponse_cb(G_GNUC_UNUSED AdwMessageDialog *self,
                                    G_GNUC_UNUSED char *response,
                                    gpointer data)
{
	g_application_quit(data);
}

/******************************************************************************
 * GtkApplication Implementation
 *****************************************************************************/
static void
pidgin_application_window_added(GtkApplication *application,
                                GtkWindow *window)
{
	PidginApplication *pidgin_application = PIDGIN_APPLICATION(application);
	GHashTableIter iter;
	gpointer key, value;

	GTK_APPLICATION_CLASS(pidgin_application_parent_class)->window_added(application,
	                                                                     window);

	if(strstr(VERSION, "-devel")) {
		gtk_widget_add_css_class(GTK_WIDGET(window), "devel");
	}

	g_hash_table_iter_init(&iter, pidgin_application->action_groups);
	while(g_hash_table_iter_next(&iter, &key, &value)) {
		GActionGroup *action_group = value;
		gchar *prefix = key;

		gtk_widget_insert_action_group(GTK_WIDGET(window), prefix,
		                               action_group);
	}
}

/******************************************************************************
 * GApplication Implementation
 *****************************************************************************/
static void
pidgin_application_startup(GApplication *application) {
	PurpleAccountManager *manager = NULL;
	GError *error = NULL;
	GList *active_accounts = NULL;

	G_APPLICATION_CLASS(pidgin_application_parent_class)->startup(application);

	/* set a user-specified config directory */
	if (opt_config_dir_arg != NULL) {
		if (g_path_is_absolute(opt_config_dir_arg)) {
			purple_util_set_user_dir(opt_config_dir_arg);
		} else {
			/* Make an absolute (if not canonical) path */
			gchar *cwd = g_get_current_dir();
			gchar *path = g_build_filename(cwd, opt_config_dir_arg, NULL);

			purple_util_set_user_dir(path);

			g_free(cwd);
			g_free(path);
		}
	}

	pidgin_debug_init_handler();
#ifdef DEBUG
	pidgin_debug_set_print_enabled(TRUE);
#else
	pidgin_debug_set_print_enabled(opt_debug);
#endif

#ifdef _WIN32
	winpidgin_init();
#endif

	if(!purple_core_init(pidgin_ui_new(), &error)) {
		GtkWidget *message = NULL;
		const char *error_message = "unknown error";

		if(error != NULL) {
			error_message = error->message;
		}

		message = adw_message_dialog_new(NULL,
		                                 _("Pidgin 3 failed to initialize"),
		                                 error_message);
		g_clear_error(&error);

		adw_message_dialog_add_responses(ADW_MESSAGE_DIALOG(message),
		                                 "close", _("Close"), NULL);
		adw_message_dialog_set_close_response(ADW_MESSAGE_DIALOG(message),
		                                      "close");

		g_signal_connect(message, "response",
		                 G_CALLBACK(pidgin_application_error_reponse_cb),
		                 application);

		gtk_window_present_with_time(GTK_WINDOW(message), GDK_CURRENT_TIME);

		return;
	}

	pidgin_application_init_plugins();

	/* load plugins we had when we quit */
	purple_plugins_load_saved(PIDGIN_PREFS_ROOT "/plugins/loaded");

	gtk_window_set_default_icon_name("pidgin");

	g_free(opt_config_dir_arg);
	opt_config_dir_arg = NULL;

	/*
	 * We want to show the blist early in the init process so the
	 * user feels warm and fuzzy.
	 */
	purple_blist_show();

	if(purple_prefs_get_bool(PIDGIN_PREFS_ROOT "/debug/enabled")) {
		pidgin_debug_window_show();
	}

	manager = purple_account_manager_get_default();

	if(opt_nologin) {
		purple_account_manager_set_online(manager, FALSE);
	} else {
		purple_account_manager_set_online(manager, TRUE);
	}

	active_accounts = purple_account_manager_get_enabled(manager);
	if(active_accounts == NULL) {
		g_action_group_activate_action(G_ACTION_GROUP(application),
		                               "manage-accounts", NULL);
	} else {
		g_list_free(active_accounts);
	}

	/* Populate our dynamic menus. */
	pidgin_application_populate_dynamic_menus(PIDGIN_APPLICATION(application));

	/* Connect to the connected and disconnected signals to manage which menus
	 * are active.
	 */
	g_signal_connect_object(manager, "account-connected",
	                        G_CALLBACK(pidgin_application_connected_cb),
	                        application, 0);
	g_signal_connect_object(manager, "account-disconnected",
	                        G_CALLBACK(pidgin_application_disconnected_cb),
	                        application, 0);
}

static void
pidgin_application_activate(G_GNUC_UNUSED GApplication *application) {
	GtkWidget *convwin = pidgin_display_window_get_default();

	if(GTK_IS_WINDOW(convwin)) {
		gtk_window_present(GTK_WINDOW(convwin));
	}
}

static gint
pidgin_application_command_line(GApplication *application,
                                GApplicationCommandLine *cmdline)
{
	gchar **argv = NULL;
	gint argc = 0, i = 0;

	argv = g_application_command_line_get_arguments(cmdline, &argc);

	if(argc == 1) {
		/* No arguments, just activate */
		g_application_activate(application);
	}

	/* Start at 1 to skip the executable name */
	for (i = 1; i < argc; i++) {
		purple_got_protocol_handler_uri(argv[i]);
	}

	g_strfreev(argv);

	return 0;
}

static gint
pidgin_application_handle_local_options(G_GNUC_UNUSED GApplication *application,
                                        GVariantDict *options)
{
	if (g_variant_dict_contains(options, "version")) {
		printf("%s %s (libpurple %s)\n", PIDGIN_NAME, DISPLAY_VERSION,
		       purple_core_get_version());

		return 0;
	}

	return -1;
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
static void
pidgin_application_dispose(GObject *obj) {
	PidginApplication *application = PIDGIN_APPLICATION(obj);

	g_clear_pointer(&application->action_groups, g_hash_table_destroy);

	G_OBJECT_CLASS(pidgin_application_parent_class)->dispose(obj);
}

static void
pidgin_application_init(PidginApplication *application) {
	GApplication *gapp = G_APPLICATION(application);
	gboolean online = FALSE;
	gint n_actions = 0;

	application->action_groups = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                                   g_free, g_object_unref);

	g_application_add_main_option_entries(gapp, option_entries);
	g_application_add_option_group(gapp, purple_get_option_group());
	g_application_add_option_group(gapp, gplugin_get_option_group());

	g_action_map_add_action_entries(G_ACTION_MAP(application), app_entries,
	                                G_N_ELEMENTS(app_entries), application);

	pidgin_application_add_shortcuts(application);

	/* Set the default state for our actions to match our online state. */
	online = purple_connections_is_online();

	n_actions = G_N_ELEMENTS(pidgin_application_online_actions);
	pidgin_application_actions_set_enabled(application,
	                                       pidgin_application_online_actions,
	                                       n_actions,
	                                       online);

	n_actions = G_N_ELEMENTS(pidgin_application_channel_actions);
	pidgin_application_actions_set_enabled(application,
	                                       pidgin_application_channel_actions,
	                                       n_actions,
	                                       online);

	n_actions = G_N_ELEMENTS(pidgin_application_room_list_actions);
	pidgin_application_actions_set_enabled(application,
	                                       pidgin_application_room_list_actions,
	                                       n_actions,
	                                       online);
}

static void
pidgin_application_class_init(PidginApplicationClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GApplicationClass *app_class = G_APPLICATION_CLASS(klass);
	GtkApplicationClass *gtk_app_class = GTK_APPLICATION_CLASS(klass);

	obj_class->dispose = pidgin_application_dispose;

	app_class->startup = pidgin_application_startup;
	app_class->activate = pidgin_application_activate;
	app_class->command_line = pidgin_application_command_line;
	app_class->handle_local_options = pidgin_application_handle_local_options;

	gtk_app_class->window_added = pidgin_application_window_added;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GApplication *
pidgin_application_new(void) {
	return g_object_new(
		PIDGIN_TYPE_APPLICATION,
		"application-id", "im.pidgin.Pidgin3",
		"flags", G_APPLICATION_CAN_OVERRIDE_APP_ID |
		         G_APPLICATION_HANDLES_COMMAND_LINE,
		"register-session", TRUE,
		NULL);
}

void
pidgin_application_add_action_group(PidginApplication *application,
                                    const gchar *prefix,
                                    GActionGroup *action_group)
{
	GList *windows = NULL;

	g_return_if_fail(prefix != NULL);

	if(G_IS_ACTION_GROUP(action_group)) {
		/* If action_group is valid, we need to create new references to the
		 * prefix and action_group when inserting them into the hash table.
		 */
		g_hash_table_insert(application->action_groups, g_strdup(prefix),
		                    g_object_ref(action_group));
	} else {
		g_hash_table_remove(application->action_groups, prefix);
	}

	/* Now walk through the windows and add/remove the action group. */
	windows = gtk_application_get_windows(GTK_APPLICATION(application));
	while(windows != NULL) {
		GtkWidget *window = GTK_WIDGET(windows->data);

		gtk_widget_insert_action_group(window, prefix, action_group);

		windows = windows->next;
	}
}

GtkWindow *
pidgin_application_get_active_window(PidginApplication *application) {
	GtkApplication *gtk_application = NULL;
	GtkWindow *window = NULL;

	g_return_val_if_fail(PIDGIN_IS_APPLICATION(application), NULL);

	gtk_application = GTK_APPLICATION(application);

	window = gtk_application_get_active_window(gtk_application);
	if(!GTK_IS_WINDOW(window)) {
		GList *windows = NULL;

		windows = gtk_application_get_windows(gtk_application);
		if(windows != NULL) {
			window = windows->data;
		}
	}

	return window;
}
