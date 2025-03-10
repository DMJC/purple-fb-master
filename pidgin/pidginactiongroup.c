/*
 * pidgin
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

#include "pidginactiongroup.h"

#include <purple.h>

#include "internal.h"

#include "pidgin/gtkblist.h"
#include "pidgin/gtkdialogs.h"
#include "pidgin/gtkpounce.h"
#include "pidgin/gtkprefs.h"
#include "pidgin/gtkprivacy.h"
#include "pidgin/gtkroomlist.h"
#include "pidgin/gtksmiley-manager.h"
#include "pidgin/gtkxfer.h"
#include "pidgin/pidginabout.h"
#include "pidgin/pidginlog.h"
#include "pidgin/pidginmooddialog.h"

struct _PidginActionGroup {
	GSimpleActionGroup parent;
};

/******************************************************************************
 * Globals
 *****************************************************************************/

/**< private >
 * online_actions:
 *
 * This list keeps track of which actions should only be enabled while online.
 */
static const gchar *pidgin_action_group_online_actions[] = {
	PIDGIN_ACTION_ADD_BUDDY,
	PIDGIN_ACTION_ADD_GROUP,
	PIDGIN_ACTION_GET_USER_INFO,
	PIDGIN_ACTION_NEW_MESSAGE,
	PIDGIN_ACTION_PRIVACY,
	PIDGIN_ACTION_SET_MOOD,
};

static const gchar *pidgin_action_group_chat_actions[] = {
	PIDGIN_ACTION_ADD_CHAT,
	PIDGIN_ACTION_JOIN_CHAT,
};

static const gchar *pidgin_action_group_room_list_actions[] = {
	PIDGIN_ACTION_ROOM_LIST,
};

/******************************************************************************
 * Helpers
 *****************************************************************************/

/*< private >
 * pidgin_action_group_bool_pref_handler:
 * @group: The #PidginActionGroup instance.
 * @action_name: The name of the action to update.
 * @value: The value of the preference.
 *
 * Changes the state of the action named @action_name to match @value.
 *
 * This function is meant to be called from a #PurplePrefCallback function as
 * there isn't a good way to have a #PurplePrefCallback with multiple items in
 * the data parameter without leaking them forever.
 */
static void
pidgin_action_group_bool_pref_handler(PidginActionGroup *group,
                                      const gchar *action_name,
                                      gboolean value)
{
	GAction *action = NULL;

	action = g_action_map_lookup_action(G_ACTION_MAP(group), action_name);
	if(action != NULL) {
		g_simple_action_set_state(G_SIMPLE_ACTION(action),
		                          g_variant_new_boolean(value));
	}
}

/*< private >
 * pidgin_action_group_setup_bool:
 * @group: The #PidginActionGroup instance.
 * @action_name: The name of the action to setup.
 * @pref_name: The name of the preference that @action_name is tied to.
 * @callback: (scope notified): A #PurplePrefCallback to call when the
 *            preference is changed.
 *
 * Initializes the boolean action named @action_name to the value of @pref_name
 * and setups up a preference change callback to @callback to maintain the
 * state of the action.
 */
static void
pidgin_action_group_setup_bool(PidginActionGroup *group,
                               const gchar *action_name,
                               const gchar *pref_name,
                               PurplePrefCallback callback)
{
	GAction *action = NULL;
	gboolean value = FALSE;

	/* find the action, if we can't find it, bail */
	action = g_action_map_lookup_action(G_ACTION_MAP(group), action_name);
	g_return_if_fail(action != NULL);

	/* get the value of the preference */
	value = purple_prefs_get_bool(pref_name);

	/* change the state of the action to match the preference value. */
	g_simple_action_set_state(G_SIMPLE_ACTION(action),
	                          g_variant_new_boolean(value));

	/* finally add a preference callback to update the state based on the
	 * preference.
	 */
	purple_prefs_connect_callback(group, pref_name, callback, group);
}

