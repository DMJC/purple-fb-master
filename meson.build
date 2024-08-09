# UPDATING VERSION NUMBERS FOR RELEASES
#
# The version number is:
#   <major>.<minor>.<micro><extra>
#
# micro += 1
#
# If any functions have been added to libpurple or Pidgin:
#   micro = 0
#   minor += 1
#
# If backwards compatibility has been broken in libpurple or Pidgin:
#   micro = 0
#   minor = 0
#   major += 1
#   purple_soversion += 1
#
# extra should be similar to one of the following:
#   For beta releases:          '-beta2'
#   For code under development: '-devel'
#   For production releases:    ''
#
project('pidgin', 'c',
    version : '3.0.0-devel',
    meson_version : '>=1.0.0',
    default_options : ['c_std=c17', 'warning_level=2'])
purple_soversion = 0

parts = meson.project_version().split('-')
if parts.length() > 1
  purple_extra_version = parts[1]
else
  purple_extra_version = ''
endif

parts = parts[0].split('.')
purple_major_version = parts[0]
purple_minor_version = parts[1]
purple_micro_version = parts[2]

GETTEXT_PACKAGE='pidgin3'
find_program('gettext')
find_program('xgettext')

add_project_arguments([
	'-DVERSION="@0@"'.format(meson.project_version()),
	'-DDISPLAY_VERSION="@0@"'.format(meson.project_version()),
	'-DPURPLE_WEBSITE="https://pidgin.im/"',
	f'-DGETTEXT_PACKAGE="@GETTEXT_PACKAGE@"'],
	language : 'c')
conf = configuration_data()
man_conf = configuration_data()
version_conf = configuration_data()

conf.set_quoted('GETTEXT_PACKAGE', GETTEXT_PACKAGE)
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('DISPLAY_VERSION', meson.project_version())

conf.set_quoted('PURPLE_BUILD_CPU', host_machine.cpu())
conf.set_quoted('PURPLE_BUILD_ARCHITECTURE', host_machine.cpu_family())
conf.set_quoted('PURPLE_BUILD_ENDIANNESS', host_machine.endian())

version_conf.set('PURPLE_MAJOR_VERSION', purple_major_version)
version_conf.set('PURPLE_MINOR_VERSION', purple_minor_version)
version_conf.set('PURPLE_MICRO_VERSION', purple_micro_version)
version_conf.set('PURPLE_EXTRA_VERSION', purple_extra_version)
version_conf.set('PURPLE_VERSION', meson.project_version())
version_conf.set('PURPLE_API_VERSION', purple_soversion)

PURPLE_LIB_VERSION = f'@purple_soversion@.@purple_minor_version@.@purple_micro_version@'

package_revision = vcs_tag(
    input : 'package_revision.h.in',
    output : 'package_revision.h',
    fallback : meson.project_version())

# Global variable for doc targets to add themselves to which we use for the
# doc alias below.
doc_targets = []

# For running `meson devenv`.
devenv = environment()

# Set G_ENABLE_DIAGNOSTIC so we get warnings about deprecated signals and
# properties.
devenv.set('G_ENABLE_DIAGNOSTIC', '1')

# For man pages.
man_conf.set('VERSION', meson.project_version())
man_conf.set('prefix', get_option('prefix'))

# Used for pkg-config files.
pkgconfig = import('pkgconfig')

# Storing build arguments
meson.add_postconf_script('mkmesonconf.py', meson.project_build_root())
conf.set('HAVE_MESON_CONFIG', true)

# Checks for programs.
compiler = meson.get_compiler('c')

# Check for Sun compiler
SUNCC = compiler.compiles('void main() {__SUNPRO_C;};')

# Check for Win32
if host_machine.system() == 'windows'
	windows = import('windows')

	IS_WIN32 = true
	ws2_32 = compiler.find_library('ws2_32')
	dnsapi = compiler.find_library('dnsapi')
	conf.set('WIN32_LEAN_AND_MEAN', true)

	conf.set('LIBPIDGIN_DLL_NAMEW', f'L"pidgin3-@purple_soversion@.dll"')
