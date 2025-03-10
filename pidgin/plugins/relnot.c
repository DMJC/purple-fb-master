/*
 * Release Notification Plugin
 *
 * Copyright (C) 2003, Nathan Walp <faceprint@faceprint.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "internal.h"

#include <libsoup/soup.h>
#include <string.h>

#include <purple.h>

#include "gtkblist.h"
#include "gtkutils.h"
#include "pidginicon.h"
#include "pidginplugininfo.h"

#include "pidgin.h"

static SoupSession *session = NULL;

/* 1 day */
#define MIN_CHECK_INTERVAL 60 * 60 * 24

static void
release_hide()
{
	/* No-op.  We may use this method in the future to avoid showing
	 * the popup twice */
}

static void
release_show()
{
	purple_notify_uri(NULL, PURPLE_WEBSITE);
}

static void
version_fetch_cb(G_GNUC_UNUSED SoupSession *session, SoupMessage *msg,
                 gpointer user_data)
{
	gchar *cur_ver;
	const char *changelog;
	GtkWidget *release_dialog;

	GString *message;
	int i = 0;

	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		return;
	}

	changelog = msg->response_body->data;

	while(changelog[i] && changelog[i] != '\n') i++;

	/* this basically means the version thing wasn't in the format we were
	 * looking for so sourceforge is probably having web server issues, and
	 * we should try again later */
	if(i == 0)
		return;

	cur_ver = g_strndup(changelog, i);

	message = g_string_new("");
	g_string_append_printf(message, _("You can upgrade to %s %s today."),
			PIDGIN_NAME, cur_ver);

	release_dialog = pidgin_make_mini_dialog(
		NULL, "dialog-information",
		_("New Version Available"),
		message->str,
		NULL,
		_("Later"), PURPLE_CALLBACK(release_hide),
		_("Download Now"), PURPLE_CALLBACK(release_show),
		NULL);

	pidgin_blist_add_alert(release_dialog);

	g_string_free(message, TRUE);
	g_free(cur_ver);
}

static void
do_check(void)
{
	int last_check = purple_prefs_get_int("/plugins/gtk/relnot/last_check");
	if(!last_check || time(NULL) - last_check > MIN_CHECK_INTERVAL) {
		SoupMessage *msg;

		purple_debug_info("relnot", "Checking for new version.");

		msg = soup_form_request_new("GET", "https://pidgin.im/version.php",
		                            "version", purple_core_get_version(),
		                            "build",
#ifdef _WIN32
		                            "purple-win32",
#else
		                            "purple",
#endif
		                            NULL);

		soup_session_queue_message(session, msg, version_fetch_cb, NULL);

		purple_prefs_set_int("/plugins/gtk/relnot/last_check", time(NULL));
	}
}

static void
signed_on_cb(PurpleConnection *gc, void *data) {
	do_check();
}

/**************************************************************************
 * Plugin stuff
 **************************************************************************/
static PidginPluginInfo *
plugin_query(GError **error)
{
	const gchar * const authors[] = {
		"Nathan Walp <faceprint@faceprint.com>",
		NULL
	};

	return pidgin_plugin_info_new(
		"id",           "gtk-relnot",
		"name",         N_("Release Notification"),
		"version",      DISPLAY_VERSION,
		"category",     N_("Notification"),
		"summary",      N_("Checks periodically for new releases."),
		"description",  N_("Checks periodically for new releases and notifies "
		                   "the user with the ChangeLog."),
		"authors",      authors,
		"website",      PURPLE_WEBSITE,
		"abi-version",  PURPLE_ABI_VERSION,
		NULL
	);
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
	purple_prefs_add_none("/plugins/gtk/relnot");
	purple_prefs_add_int("/plugins/gtk/relnot/last_check", 0);

	purple_signal_connect(purple_connections_get_handle(), "signed-on",
						plugin, PURPLE_CALLBACK(signed_on_cb), NULL);

	session = soup_session_new();

	/* we don't check if we're offline */
	if(purple_connections_get_all())
		do_check();

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
	soup_session_abort(session);
	g_clear_object(&session);
	return TRUE;
}

PURPLE_PLUGIN_INIT(relnot, plugin_query, plugin_load, plugin_unload);
