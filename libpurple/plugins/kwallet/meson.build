kwallet_moc = qt5.preprocess(
	moc_headers: 'purplekwallet.h',
	dependencies: qt5_dep,
)

kwallet_sources = [
	'purplekwallet.cpp',
	'purplekwallet.h',
	kwallet_moc,
]

library('purplekwallet', kwallet_sources,
    cpp_args : ['-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="Purple-KWalletKeyring"'],
    gnu_symbol_visibility : 'hidden',
    dependencies : [kwallet, qt5_dep, libpurple_dep],
    name_prefix : '',
    install : true, install_dir : PURPLE_PLUGINDIR)

devenv.append('PURPLE_PLUGIN_PATH', meson.current_build_dir())
