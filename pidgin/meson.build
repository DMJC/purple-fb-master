if not ENABLE_GTK
  subdir_done()
endif

libpidgin_SOURCES = [
	'gtkmedia.c',
	'gtknotify.c',
	'gtkrequest.c',
	'gtkutils.c',
	'gtkwhiteboard.c',
	'libpidgin.c',
	'pidginabout.c',
	'pidginaccountchooser.c',
	'pidginaccountdisplay.c',
	'pidginaccounteditor.c',
	'pidginaccountfilterconnected.c',
	'pidginaccountfilterprotocol.c',
	'pidginaccountmanager.c',
	'pidginaccountmanagerrow.c',
	'pidginaccountrow.c',
	'pidginaccountsdisabledmenu.c',
	'pidginaccountsenabledmenu.c',
	'pidginapplication.c',
	'pidginautoadjustment.c',
	'pidginavatar.c',
	'pidginchanneljoindialog.c',
	'pidgincolor.c',
	'pidgincontactinfomenu.c',
	'pidgincontactlist.c',
	'pidginconversation.c',
	'pidgindebug.c',
	'pidgindisplayitem.c',
	'pidgindisplaywindow.c',
	'pidginiconname.c',
	'pidgininfopane.c',
	'pidginkeypad.c',
	'pidginnotifiable.c',
	'pidginnotificationaddcontact.c',
	'pidginnotificationauthorizationrequest.c',
	'pidginnotificationconnectionerror.c',
	'pidginnotificationlist.c',
	'pidginpluginsmenu.c',
	'pidginpresenceicon.c',
	'pidginprotocolchooser.c',
	'pidgintextbuffer.c',
	'pidginui.c',
	'prefs/pidginprefs.c',
	'prefs/pidginawayprefs.c',
	'prefs/pidginconversationprefs.c',
	'prefs/pidgincredentialproviderrow.c',
	'prefs/pidgincredentialprefs.c',
	'prefs/pidginnetworkprefs.c',
	'prefs/pidginproxyprefs.c',
	'prefs/pidginvvprefs.c',
]

libpidgin_headers = [
	'gtkmedia.h',
	'gtknotify.h',
	'gtkrequest.h',
	'gtkutils.h',
	'gtkwhiteboard.h',
	'pidginabout.h',
	'pidginaccountchooser.h',
	'pidginaccountdisplay.h',
	'pidginaccounteditor.h',
	'pidginaccountfilterconnected.h',
	'pidginaccountfilterprotocol.h',
	'pidginaccountmanager.h',
	'pidginaccountmanagerrow.h',
	'pidginaccountrow.h',
	'pidginaccountsdisabledmenu.h',
	'pidginaccountsenabledmenu.h',
	'pidginapplication.h',
	'pidginautoadjustment.h',
	'pidginavatar.h',
	'pidginchanneljoindialog.h',
	'pidgincolor.h',
	'pidgincontactinfomenu.h',
	'pidgincontactlist.h',
	'pidginconversation.h',
	'pidgincore.h',
	'pidgindisplayitem.h',
	'pidgindisplaywindow.h',
	'pidgindebug.h',
	'pidginiconname.h',
	'pidgininfopane.h',
	'pidginkeypad.h',
	'pidginnotifiable.h',
	'pidginnotificationaddcontact.h',
	'pidginnotificationauthorizationrequest.h',
	'pidginnotificationconnectionerror.h',
	'pidginnotificationlist.h',
	'pidginpluginsmenu.h',
	'pidginpresenceicon.h',
	'pidginprotocolchooser.h',
	'pidgintextbuffer.h',
	'pidginui.h',
	'pidginversion.h',
]

libpidgin_prefs_headers = [
	'prefs/pidginprefs.h',
	'prefs/pidginawayprefs.h',
	'prefs/pidginconversationprefs.h',
	'prefs/pidgincredentialproviderrow.h',
	'prefs/pidgincredentialprefs.h',
	'prefs/pidginnetworkprefs.h',
	'prefs/pidginproxyprefs.h',
	'prefs/pidginvvprefs.h',
]

pidgin_SOURCES = [
	'pidgin.c'
]

libpidgin_built_sources = []

libpidgin_built_headers = []

pidgin_resource = gnome.compile_resources('pidginresources',
    'resources/pidgin.gresource.xml',
    source_dir : 'resources',
    c_name : 'pidgin')
libpidgin_built_sources += pidgin_resource

pidgin_filebase = 'pidgin-3'
pidgin_include_base = pidgin_filebase / 'pidgin'

_libpidgin_dependencies = [
	glib,
	gplugin_gtk_dep,
	gtk,
	IOKIT,
	json,
	libadwaita,
	math,
	libpurple_dep,
]

