#include <purple.h>

static GPluginPluginInfo *
hello_world_query(GError **error) {
	const gchar * const authors[] = {
		"Author Name <aname@example.com>",
		NULL
	};

	return purple_plugin_info_new (
		"id", "hello_world",
		"name", "Hello World!",
		"version", "1.0",
		"category", "Example",
		"summary", "Hello World Plugin",
		"description", "Creates a \"Hello World!\" notification when loaded",
		"authors", authors,
		"website", "http://helloworld.tld",
		"abi-version", PURPLE_ABI_VERSION,
		NULL
	);
}

static gboolean
hello_world_load(GPluginPlugin *plugin, GError **error) {
	PurpleNotification *notification = NULL;
	PurpleNotificationManager *manager = NULL;

	notification = purple_notification_new_generic(NULL, "Hello World!");
	purple_notification_set_subtitle(notification,
	                                 "This is the Hello World! plugin :)");

	manager = purple_notification_manager_get_default();
	purple_notification_manager_add(manager, notification);
	g_clear_object(&notification);

	return TRUE;
}

static gboolean
hello_world_unload(GPluginPlugin *plugin, gboolean shutdown, GError **error) {
	return TRUE;
}

GPLUGIN_NATIVE_PLUGIN_DECLARE(hello_world)