else
	IS_WIN32 = false
	ws2_32 = []
	dnsapi = []
endif

# Checks for header files.
conf.set('HAVE_UNISTD_H', compiler.has_header('unistd.h'))

# Check for directories
if IS_WIN32
	foreach dir : ['bin', 'lib', 'data', 'sysconf', 'locale']
		path = get_option('prefix') / get_option(dir + 'dir')
		conf.set_quoted('WIN32_FHS_@0@DIR'.format(dir.to_upper()), path)
	endforeach

	conf.set('PURPLE_LIBDIR',
		 f'wpurple_lib_dir("purple-@purple_major_version@")')
	conf.set('PIDGIN_LIBDIR',
		 f'wpurple_lib_dir("pidgin-@purple_major_version@")')

	conf.set('PURPLE_DATADIR', 'wpurple_data_dir()')
	conf.set('PURPLE_SYSCONFDIR', 'wpurple_sysconf_dir()')
	conf.set('PURPLE_LOCALEDIR', 'wpurple_locale_dir()')
else
	foreach dir : ['data', 'sysconf', 'locale']
		path = get_option('prefix') / get_option(dir + 'dir')
		conf.set_quoted('PURPLE_@0@DIR'.format(dir.to_upper()), path)
	endforeach

	common_libdir = get_option('prefix') / get_option('libdir')
	conf.set_quoted('PURPLE_LIBDIR',
	                common_libdir / f'purple-@purple_major_version@')
	conf.set_quoted('PIDGIN_LIBDIR',
	                common_libdir / f'pidgin-@purple_major_version@')
endif

abslibdir = get_option('prefix') / get_option('libdir')
PURPLE_PLUGINDIR = abslibdir / f'purple-@purple_major_version@'
conf.set_quoted('PURPLE_PLUGINDIR', PURPLE_PLUGINDIR)
PIDGIN_PLUGINDIR = abslibdir / f'pidgin-@purple_major_version@'
conf.set_quoted('PIDGIN_PLUGINDIR', PIDGIN_PLUGINDIR)

# Windows and Haiku do not use libm for the math functions, they are part
# of the C library
math = compiler.find_library('m', required: false)

IOKIT = []
if host_machine.system() == 'darwin'
	IOKIT = dependency('appleframeworks',
	                   modules : ['IOKit', 'CoreFoundation'])

	conf.set('HAVE_IOKIT', true)
endif

#######################################################################
# Check for GLib (required)
#######################################################################
# Once we require >= 2.74.0, remove the hack in the if(TRUE) block in
# libpurple/core.c.
glib = dependency('glib-2.0', version : '>= 2.76.0')
gio = dependency('gio-2.0')
gnome = import('gnome')

add_project_arguments(
	'-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_76',
	'-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_76',
	language : 'c',)

#######################################################################
# Check for gdk-pixbuf (required)
#######################################################################
gdk_pixbuf = dependency('gdk-pixbuf-2.0', version : '>= 2.26.0')

#######################################################################
# Check for GObject Introspection
#######################################################################
if get_option('introspection')
	enable_introspection = dependency('gobject-introspection-1.0', version : '>= 1.39.0').found()
else
	enable_introspection = false
endif

#######################################################################
# Check Pidgin dependencies
#######################################################################
if get_option('gtkui')
	gtk = dependency('gtk4', version : '>= 4.10.0')
	libadwaita = dependency('libadwaita-1', version : '>= 1.2')
endif	# GTK

ENABLE_GTK = get_option('gtkui')


#######################################################################
# Check for LibXML2 (required)
#######################################################################
libxml = dependency('libxml-2.0', version : '>= 2.6.0')
if libxml.version().version_compare('<2.6.18')
	message('Versions of libxml2 < 2.6.18 may contain bugs that could cause XMPP messages to be discarded.')
endif

#######################################################################
# Check for JSON-GLib (required)
#######################################################################

json = dependency('json-glib-1.0', version : '>= 0.14.0')

######################################################################
# Check for libsoup (required)
#######################################################################

