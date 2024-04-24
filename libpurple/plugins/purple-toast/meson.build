subdir_done()

library('purple-toast', 'purple-toast.c',
	c_args : ['-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="PurplePlugin-Toast"'],
	gnu_symbol_visibility : 'hidden',
	dependencies : [libpurple_dep],
	name_prefix: '',
	install : true,
	install_dir : PURPLE_PLUGINDIR)

devenv.append('PURPLE_PLUGIN_PATH', meson.current_build_dir())