/*< private >
 * pidgin_action_group_string_pref_handler:
 * @group: The #PidginActionGroup instance.
 * @action_name: The name of the action to update.
 * @value: The value of the preference.
 *
 * Changes the state of the action named @action_name to match @value.
 *
 * This function is meant to be called from a #PurplePrefCallback function as
 * there isn't a good way to have a #PurplePrefCallback with multiple items in
 * the data parameter without leaking them forever.
 */
static void
pidgin_action_group_string_pref_handler(PidginActionGroup *group,
                                        const gchar *action_name,
                                        const gchar *value)
{
	GAction *action = NULL;

	action = g_action_map_lookup_action(G_ACTION_MAP(group), action_name);
	if(action != NULL) {
		g_simple_action_set_state(G_SIMPLE_ACTION(action),
		                          g_variant_new_string(value));
	}
}

/*< private >
 * pidgin_action_group_setup_string:
 * @group: The #PidginActionGroup instance.
 * @action_name: The name of the action to setup.
 * @pref_name: The name of the preference that @action_name is tied to.
 * @callback: (scope notified): A #PurplePrefCallback to call when the
 *            preference is changed.
 *
 * Initializes the string action named @action_name to the value of @pref_name
 * and setups up a preference change callback to @callback to maintain the
 * state of the action.
 */
static void
pidgin_action_group_setup_string(PidginActionGroup *group,
                                 const gchar *action_name,
                                 const gchar *pref_name,
                                 PurplePrefCallback callback)
{
	GAction *action = NULL;
	const gchar *value = NULL;

	/* find the action, if we can't find it, bail */
	action = g_action_map_lookup_action(G_ACTION_MAP(group), action_name);
	g_return_if_fail(action != NULL);

	/* change the state of the action to match the preference value. */
	value = purple_prefs_get_string(pref_name);
	g_simple_action_set_state(G_SIMPLE_ACTION(action),
	                          g_variant_new_string(value));

	/* finally add a preference callback to update the state based on the
	 * preference.
	 */
	purple_prefs_connect_callback(group, pref_name, callback, group);
}

/*< private >
 * pidgin_action_group_actions_set_enable:
 * @group: The #PidginActionGroup instance.
 * @actions: The action names.
 * @n_actions: The number of @actions.
 * @enabled: Whether or not to enable the actions.
 *
 * Sets the enabled property of the named actions to @enabled.
 */
static void
pidgin_action_group_actions_set_enable(PidginActionGroup *group,
                                       const gchar *const *actions,
                                       gint n_actions,
                                       gboolean enabled)
{
	gint i = 0;

	for(i = 0; i < n_actions; i++) {
		GAction *action = NULL;
		const gchar *name = actions[i];

		action = g_action_map_lookup_action(G_ACTION_MAP(group), name);

		if(action != NULL) {
			g_simple_action_set_enabled(G_SIMPLE_ACTION(action), enabled);
		} else {
			g_warning("Failed to find action named %s", name);
		}
	}
}

/*< private >
 * pidgin_action_group_online_actions_set_enable:
 * @group: The #PidginActionGroup instance.
 * @enabled: %TRUE to enable the actions, %FALSE to disable them.
 *
 * Enables/disables the actions that require being online to use.
 */
static void
pidgin_action_group_online_actions_set_enable(PidginActionGroup *group,
                                              gboolean enabled)
{
	gint n_actions = G_N_ELEMENTS(pidgin_action_group_online_actions);

	pidgin_action_group_actions_set_enable(group,
	                                       pidgin_action_group_online_actions,
	                                       n_actions,
	                                       enabled);
}

/*< private >
 * pidgin_action_group_chat_actions_set_enable:
 * @group: The #PidginActionGroup instance.
 * @enabled: Whether or not to enable/disable the actions.
 *
 * Sets the enabled state of the chat specific actions to the value of @enabled.
 */
