if IS_WIN32
	subdir_done()
endif

GTK4_X11 = dependency('gtk4-x11', required : false)

if GTK4_X11.found()
	shared_module('idle-xscreensaver', 'idle-xscreensaver.c',
	    c_args : ['-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Purple-IdleXScreenSaver"'],
	    gnu_symbol_visibility : 'hidden',
	    dependencies : [libpurple_dep, libpidgin_dep, GTK4_X11],
	    name_prefix : '',
	    install : true, install_dir : PIDGIN_PLUGINDIR)

	devenv.append('PIDGIN_PLUGIN_PATH', meson.current_build_dir())
endif
