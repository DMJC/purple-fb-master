subdir('icons')

devenv.prepend('XDG_DATA_DIRS', meson.current_source_dir())

if host_machine.system() == 'windows'
	subdir_done()
endif

i18n = import('i18n')

###############################################################################
# MetaInfo file
###############################################################################
metainfo_base = 'im.pidgin.Pidgin3.metainfo.xml'
metainfo_file = meson.current_source_dir() / metainfo_base

appstreamcli_exe = find_program('appstreamcli', required: false)
if appstreamcli_exe.found()
	test(
		'validate pidgin3 metainfo file',
		appstreamcli_exe,
		args: ['validate', '--no-net', '--pedantic', metainfo_file],
		verbose: true,
	)
endif

i18n.merge_file(
	input:  metainfo_file,
	output: metainfo_base,
	type: 'xml',
	po_dir: meson.project_source_root() / 'po',
	install: true,
	install_dir: get_option('datadir') / 'metainfo',
)

###############################################################################
# Desktop file
###############################################################################
PIDGIN_DESKTOP_FILE = 'im.pidgin.Pidgin3.desktop'
desktop_file_in = configure_file(
	input : f'@PIDGIN_DESKTOP_FILE@.in.in',
	output : f'@PIDGIN_DESKTOP_FILE@.in',
	configuration : conf,
)

i18n.merge_file(
	input : desktop_file_in,
	output : PIDGIN_DESKTOP_FILE,
	po_dir : meson.project_source_root() / 'po',
	type : 'desktop',
	install : true,
	install_dir : get_option('datadir') / 'applications',
)

desktop_file_validate_exe = find_program('desktop-file-validate', required: false)
if desktop_file_validate_exe.found()
	test(
		'validate pidgin3 desktop file',
		desktop_file_validate_exe,
		args: [meson.current_build_dir() / PIDGIN_DESKTOP_FILE],
		verbose: true,
	)
endif
