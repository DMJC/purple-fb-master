XMPP_SOURCES = [
	'purplexmppconnection.c',
	'purplexmppcore.c',
	'purplexmppprotocol.c',
]

XMPP_HEADERS = [
	'purplexmppconnection.h',
	'purplexmppconstants.h',
	'purplexmppcore.h',
	'purplexmppprotocol.h',
]

if not DYNAMIC_XMPP
	subdir_done()
endif

xmpp_resources = gnome.compile_resources('xmppresource',
	'resources/xmpp.gresource.xml',
	source_dir : 'resources',
	c_name : 'purple_xmpp')
XMPP_SOURCES += xmpp_resources

xmpp_prpl = library('xmpp',
	XMPP_SOURCES + XMPP_HEADERS,
	c_args : ['-DPURPLE_XMPP_COMPILATION', '-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Purple-XMPP"'],
	gnu_symbol_visibility : 'hidden',
	dependencies : [libpurple_dep, glib, gio, hasl, xeme],
	install : true,
	install_dir : PURPLE_PLUGINDIR)

devenv.append('PURPLE_PLUGIN_PATH', meson.current_build_dir())
