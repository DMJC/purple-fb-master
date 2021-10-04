/*
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

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

#define STATENOTIFY_PLUGIN_ID "core-statenotify"

static void
write_status(PurpleBuddy *buddy, const char *message)
{
	PurpleAccount *account = NULL;
	PurpleConversation *im;
	PurpleConversationManager *manager;
	const char *who;
	char buf[256];
	char *escaped;
	const gchar *buddy_name = NULL;

	account = purple_buddy_get_account(buddy);
	buddy_name = purple_buddy_get_name(buddy);

	manager = purple_conversation_manager_get_default();
	im = purple_conversation_manager_find_im(manager, account, buddy_name);

	if (im == NULL)
		return;

	/* Prevent duplicate notifications for buddies in multiple groups */
	if (buddy != purple_blist_find_buddy(account, buddy_name))
		return;

	who = purple_buddy_get_alias(buddy);
	escaped = g_markup_escape_text(who, -1);

	g_snprintf(buf, sizeof(buf), message, escaped);
	g_free(escaped);

	purple_conversation_write_system_message(im, buf,
		PURPLE_MESSAGE_ACTIVE_ONLY | PURPLE_MESSAGE_NO_LINKIFY);
}

static void
buddy_status_changed_cb(PurpleBuddy *buddy, PurpleStatus *old_status,
                        PurpleStatus *status, void *data)
{
	gboolean available, old_available;

	if (!purple_status_is_exclusive(status) ||
			!purple_status_is_exclusive(old_status))
		return;

	available = purple_status_is_available(status);
	old_available = purple_status_is_available(old_status);

	if (purple_prefs_get_bool("/plugins/core/statenotify/notify_away")) {
		if (available && !old_available)
			write_status(buddy, _("%s is no longer away."));
		else if (!available && old_available)
			write_status(buddy, _("%s has gone away."));
	}
}

static void
buddy_idle_changed_cb(PurpleBuddy *buddy, gboolean old_idle, gboolean idle,
                      void *data)
{
	if (purple_prefs_get_bool("/plugins/core/statenotify/notify_idle")) {
		if (idle && !old_idle) {
			write_status(buddy, _("%s has become idle."));
		} else if (!idle && old_idle) {
			write_status(buddy, _("%s is no longer idle."));
		}
	}
}

static void
buddy_signon_cb(PurpleBuddy *buddy, void *data)
{
	if (purple_prefs_get_bool("/plugins/core/statenotify/notify_signon"))
		write_status(buddy, _("%s has signed on."));
}

static void
buddy_signoff_cb(PurpleBuddy *buddy, void *data)
{
	if (purple_prefs_get_bool("/plugins/core/statenotify/notify_signon"))
		write_status(buddy, _("%s has signed off."));
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_label(_("Notify When"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/statenotify/notify_away", _("Buddy Goes _Away"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/statenotify/notify_idle", _("Buddy Goes _Idle"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/statenotify/notify_signon", _("Buddy _Signs On/Off"));
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static GPluginPluginInfo *
state_notify_query(GError **error)
{
	const gchar * const authors[] = {
		"Christian Hammond <chipx86@gnupdate.org>",
		NULL
	};

	return purple_plugin_info_new(
		"id",             STATENOTIFY_PLUGIN_ID,
		"name",           N_("Buddy State Notification"),
		"version",        DISPLAY_VERSION,
		"category",       N_("Notification"),
		"summary",        N_("Notifies in a conversation window when a "
		                     "buddy goes or returns from away or idle."),
		"description",    N_("Notifies in a conversation window when a "
		                     "buddy goes or returns from away or idle."),
		"authors",        authors,
		"website",        PURPLE_WEBSITE,
		"abi-version",    PURPLE_ABI_VERSION,
		"pref-frame-cb",  get_plugin_pref_frame,
		NULL
	);
}

static gboolean
state_notify_load(GPluginPlugin *plugin, GError **error)
{
	void *blist_handle = purple_blist_get_handle();

	purple_prefs_add_none("/plugins/core/statenotify");
	purple_prefs_add_bool("/plugins/core/statenotify/notify_away", TRUE);
	purple_prefs_add_bool("/plugins/core/statenotify/notify_idle", TRUE);
	purple_prefs_add_bool("/plugins/core/statenotify/notify_signon", TRUE);

	purple_signal_connect(blist_handle, "buddy-status-changed", plugin,
	                    PURPLE_CALLBACK(buddy_status_changed_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-idle-changed", plugin,
	                    PURPLE_CALLBACK(buddy_idle_changed_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-signed-on", plugin,
	                    PURPLE_CALLBACK(buddy_signon_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-signed-off", plugin,
	                    PURPLE_CALLBACK(buddy_signoff_cb), NULL);

	return TRUE;
}

static gboolean
state_notify_unload(GPluginPlugin *plugin, gboolean shutdown, GError **error)
{
	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(state_notify)
