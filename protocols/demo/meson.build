DEMO_SOURCES = [
	'purpledemoconnection.c',
	'purpledemoconnection.h',
	'purpledemocontacts.c',
	'purpledemocontacts.h',
	'purpledemoplugin.c',
	'purpledemoplugin.h',
	'purpledemoprotocol.c',
	'purpledemoprotocol.h',
	'purpledemoprotocolactions.c',
	'purpledemoprotocolactions.h',
	'purpledemoprotocolclient.c',
	'purpledemoprotocolclient.h',
	'purpledemoprotocolcontacts.c',
	'purpledemoprotocolcontacts.h',
	'purpledemoprotocolconversation.c',
	'purpledemoprotocolconversation.h',
]

if DYNAMIC_DEMO
	demo_resources = gnome.compile_resources('purpledemoresource',
		'resources/purpledemo.gresource.xml',
		source_dir : 'resources',
		c_name : 'purple_demo')
	DEMO_SOURCES += demo_resources

	shared_library('demo', DEMO_SOURCES,
		c_args : ['-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Purple-Demo"'],
		gnu_symbol_visibility : 'hidden',
		dependencies : [glib, json, libpurple_dep],
		install : true,
		install_dir : PURPLE_PLUGINDIR)

	devenv.append('PURPLE_PLUGIN_PATH', meson.current_build_dir())
endif