static void
pidgin_action_group_chat_actions_set_enable(PidginActionGroup *group,
                                            gboolean enabled)
{
	gint n_actions = G_N_ELEMENTS(pidgin_action_group_chat_actions);

	pidgin_action_group_actions_set_enable(group,
	                                       pidgin_action_group_chat_actions,
	                                       n_actions,
	                                       enabled);
}

/*< private >
 * pidgin_action_group_room_list_actions_set_enable:
 * @group: The #PidginActionGroup instance.
 * @enabled: Whether or not to enable/disable the actions.
 *
 * Sets the enabled state of the room list specific actions to the value of
 * @enabled.
 */
static void
pidgin_action_group_room_list_actions_set_enable(PidginActionGroup *group,
                                                 gboolean enabled)
{
	gint n_actions = G_N_ELEMENTS(pidgin_action_group_room_list_actions);

	pidgin_action_group_actions_set_enable(group,
	                                       pidgin_action_group_room_list_actions,
	                                       n_actions,
	                                       enabled);
}

/******************************************************************************
 * Purple Signal Callbacks
 *****************************************************************************/
static void
pidgin_action_group_online_cb(gpointer data) {
	pidgin_action_group_online_actions_set_enable(PIDGIN_ACTION_GROUP(data),
	                                              TRUE);
}

static void
pidgin_action_group_offline_cb(gpointer data) {
	pidgin_action_group_online_actions_set_enable(PIDGIN_ACTION_GROUP(data),
	                                              FALSE);
}

static void
pidgin_action_group_signed_on_cb(PurpleAccount *account, gpointer data) {
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);
	PurpleProtocol *protocol = NULL;
	const gchar *protocol_id = NULL;
	gboolean should_enable_chat = FALSE, should_enable_room_list = FALSE;

	protocol_id = purple_account_get_protocol_id(account);
	protocol = purple_protocols_find(protocol_id);

	/* We assume that the current state is correct, so we don't bother changing
	 * state unless the newly connected account implements the chat interface,
	 * which would cause a state change.
	 */
	should_enable_chat = PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, info);
	if(should_enable_chat) {
		pidgin_action_group_chat_actions_set_enable(group, TRUE);
	}

	/* likewise, for the room list, we only care about enabling in this
	 * handler.
	 */
	should_enable_room_list = PURPLE_PROTOCOL_IMPLEMENTS(protocol, ROOMLIST,
	                                                     get_list);
	if(should_enable_room_list) {
		pidgin_action_group_room_list_actions_set_enable(group, TRUE);
	}
}

static void
pidgin_action_group_signed_off_cb(PurpleAccount *account, gpointer data) {
	gboolean should_disable_chat = TRUE, should_disable_room_list = TRUE;
	GList *connections = NULL, *l = NULL;

	/* walk through all the connections, looking for online ones that implement
	 * the chat interface.  We don't bother checking the account that this
	 * signal was emitted for, because it's already offline and will be
	 * filtered out by the online check.
	 */
	connections = purple_connections_get_all();
	for(l = connections; l != NULL; l = l->next) {
		PurpleConnection *connection = PURPLE_CONNECTION(l->data);
		PurpleProtocol *protocol = NULL;

		/* if the connection isn't online, we don't care about it */
		if(!PURPLE_CONNECTION_IS_CONNECTED(connection)) {
			continue;
		}

		protocol = purple_connection_get_protocol(connection);

		/* check if the protocol implements the chat interface */
		if(PURPLE_PROTOCOL_IMPLEMENTS(protocol, CHAT, info)) {
			should_disable_chat = FALSE;
		}

		/* check if the protocol implement the room list interface */
		if(PURPLE_PROTOCOL_IMPLEMENTS(protocol, ROOMLIST, get_list)) {
			should_disable_room_list = FALSE;
		}

		/* if we can't disable both, we can bail out of the loop */
		if(!should_disable_chat && !should_disable_room_list) {
			break;
		}
	}

	if(should_disable_chat) {
		pidgin_action_group_chat_actions_set_enable(PIDGIN_ACTION_GROUP(data),
		                                            FALSE);
	}

	if(should_disable_room_list) {
		pidgin_action_group_room_list_actions_set_enable(PIDGIN_ACTION_GROUP(data),
		                                                 FALSE);
	}
}

