Title: C Plugin Tutorial
Slug: c-plugin-tutorial

C plugins are native plugins. They have complete access to all of the API,
and can do basically whatever they want.

# Getting Started

To develop a plugin you need to have the libpurple source code or development
headers for your platform. It is generally a good idea to compile against the
same version of libpurple that you are running. You may also want to develop
against the code in our Mercurial repository if you need to use a new feature.

# An Example

I know every tutorial has a hello world, so why should libpurple be any
different?

```c
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
```

Okay, so what does all this mean? We start off by including `purple.h`. This
file includes all the libpurple header files.

`hello_world_query`, `hello_world_load` and `hello_world_unload` are functions
that must be implemented in every plugin. These functions are named according
to the value passed to `GPLUGIN_NATIVE_PLUGIN_DECLARE` which is described
below.

`hello_world_query` is called when the plugin is queried by the plugin system,
and returns various information about the plugin in form of a newly created
instance of [class@GPlugin.PluginInfo] or a subclass. For a list of all
available properties, see [ctor@PluginInfo.new].

If anything should go wrong during the query you can return an error by using
[func@GLib.set_error] on the `error` argument.

`hello_world_load` is called when the plugin is loaded. That is the user has
enabled the plugin or libpurple is loading a plugin that was previously loaded.
You can initialize any variables, register dynamic types, and so on in this
function. In this plugin we'll just use it to create and display a
notification. Just like `hello_world_query` if there are any errors that arise,
you can call [func@GLib.set_error] on the `error` argument and return `FALSE`.

`hello_world_unload` is called when the plugin is unloaded. That is, when the
user has manually unloaded the plugin or the program is shutting down. We can
use it to wrap up everything, and free our variables. If the program is
shutting down, the `shutdown` argument will be `TRUE`. Again, if there are any
errors, you can call [func@GLib.set_error] on the `error` argument and return
`FALSE`.

Finally we have `GPLUGIN_NATIVE_PLUGIN_DECLARE()`. It is a helper macro that
makes creating plugins easier. It has a single argument which is the prefix
used for the `_query`, `_load`, and `_unload` functions.

# Building

You may want to compile your plugin by using your libpurple from the packages
on your system. You can easily do this with meson build system. Let's say
you've already wrote the hello world plugin and saved it as `hello_world.c`.
Create a file named `meson.build`:

```python
project('hello_world_plugin', 'c')

libpurple_dep = dependency('purple-3')

PURPLE_PLUGINDIR = libpurple_dep.get_pkgconfig_variable('plugindir')

library('libhelloworld', 'hello_world.c',
	c_args : [
		'-DG_LOG_USE_STRUCTURED',
		'-DG_LOG_DOMAIN="PurplePlugin-HelloWorld"'
	],
	dependencies : [libpurple_dep],
	name_prefix : '',
	install : true,
	install_dir : PURPLE_PLUGINDIR)
```

This `meson.build` file will build your plugin and install it into the right
location for libpurple to find it.

If you are using this example to compile your own plugin. You should change the
project name at the beginning of the file as well as the library name which in
this case is `libhelloworld`. You will also need to change the `G_LOG_DOMAIN`
macro to your own log domain.

Now, you can quickly build your plugin with the following commands:

```
meson setup build
meson compile -C build
meson install -C build
```