libsoup = dependency('libsoup-3.0', version : '>= 3')
add_project_arguments(
	'-DSOUP_VERSION_MIN_REQUIRED=SOUP_VERSION_3_0',
	'-DSOUP_VERSION_MAX_ALLOWED=SOUP_VERSION_3_0',
	language : 'c')

#######################################################################
# Check for sqlite3 (required)
#######################################################################
sqlite3 = dependency('sqlite3', version : '>= 3.27.0')

#######################################################################
# Check for GStreamer
#######################################################################

gstreamer = dependency('gstreamer-1.0', version : '>=1.14')

#######################################################################
# Check for Raw data streams support in Farstream
#######################################################################
gstreamer_app = dependency('gstreamer-app-1.0')

#######################################################################
# Check for birb glib utility library
#######################################################################
birb_dep = dependency('birb')

#######################################################################
# Check for Xeme XMPP Library
#######################################################################
xeme = dependency('xeme')

dependency('shoes', required : false)

DEFAULT_PRPLS = ['demo', 'ircv3', 'xmpp']

dynamic_list = get_option('dynamic-prpls').split(',')
if dynamic_list == ['all']
	dynamic_list = DEFAULT_PRPLS
endif
DYNAMIC_PRPLS = []
foreach prpl : dynamic_list
	if prpl == ''
		# The list was empty; do nothing.
	elif prpl == 'xmpp' and not xeme.found()
		# Do nothing.
	else
		DYNAMIC_PRPLS += [prpl]
	endif
endforeach

DYNAMIC_DEMO = DYNAMIC_PRPLS.contains('demo')
DYNAMIC_IRCV3 = DYNAMIC_PRPLS.contains('ircv3')
DYNAMIC_XMPP = DYNAMIC_PRPLS.contains('xmpp')

add_project_arguments(
    '-DPURPLE_DISABLE_DEPRECATED',
    '-DPIDGIN_DISABLE_DEPRECATED',
    '-DGNT_DISABLE_DEPRECATED',
    language : 'c')
if get_option('buildtype') != 'plain' and compiler.get_id() == 'gcc'
	# We enable -Wall later.
	# If it's set after the warning CFLAGS in the compiler invocation, it counteracts the -Wno... flags.
	# This leads to warnings we don't want.
#	CFLAGS=`echo $CFLAGS |$sedpath 's/-Wall//'`

	# ENABLE WARNINGS SUPPORTED BY THE VERSION OF GCC IN USE
	#
	# Future Possibilities
	#
	# Consider adding -Wbad-function-cast.
	#	This leads to spurious warnings using GPOINTER_TO_INT(), et al. directly on a function call.
	#		We'd need an intermediate variable.
	#
	foreach newflag : [
			'-Waggregate-return',
			'-Wcast-align',
			'-Wdeclaration-after-statement',
			'-Wendif-labels',
			'-Werror-implicit-function-declaration',
			'-Wextra -Wno-unused-parameter',
			'-Wformat',
			'-Wformat-security',
			'-Werror=format-security',
			'-Winit-self',
			'-Wmissing-declarations',
			'-Wmissing-noreturn',
			'-Wmissing-prototypes',
			'-Wpointer-arith',
			'-Wfloat-equal',
			'-Wundef']
		if compiler.has_argument(newflag)
			add_project_arguments(newflag, language : 'c')
		endif
	endforeach
endif
if get_option('buildtype') != 'plain' and SUNCC
	add_project_arguments('-features=extensions', language : 'c')
endif

pidgin3path = find_program('pidgin3', required : false)

#######################################################################
# Check for Unity and Messaging Menu
# Remove when Ubuntu 16.04 is EOL
#######################################################################
UNITY = [
	dependency('unity', version : '>= 6.8', required : get_option('unity-integration')),
	dependency('messaging-menu', version : '>= 12.10', required : get_option('unity-integration'))
]
enable_unity = UNITY[0].found() and UNITY[1].found()
if enable_unity
	conf.set('USES_MM_CHAT_SECTION', 'X-MessagingMenu-UsesChatSection=true')
else
	conf.set('USES_MM_CHAT_SECTION', '')
endif

#######################################################################
# Check for Secret Service headers
#######################################################################

if IS_WIN32
	libsecret = disabler()