/******************************************************************************
 * Preference Callbacks
 *****************************************************************************/
static void
pidgin_action_group_mute_sounds_callback(const gchar *name,
                                         PurplePrefType type,
                                         gconstpointer value,
                                         gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_bool_pref_handler(group, PIDGIN_ACTION_MUTE_SOUNDS,
	                                      (gboolean)GPOINTER_TO_INT(value));
}

static void
pidgin_action_group_show_buddy_icons_callback(const gchar *name,
                                              PurplePrefType type,
                                              gconstpointer value,
                                              gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_bool_pref_handler(group,
	                                      PIDGIN_ACTION_SHOW_BUDDY_ICONS,
	                                      (gboolean)GPOINTER_TO_INT(value));
}

static void
pidgin_action_group_show_empty_groups_callback(const gchar *name,
                                               PurplePrefType type,
                                               gconstpointer value,
                                               gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_bool_pref_handler(group,
	                                      PIDGIN_ACTION_SHOW_EMPTY_GROUPS,
	                                      (gboolean)GPOINTER_TO_INT(value));
}

static void
pidgin_action_group_show_idle_times_callback(const gchar *name,
                                             PurplePrefType type,
                                             gconstpointer value,
                                             gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_bool_pref_handler(group,
	                                      PIDGIN_ACTION_SHOW_IDLE_TIMES,
	                                      (gboolean)GPOINTER_TO_INT(value));
}

static void
pidgin_action_group_show_offline_buddies_callback(const gchar *name,
                                                  PurplePrefType type,
                                                  gconstpointer value,
                                                  gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_bool_pref_handler(group,
	                                      PIDGIN_ACTION_SHOW_OFFLINE_BUDDIES,
	                                      (gboolean)GPOINTER_TO_INT(value));
}

static void
pidgin_action_group_show_protocol_icons_callback(const gchar *name,
                                                 PurplePrefType type,
                                                 gconstpointer value,
                                                 gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_bool_pref_handler(group,
	                                      PIDGIN_ACTION_SHOW_PROTOCOL_ICONS,
	                                      (gboolean)GPOINTER_TO_INT(value));
}

static void
pidgin_action_group_sort_method_callback(const gchar *name,
                                         PurplePrefType type,
                                         gconstpointer value,
                                         gpointer data)
{
	PidginActionGroup *group = PIDGIN_ACTION_GROUP(data);

	pidgin_action_group_string_pref_handler(group,
	                                        PIDGIN_ACTION_SORT_METHOD,
	                                        value);
}

/******************************************************************************
 * Action Callbacks
 *****************************************************************************/
static void
pidgin_action_group_about(GSimpleAction *simple, GVariant *parameter,
                          gpointer data)
{
	GtkWidget *about = pidgin_about_dialog_new();

	/* fix me? */
#if 0
	gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(window));
#endif

	gtk_widget_show_all(about);
}

static void
pidgin_action_group_add_buddy(GSimpleAction *simple, GVariant *parameter,
                              gpointer data)
{
	purple_blist_request_add_buddy(NULL, NULL, NULL, NULL);
}

static void
pidgin_action_group_add_chat(GSimpleAction *simple, GVariant *parameter,
                             gpointer data)
{
	purple_blist_request_add_chat(NULL, NULL, NULL, NULL);
}

static void
pidgin_action_group_add_group(GSimpleAction *simple, GVariant *parameter,
                              gpointer data)
{
	purple_blist_request_add_group();
}

static void
pidgin_action_group_buddy_pounces(GSimpleAction *simple, GVariant *parameter,
                                  gpointer data)
{
	pidgin_pounces_manager_show();
}

static void
pidgin_action_group_custom_smiley(GSimpleAction *simple, GVariant *parameter,
                                  gpointer data)
{
	pidgin_smiley_manager_show();
}