if IS_WIN32
	libpidgin_SOURCES += [
		'pidgin.c',
		'win32/gtkwin32dep.c',
	]

	pidgin_dll_rc = configure_file(
	    input : 'win32/pidgin_dll_rc.rc.in',
	    output : 'pidgin_dll_rc.rc',
	    configuration : version_conf)
	libpidgin_SOURCES += windows.compile_resources(pidgin_dll_rc,
	    include_directories : include_directories('win32')
	)

	pidgin_exe_rc = configure_file(
	    input : 'win32/pidgin_exe_rc.rc.in',
	    output : 'pidgin_exe_rc.rc',
	    configuration : version_conf)

	pidgin_WIN32_RESOURCES = windows.compile_resources(pidgin_exe_rc,
		include_directories : include_directories('win32'))

	pidgin_SOURCES = [
		'win32/winpidgin.c',
		pidgin_WIN32_RESOURCES,
	]

	_libpidgin_dependencies += [
		dependency('gmodule-2.0'),
	]
endif

PIDGIN_H_INCLUDES = []
foreach header : libpidgin_headers + libpidgin_prefs_headers + ['pidginversionconsts.h']
	PIDGIN_H_INCLUDES += f'#include <pidgin/@header@>'
endforeach
pidgin_h_conf = configuration_data()
pidgin_h_conf.set('PIDGIN_H_INCLUDES', '\n'.join(PIDGIN_H_INCLUDES))
pidgin_h = configure_file(input : 'pidgin.h.in',
                          output : 'pidgin.h',
                          configuration : pidgin_h_conf,
                          install : true,
                          install_dir : get_option('includedir') / pidgin_filebase)
pidgin_version_h = configure_file(input : 'pidginversionconsts.h.in',
                                  output : 'pidginversionconsts.h',
                                  configuration : version_conf,
                                  install : true,
                                  install_dir : get_option('includedir') / pidgin_filebase)

install_headers(libpidgin_headers, subdir : pidgin_include_base)
install_headers(libpidgin_prefs_headers, subdir : pidgin_include_base / 'prefs')

libpidgin_built_headers += [
	pidgin_h,
	pidgin_version_h,
]

libpidgin_inc = include_directories('.')
libpidgin = shared_library('pidgin3',
    libpidgin_SOURCES + libpidgin_built_sources + libpidgin_built_headers,
    package_revision,
    c_args : ['-DPIDGIN_COMPILATION', '-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Pidgin"'],
    gnu_symbol_visibility : 'hidden',
    include_directories : [toplevel_inc, include_directories('prefs')],
    version : PURPLE_LIB_VERSION,
    dependencies : _libpidgin_dependencies,
    install : true)
libpidgin_dep = declare_dependency(
    include_directories : [toplevel_inc, libpidgin_inc],
    link_with : libpidgin,
    sources : libpidgin_built_headers,
    dependencies : [gtk, glib, libadwaita, math, gplugin_gtk_dep])

executable('pidgin3',
    pidgin_SOURCES,
    c_args : ['-DPIDGIN_COMPILATION', '-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Pidgin"'],
    include_directories : toplevel_inc,
    dependencies : IS_WIN32 ? [] : [libpurple_dep, libpidgin_dep],
    win_subsystem : 'windows',
    install : true)

meson.override_dependency(pidgin_filebase, libpidgin_dep)

pkgconfig.generate(
    libpidgin,
    name : 'Pidgin',
    description : 'Pidgin is a GTK based instant messenger application.',
    version : meson.project_version(),
    filebase : pidgin_filebase,
    subdirs : pidgin_filebase,
    libraries : [gtk, libadwaita, libpurple_dep, gplugin_gtk_dep],
    variables : [f'plugindir=${libdir}/pidgin-@purple_major_version@'])

if enable_introspection
	introspection_sources = libpidgin_SOURCES + libpidgin_headers + libpidgin_prefs_headers

	pidgin_gir = gnome.generate_gir(libpidgin,
		sources : introspection_sources,
		header : 'pidgin.h',
		includes : ['GLib-2.0', 'GObject-2.0', 'Gtk-4.0', libpurple_gir[0]],
		namespace : 'Pidgin',
		symbol_prefix : 'pidgin',
		identifier_prefix : 'Pidgin',
		export_packages : f'pidgin-@purple_major_version@',
		nsversion : f'@purple_major_version@.@purple_minor_version@',
		dependencies : [gplugin_dep, gplugin_gtk_dep, libpurple_dep],
		install : true,
		extra_args : ['-DPIDGIN_COMPILATION', '--quiet'])
endif

subdir('data')
subdir('plugins')
subdir('tests')