else
	libsecret = dependency('libsecret-1', required : get_option('libsecret'))
endif

#######################################################################
# Check for KWallet headers
#######################################################################

if IS_WIN32 or not add_languages('cpp', required : get_option('kwallet'), native: false)
	kwallet = disabler()
else
	add_project_arguments([
		'-DDISPLAY_VERSION="@0@"'.format(meson.project_version()),
		'-DPURPLE_WEBSITE="https://pidgin.im/"',
		f'-DGETTEXT_PACKAGE="@GETTEXT_PACKAGE@"'],
		language : 'cpp')

	qt5 = import('qt5')

	qt5_dep = dependency('qt5', modules: ['Core'], required : get_option('kwallet'))

	kwallet = dependency('KF5Wallet', required : get_option('kwallet'))
endif

#######################################################################
# Check for GPlugin
#######################################################################
gplugin_version = ['>=0.44.0', '<0.45.0']
gplugin_dep = dependency('gplugin',
	version : gplugin_version,
	fallback : ['gplugin', 'gplugin_dep'])

if get_option('gtkui')
	gplugin_gtk_dep = dependency('gplugin-gtk4',
		version : gplugin_version,
		fallback : ['gplugin-gtk4', 'gplugin_gtk4_dep'])
endif

#######################################################################
# Check for Hasl
#######################################################################
hasl = dependency('hasl', version : '>= 0.3.0')

#######################################################################
# Documentation
#######################################################################

if get_option('doc') and not get_option('introspection')
    error('Documentation requires GObject Introspection.')
endif

dependency(
    'gi-docgen', version: '>= 2021.1',
    fallback: ['gi-docgen', 'dummy_dep'],
    required: get_option('doc')
)

gidocgen = find_program('gi-docgen', required : get_option('doc'))
docs_dir = get_option('prefix') / get_option('datadir') / 'doc'

#######################################################################
# Random Stuff
#######################################################################
# So that purpleconfig.h may be found.
toplevel_inc = include_directories('.')

# make sure we found our license checker script
check_license_header = find_program('./scripts/check_license_header.py', required : true)

subdir('libpurple')
subdir('purple-history')
subdir('pidgin')
subdir('protocols')
subdir('doc')
subdir('po')

configure_file(output: 'purpleconfig.h', configuration: conf)

config_home = get_option('devenv-config-dir')
if config_home == ''
	config_home = meson.global_build_root() / 'config'
endif
devenv.set('XDG_CONFIG_HOME', config_home)

meson.add_devenv(devenv)

if meson.backend() == 'ninja'
	run_target('turtles',
		command : ['ninja', '-C', '@BUILD_ROOT@', 'pidgin3-pot', 'all', 'test'])
endif

# doc alias target, depends on doc_targets defined above and populated in sub
# directories.
alias_target('doc', doc_targets)

summary({
    'prefix': get_option('prefix'),
    'bindir': get_option('bindir'),
    'libdir': get_option('libdir'),
    'datadir': get_option('datadir'),
}, section : 'Directories')

# TODO: Remove `.found()` once https://github.com/mesonbuild/meson/pull/10949
# is merged and in a release that we require.
summary({
    'GTK': get_option('gtkui'),
}, section: 'User Interfaces', bool_yn: true)

summary({
    'Dynamic protocols': DYNAMIC_PRPLS,
}, section: 'Protocol Support', bool_yn: true, list_sep: ', ')

# TODO: Remove `.found()` once https://github.com/mesonbuild/meson/pull/10949
# is merged and in a release that we require.
summary({
    'KWallet credential provider': kwallet.found(),
    'libsecret credential provider': libsecret.found(),
    'Unity integration': enable_unity,
    'WinCred credential provider': IS_WIN32,
}, section: 'Plugin support', bool_yn: true)

summary({
    'Enable Introspection': enable_introspection,
    'Generate documentation': get_option('doc'),
    'Has you': true,
}, section: 'Miscellaneous', bool_yn: true)

if pidgin3path.found()
    summary('You have an old copy of pidgin3 at', pidgin3path.full_path(),
            section: 'Warnings')
endif