static void
pidgin_action_group_debug(GSimpleAction *simple, GVariant *parameter,
                          gpointer data)
{
	gboolean old = purple_prefs_get_bool(PIDGIN_PREFS_ROOT "/debug/enabled");
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/debug/enabled", !old);
}

static void
pidgin_action_group_file_transfers(GSimpleAction *simple, GVariant *parameter,
                                   gpointer data)
{
	pidgin_xfer_dialog_show(NULL);
}

static void
pidgin_action_group_get_user_info(GSimpleAction *simple, GVariant *parameter,
                                  gpointer data)
{
	pidgin_dialogs_info();
}

static void
pidgin_action_group_join_chat(GSimpleAction *simple, GVariant *parameter,
                              gpointer data)
{
	pidgin_blist_joinchat_show();
}

static void
pidgin_action_group_mute_sounds(GSimpleAction *action, GVariant *value,
                                gpointer data)
{
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/sound/mute",
	                      g_variant_get_boolean(value));
}

static void
pidgin_action_group_new_message(GSimpleAction *simple, GVariant *parameter,
                                gpointer data)
{
	pidgin_dialogs_im();
}

static void
pidgin_action_group_online_help(GSimpleAction *simple, GVariant *parameter,
                                gpointer data)
{
	purple_notify_uri(NULL, PURPLE_WEBSITE "help");
}

static void
pidgin_action_group_preferences(GSimpleAction *simple, GVariant *parameter,
                                gpointer data)
{
	pidgin_prefs_show();
}

static void
pidgin_action_group_privacy(GSimpleAction *simple, GVariant *parameter,
                            gpointer data)
{
	pidgin_privacy_dialog_show();
}

static void
pidgin_action_group_quit(GSimpleAction *simple, GVariant *parameter,
                         gpointer data)
{
	purple_core_quit();
}

static void
pidgin_action_group_room_list(GSimpleAction *simple, GVariant *parameter,
                              gpointer data)
{
	pidgin_roomlist_dialog_show();
}

static void
pidgin_action_group_set_mood(GSimpleAction *simple, GVariant *parameter,
                             gpointer data)
{
	pidgin_mood_dialog_show(NULL);
}

static void
pidgin_action_group_show_buddy_icons(GSimpleAction *action, GVariant *value,
                                     gpointer data)
{
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/blist/show_buddy_icons",
	                      g_variant_get_boolean(value));
}

static void
pidgin_action_group_show_empty_groups(GSimpleAction *action, GVariant *value,
                                      gpointer data)
{
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/blist/show_empty_groups",
	                      g_variant_get_boolean(value));
}

static void
pidgin_action_group_show_idle_times(GSimpleAction *action,
                                    GVariant *value,
                                    gpointer data)
{
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/blist/show_idle_time",
	                      g_variant_get_boolean(value));
}

static void
pidgin_action_group_show_offline_buddies(GSimpleAction *action,
                                         GVariant *value,
                                         gpointer data)
{
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/blist/show_offline_buddies",
	                      g_variant_get_boolean(value));
}

static void
pidgin_action_group_show_protocol_icons(GSimpleAction *action,
                                        GVariant *value,
                                        gpointer data)
{
	purple_prefs_set_bool(PIDGIN_PREFS_ROOT "/blist/show_protocol_icons",
	                      g_variant_get_boolean(value));
}

static void
pidgin_action_group_sort_method(GSimpleAction *action, GVariant *value,
                                gpointer data)
{
	purple_prefs_set_string(PIDGIN_PREFS_ROOT "/blist/sort_type",
	                        g_variant_get_string(value, NULL));
}

static void
pidgin_action_group_system_log(GSimpleAction *simple, GVariant *parameter,
                               gpointer data)
{
	pidgin_syslog_show();
}

static void
pidgin_action_group_view_user_log(GSimpleAction *simple, GVariant *parameter,
                                  gpointer data)
{
	pidgin_dialogs_log();
}

