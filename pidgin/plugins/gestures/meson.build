gestures_SOURCES = [
	'gestures.c',
	'gstroke.h',
	'gstroke-internal.h',
	'stroke.c',
	'stroke-draw.c'
]

library('gestures', gestures_SOURCES,
    c_args : ['-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="PidginPlugin-Gestures"'],
    gnu_symbol_visibility : 'hidden',
    dependencies : [libpurple_dep, libpidgin_dep, glib],
    name_prefix : '',
    build_by_default : false,  # FIXME: Port to GTK4
    install : false, install_dir : PIDGIN_PLUGINDIR)

settings_schemas = [
  'im.pidgin.Pidgin.plugin.Gestures.gschema.xml',
]

install_data(settings_schemas, install_dir: schemas_dir)
gnome.post_install(glib_compile_schemas: true)

# Compile the schemas in the current directory; this is only useful for testing
gnome.compile_schemas(depend_files: files(settings_schemas))

devenv.append('PIDGIN_PLUGIN_PATH', meson.current_build_dir())
