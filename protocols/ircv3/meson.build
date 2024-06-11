IRCV3_SOURCES = [
	'purpleircv3connection.c',
	'purpleircv3core.c',
	'purpleircv3messagehandlers.c',
	'purpleircv3protocol.c',
	'purpleircv3protocolconversation.c',
]

IRCV3_HEADERS = [
	'purpleircv3connection.h',
	'purpleircv3core.h',
	'purpleircv3messagehandlers.h',
	'purpleircv3protocol.h',
	'purpleircv3protocolconversation.h',
]

if not DYNAMIC_IRCV3
	subdir_done()
endif

ibis_dep = dependency('ibis', version : '>= 0.2.0')

ircv3_filebase = f'purple-@purple_major_version@-ircv3'

ircv3_includes = include_directories('.')
ircv3_include_base = purple_include_base / 'protocols/ircv3'


ircv3_resources = gnome.compile_resources('ircv3resource',
	'resources/ircv3.gresource.xml',
	source_dir : 'resources',
	c_name : 'purple_ircv3')
IRCV3_SOURCES += ircv3_resources

shared_library('ircv3', IRCV3_SOURCES + IRCV3_HEADERS,
	c_args : ['-DPURPLE_IRCV3_COMPILATION', '-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Purple-IRCv3"'],
	gnu_symbol_visibility : 'hidden',
	dependencies : [birb_dep, libpurple_dep, glib, gio, hasl, ibis_dep],
	install : true,
	install_dir : PURPLE_PLUGINDIR)

devenv.append('PURPLE_PLUGIN_PATH', meson.current_build_dir())