/******************************************************************************
 * GObject Implementation
 *****************************************************************************/
G_DEFINE_TYPE(PidginActionGroup, pidgin_action_group,
              G_TYPE_SIMPLE_ACTION_GROUP)

static void
pidgin_action_group_init(PidginActionGroup *group) {
	gpointer handle = NULL;
	GActionEntry entries[] = {
		{
			.name = PIDGIN_ACTION_ABOUT,
			.activate = pidgin_action_group_about,
		}, {
			.name = PIDGIN_ACTION_ADD_BUDDY,
			.activate = pidgin_action_group_add_buddy,
		}, {
			.name = PIDGIN_ACTION_ADD_CHAT,
			.activate = pidgin_action_group_add_chat,
		}, {
			.name = PIDGIN_ACTION_ADD_GROUP,
			.activate = pidgin_action_group_add_group,
		}, {
			.name = PIDGIN_ACTION_BUDDY_POUNCES,
			.activate = pidgin_action_group_buddy_pounces,
		}, {
			.name = PIDGIN_ACTION_CUSTOM_SMILEY,
			.activate = pidgin_action_group_custom_smiley,
		}, {
			.name = PIDGIN_ACTION_DEBUG,
			.activate = pidgin_action_group_debug,
		}, {
			.name = PIDGIN_ACTION_FILE_TRANSFERS,
			.activate = pidgin_action_group_file_transfers,
		}, {
			.name = PIDGIN_ACTION_GET_USER_INFO,
			.activate = pidgin_action_group_get_user_info,
		}, {
			.name = PIDGIN_ACTION_JOIN_CHAT,
			.activate = pidgin_action_group_join_chat,
		}, {
			.name = PIDGIN_ACTION_MUTE_SOUNDS,
			.state = "false",
			.change_state = pidgin_action_group_mute_sounds,
		}, {
			.name = PIDGIN_ACTION_NEW_MESSAGE,
			.activate = pidgin_action_group_new_message,
		}, {
			.name = PIDGIN_ACTION_ONLINE_HELP,
			.activate = pidgin_action_group_online_help,
		}, {
			.name = PIDGIN_ACTION_PREFERENCES,
			.activate = pidgin_action_group_preferences,
		}, {
			.name = PIDGIN_ACTION_PRIVACY,
			.activate = pidgin_action_group_privacy,
		}, {
			.name = PIDGIN_ACTION_QUIT,
			.activate = pidgin_action_group_quit,
		}, {
			.name = PIDGIN_ACTION_ROOM_LIST,
			.activate = pidgin_action_group_room_list,
		}, {
			.name = PIDGIN_ACTION_SET_MOOD,
			.activate = pidgin_action_group_set_mood,
		}, {
			.name = PIDGIN_ACTION_SHOW_BUDDY_ICONS,
			.state = "false",
			.change_state = pidgin_action_group_show_buddy_icons,
		}, {
			.name = PIDGIN_ACTION_SHOW_EMPTY_GROUPS,
			.state = "false",
			.change_state = pidgin_action_group_show_empty_groups,
		}, {
			.name = PIDGIN_ACTION_SHOW_IDLE_TIMES,
			.state = "false",
			.change_state = pidgin_action_group_show_idle_times,
		}, {
			.name = PIDGIN_ACTION_SHOW_OFFLINE_BUDDIES,
			.state = "false",
			.change_state = pidgin_action_group_show_offline_buddies,
		}, {
			.name = PIDGIN_ACTION_SHOW_PROTOCOL_ICONS,
			.state = "false",
			.change_state = pidgin_action_group_show_protocol_icons,
		}, {
			.name = PIDGIN_ACTION_SORT_METHOD,
			.parameter_type = "s",
			.state = "'none'",
			.change_state = pidgin_action_group_sort_method,
		}, {
			.name = PIDGIN_ACTION_SYSTEM_LOG,
			.activate = pidgin_action_group_system_log,
		}, {
			.name = PIDGIN_ACTION_VIEW_USER_LOG,
			.activate = pidgin_action_group_view_user_log,
		},
	};

	g_action_map_add_action_entries(G_ACTION_MAP(group), entries,
	                                G_N_ELEMENTS(entries), NULL);

	/* now add some handlers for preference changes and set actions to the
	 * correct value.
	 */
	pidgin_action_group_setup_bool(group, PIDGIN_ACTION_MUTE_SOUNDS,
	                               PIDGIN_PREFS_ROOT "/sound/mute",
	                               pidgin_action_group_mute_sounds_callback);
	pidgin_action_group_setup_bool(group, PIDGIN_ACTION_SHOW_BUDDY_ICONS,
	                               PIDGIN_PREFS_ROOT "/blist/show_buddy_icons",
	                               pidgin_action_group_show_buddy_icons_callback);
	pidgin_action_group_setup_bool(group, PIDGIN_ACTION_SHOW_EMPTY_GROUPS,
	                               PIDGIN_PREFS_ROOT "/blist/show_empty_groups",
	                               pidgin_action_group_show_empty_groups_callback);
	pidgin_action_group_setup_bool(group, PIDGIN_ACTION_SHOW_IDLE_TIMES,
	                               PIDGIN_PREFS_ROOT "/blist/show_idle_time",
	                               pidgin_action_group_show_idle_times_callback);
	pidgin_action_group_setup_bool(group, PIDGIN_ACTION_SHOW_OFFLINE_BUDDIES,
	                               PIDGIN_PREFS_ROOT "/blist/show_offline_buddies",
	                               pidgin_action_group_show_offline_buddies_callback);
	pidgin_action_group_setup_bool(group, PIDGIN_ACTION_SHOW_PROTOCOL_ICONS,
	                               PIDGIN_PREFS_ROOT "/blist/show_protocol_icons",
	                               pidgin_action_group_show_protocol_icons_callback);

	pidgin_action_group_setup_string(group, PIDGIN_ACTION_SORT_METHOD,
	                                 PIDGIN_PREFS_ROOT "/blist/sort_type",
	                                 pidgin_action_group_sort_method_callback);

	/* assume we are offline and disable all of the actions that require us to
	 * be online.
	 */
	pidgin_action_group_online_actions_set_enable(group, FALSE);
	pidgin_action_group_chat_actions_set_enable(group, FALSE);
	pidgin_action_group_room_list_actions_set_enable(group, FALSE);

	/* connect to the online and offline signals in purple connections.  This
	 * is used to toggle states of actions that require being online.
	 */
	handle = purple_connections_get_handle();
	purple_signal_connect(handle, "online", group,
	                      PURPLE_CALLBACK(pidgin_action_group_online_cb),
	                      group);
	purple_signal_connect(handle, "offline", group,
	                      PURPLE_CALLBACK(pidgin_action_group_offline_cb),
	                      group);

	/* connect to account-signed-on and account-signed-off to toggle actions
	 * that depend on specific interfaces in accounts.
	 */
	handle = purple_accounts_get_handle();
	purple_signal_connect(handle, "account-signed-on", group,
	                      PURPLE_CALLBACK(pidgin_action_group_signed_on_cb),
	                      group);
	purple_signal_connect(handle, "account-signed-off", group,
	                      PURPLE_CALLBACK(pidgin_action_group_signed_off_cb),
	                      group);
};

static void
pidgin_action_group_finalize(GObject *obj) {
	purple_signals_disconnect_by_handle(obj);

	G_OBJECT_CLASS(pidgin_action_group_parent_class)->finalize(obj);
}

static void
pidgin_action_group_class_init(PidginActionGroupClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	obj_class->finalize = pidgin_action_group_finalize;
}

/******************************************************************************
 * Public API
 *****************************************************************************/
GSimpleActionGroup *
pidgin_action_group_new(void) {
	return G_SIMPLE_ACTION_GROUP(g_object_new(PIDGIN_TYPE_ACTION_GROUP, NULL));
}

