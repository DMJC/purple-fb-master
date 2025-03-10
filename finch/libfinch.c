/*
 * finch
 *
 * Finch is the legal property of its developers, whose names are too numerous
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

#include <internal.h>
#include <locale.h>
#include "finch.h"

#include <purple.h>

#include "gntdebug.h"
#include "gntprefs.h"
#include "gntui.h"
#include "gntidle.h"

#include "config.h"
#include "package_revision.h"

static void
debug_init(void)
{
	FinchDebugUi *ui = finch_debug_ui_new();
	purple_debug_set_ui(PURPLE_DEBUG_UI(ui));
}

static PurpleUiInfo *
finch_ui_get_info(void) {
	return purple_ui_info_new(_("Finch"), VERSION, "https://pidgin.im",
	                          "https://developer.pidgin.im", "console");
}

static void
finch_quit(void)
{
	finch_ui_uninit();
}

static PurpleCoreUiOps core_ops =
{
	finch_prefs_init,
	debug_init,
	finch_ui_init,
	finch_quit,
	finch_ui_get_info,
};

static PurpleCoreUiOps *
gnt_core_get_ui_ops(void)
{
	return &core_ops;
}

static int
init_libpurple(int argc, char **argv)
{
	char *path;
	gboolean opt_nologin = FALSE;
	gboolean opt_version = FALSE;
	char *opt_config_dir_arg = NULL;
	gboolean debug_enabled = FALSE;
	GOptionContext *context;
	gchar **args;
	GError *error = NULL;

	GOptionEntry option_entries[] = {
		{"config", 'c', 0,
			G_OPTION_ARG_FILENAME, &opt_config_dir_arg,
			_("use DIR for config files"), _("DIR")},
		{"nologin", 'n', 0,
			G_OPTION_ARG_NONE, &opt_nologin,
			_("don't automatically login"), NULL},
		{"version", 'v', 0,
			G_OPTION_ARG_NONE, &opt_version,
			_("display the current version and exit"), NULL},
		{NULL}
	};

	bindtextdomain(PACKAGE, PURPLE_LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);

	setlocale(LC_ALL, "");

	context = g_option_context_new(NULL);
	g_option_context_set_summary(context, DISPLAY_VERSION);
	g_option_context_add_main_entries(context, option_entries, PACKAGE);

	g_option_context_add_group(context, purple_get_option_group());
	g_option_context_add_group(context, gplugin_get_option_group());

#ifdef G_OS_WIN32
	/* Handle Unicode filenames on Windows. See GOptionContext docs. */
	args = g_win32_get_command_line();
#else
	args = g_strdupv(argv);
#endif

	if (!g_option_context_parse_strv(context, &args, &error)) {
		g_strfreev(args);
		g_printerr(_("%s: %s\nTry `%s -h' for more information.\n"),
				DISPLAY_VERSION, error->message, argv[0]);
		g_clear_error(&error);
		return 1;
	}

	g_strfreev(args);

	/* show version message */
	if (opt_version) {
		/* Translators may want to transliterate the name.
		 It is not to be translated. */
		printf("%s %s (%s)\n", _("Finch"), DISPLAY_VERSION, REVISION);
		return 0;
	}

	/* set a user-specified config directory */
	if (opt_config_dir_arg != NULL) {
		if (g_path_is_absolute(opt_config_dir_arg)) {
			purple_util_set_user_dir(opt_config_dir_arg);
		} else {
			/* Make an absolute (if not canonical) path */
			char *cwd = g_get_current_dir();
			char *path = g_build_path(G_DIR_SEPARATOR_S, cwd, opt_config_dir_arg, NULL);
			purple_util_set_user_dir(path);
			g_free(path);
			g_free(cwd);
		}

		g_free(opt_config_dir_arg);
	}

	/*
	 * We're done piddling around with command line arguments.
	 * Fire up this baby.
	 */

	/* We don't want debug-messages to show up and corrupt the display */
	purple_debug_set_enabled(debug_enabled);

	purple_core_set_ui_ops(gnt_core_get_ui_ops());
	purple_idle_set_ui_ops(finch_idle_get_ui_ops());

	if (!purple_core_init(FINCH_UI))
	{
		fprintf(stderr,
				"Initialization of the Purple core failed. Dumping core.\n"
				"Please report this!\n");
		abort();
	}

	path = g_build_filename(purple_data_dir(), "plugins", NULL);
	if (g_mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR) != 0 && errno != EEXIST)
		fprintf(stderr, "Couldn't create plugins dir\n");
	purple_plugins_add_search_path(path);
	g_free(path);

	purple_plugins_add_search_path(FINCH_LIBDIR);
	purple_plugins_refresh();

	/* TODO: should this be moved into finch_prefs_init() ? */
	finch_prefs_update_old();

	/* load plugins we had when we quit */
	purple_plugins_load_saved("/finch/plugins/loaded");

	if (opt_nologin)
	{
		/* Set all accounts to "offline" */
		PurpleSavedStatus *saved_status;

		/* If we've used this type+message before, lookup the transient status */
		saved_status = purple_savedstatus_find_transient_by_type_and_message(
							PURPLE_STATUS_OFFLINE, NULL);

		/* If this type+message is unique then create a new transient saved status */
		if (saved_status == NULL)
			saved_status = purple_savedstatus_new(NULL, PURPLE_STATUS_OFFLINE);

		/* Set the status for each account */
		purple_savedstatus_activate(saved_status);
	}
	else
	{
		/* Everything is good to go--sign on already */
		if (!purple_prefs_get_bool("/purple/savedstatus/startup_current_status"))
			purple_savedstatus_activate(purple_savedstatus_get_startup());
		purple_accounts_restore_current_statuses();
	}

	return 1;
}

gboolean finch_start(int *argc, char ***argv)
{
	/* Initialize the libpurple stuff */
	if (!init_libpurple(*argc, *argv))
		return FALSE;

	purple_blist_show();
	return TRUE;
}
