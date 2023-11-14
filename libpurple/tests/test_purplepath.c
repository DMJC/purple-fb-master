#include <glib.h>

#include <purple.h>

#include "test_ui.h"

static void
test_purplepath_cache_dir(void) {
	gchar *cache_dir;

	cache_dir = g_build_filename(g_get_user_cache_dir(), "test", NULL);
	g_assert_cmpstr(cache_dir, ==, purple_cache_dir());
	g_free(cache_dir);
}

static void
test_purplepath_config_dir(void) {
	gchar *config_dir;

	config_dir = g_build_filename(g_get_user_config_dir(), "test", NULL);
	g_assert_cmpstr(config_dir, ==, purple_config_dir());
	g_free(config_dir);
}

static void
test_purplepath_data_dir(void) {
	gchar *data_dir;

	data_dir = g_build_filename(g_get_user_data_dir(), "test", NULL);
	g_assert_cmpstr(data_dir, ==, purple_data_dir());
	g_free(data_dir);
}

gint
main(gint argc, gchar **argv) {
	gint ret = 0;

	g_test_init(&argc, &argv, NULL);

	test_ui_purple_init();

	g_test_add_func("/purplepath/cachedir",
	                test_purplepath_cache_dir);
	g_test_add_func("/purplepath/configdir",
	                test_purplepath_config_dir);
	g_test_add_func("/purplepath/datadir",
	                test_purplepath_data_dir);

	ret = g_test_run();

	test_ui_purple_uninit();

	return ret;
}
