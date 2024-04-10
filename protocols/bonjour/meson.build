BONJOUR_SOURCES = [
	'purplebonjourconnection.c',
	'purplebonjourcore.c',
	'purplebonjourprotocol.c',
]

BONJOUR_HEADERS = [
	'purplebonjourconnection.h',
	'purplebonjourconstants.h',
	'purplebonjourcore.h',
	'purplebonjourprotocol.h',
]

bonjour_resources = gnome.compile_resources('bonjourresource',
	'resources/bonjour.gresource.xml',
	source_dir : 'resources',
	c_name : 'purple_bonjour')
BONJOUR_SOURCES += bonjour_resources

bonjour_prpl = library('bonjournouveau',
	BONJOUR_SOURCES, BONJOUR_HEADERS,
	c_args : ['-DPURPLE_BONJOUR_COMPILATION', '-DG_LOG_USE_STRUCTURED',
	          '-DG_LOG_DOMAIN="Purple-Bonjour"'],
	gnu_symbol_visibility : 'hidden',
	dependencies : [libpurple_dep, glib, gio],
	install : true,
	install_dir : PURPLE_PLUGINDIR)

devenv.append('PURPLE_PLUGIN_PATH', meson.current_build_dir())
