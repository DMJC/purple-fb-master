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

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <gdk/x11/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gplugin.h>
#include <gplugin-native.h>

#include <purple.h>

#include <pidgin.h>

#define PIDGIN_IDLE_XSCREENSAVER_DOMAIN (g_quark_from_static_string("idle-xscreensaver"))
#define PIDGIN_IDLE_XSCREENSAVER_IDLE_SOURCE ("pidgin-xscreensaver")

/******************************************************************************
 * Globals
 *****************************************************************************/
static GDateTime *idle_at = NULL;

static Atom XA_BLANK = 0;
static Atom XA_SCREENSAVER_STATUS = 0;

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static gboolean
pidgin_idle_xscreensaver_xevent_cb(GdkX11Display *gdk_display, gpointer xevent,
                                   G_GNUC_UNUSED gpointer data)
{
	XEvent *event = NULL;

	if(xevent == NULL) {
		return FALSE;
	}

	event = xevent;

	if(event->type == PropertyNotify &&
	   event->xproperty.state == PropertyNewValue &&
	   event->xproperty.atom == XA_SCREENSAVER_STATUS)
	{
		Atom type;
		Display *x11_display = NULL;
		Window root_window;
		int result = 0;
		int format = 0;
		gulong n_items = 0;
		gulong bytes_after = 0;
		guchar *x_data_ptr = NULL;

		x11_display = gdk_x11_display_get_xdisplay(gdk_display);
		root_window = RootWindow(x11_display, 0);

		/* We don't use format or bytes_after, but if you pass NULL we
		 * segfault, so we need them to avoid that.
		 */
		result = XGetWindowProperty(x11_display, root_window,
		                            XA_SCREENSAVER_STATUS, 0, 999, False,
		                            XA_INTEGER, &type, &format, &n_items,
		                            &bytes_after, &x_data_ptr);
		if(result == Success && type != 0 && x_data_ptr != NULL) {
			gulong *x_data = (gulong *)x_data_ptr;
			gboolean need_to_set = FALSE;
			const char *state = NULL;

			if(type != XA_INTEGER || n_items < 3) {
				g_clear_pointer(&x_data, XFree);

				g_message("malformed status, ignoring");

				return FALSE;
			}

			if(x_data[0] == XA_BLANK && idle_at == NULL) {
				state = "blank";
				idle_at = g_date_time_new_from_unix_local((gint64)x_data[1]);
				need_to_set = TRUE;
			} else if(x_data[0] == 0) {
				need_to_set = TRUE;
				state = "unblank";
				g_clear_pointer(&idle_at, g_date_time_unref);
			}

			if(need_to_set) {
				PurpleIdleManager *manager = NULL;

				manager = purple_idle_manager_get_default();
				purple_idle_manager_set_source(manager,
				                               PIDGIN_IDLE_XSCREENSAVER_IDLE_SOURCE,
				                               idle_at);
			}

			if(state != NULL) {
				g_message("state changed to %s", state);
			}

			g_clear_pointer(&x_data, XFree);
		}
	}

	return FALSE;
}

/******************************************************************************
 * Plugin Exports
 *****************************************************************************/
static GPluginPluginInfo *
pidgin_idle_xscreensaver_query(G_GNUC_UNUSED GError **error) {
	const gchar * const authors[] = {
		"Pidgin Developers <devel@pidgin.im>",
		NULL
	};
	const char *description = N_("Reads the idle status from XScreenSaver. "
		"If you aren't using XScreenSaver directly this will not do anything "
		"useful for you.");

	return purple_plugin_info_new(
		"id", "pidgin-idle-xscreensaver",
		"abi-version", PURPLE_ABI_VERSION,
		"name", N_("XScreenSaver idle reporting"),
		"version", DISPLAY_VERSION,
		"category", N_("Presence"),
		"summary", N_("Read idle state from XScreenSaver"),
		"description", description,
		"authors", authors,
		"website", PURPLE_WEBSITE,
		NULL);
}

static gboolean
pidgin_idle_xscreensaver_load(GPluginPlugin *plugin, GError **error) {
	PurpleIdleManager *manager = NULL;
	GdkDisplay *gdk_display = NULL;
	Display *x11_display = NULL;
	Window root_window;
	XWindowAttributes attributes;

	/* Remove any previous idle source we had to make sure we're in a known
	 * state.
	 */
	manager = purple_idle_manager_get_default();
	purple_idle_manager_set_source(manager,
	                               PIDGIN_IDLE_XSCREENSAVER_IDLE_SOURCE,
	                               NULL);

	gdk_display = gdk_display_get_default();
	if(gdk_display == NULL) {
		g_set_error_literal(error, PIDGIN_IDLE_XSCREENSAVER_DOMAIN, 0,
		                    _("failed to find display"));

		return FALSE;
	}

	if(!GDK_IS_X11_DISPLAY(gdk_display)) {
		g_set_error_literal(error, PIDGIN_IDLE_XSCREENSAVER_DOMAIN, 0,
		                    _("display is not an x11 display"));

		return FALSE;
	}

	/* Set our input mask. */
	x11_display = gdk_x11_display_get_xdisplay(gdk_display);
	root_window = RootWindow(x11_display, 0);
	XGetWindowAttributes(x11_display, root_window, &attributes);
	XSelectInput(x11_display, root_window,
	             attributes.your_event_mask | PropertyChangeMask);

	/* Grab our atoms. */
	XA_BLANK = gdk_x11_get_xatom_by_name_for_display(GDK_X11_DISPLAY(gdk_display),
	                                                 "BLANK");
	XA_SCREENSAVER_STATUS = gdk_x11_get_xatom_by_name_for_display(GDK_X11_DISPLAY(gdk_display),
	                                                              "_SCREENSAVER_STATUS");

	/* Connect our signal. */
	g_signal_connect_object(gdk_display, "xevent",
	                        G_CALLBACK(pidgin_idle_xscreensaver_xevent_cb),
	                        plugin, 0);

	return TRUE;
}

static gboolean
pidgin_idle_xscreensaver_unload(GPluginPlugin *plugin,
                                G_GNUC_UNUSED gboolean shutdown,
                                G_GNUC_UNUSED GError **error)
{
	GdkDisplay *display = NULL;

	g_clear_pointer(&idle_at, g_date_time_unref);

	display = gdk_display_get_default();
	g_signal_handlers_disconnect_by_func(display,
	                                     pidgin_idle_xscreensaver_xevent_cb,
	                                     plugin);

	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(pidgin_idle_xscreensaver)
