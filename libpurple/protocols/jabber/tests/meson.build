foreach prog : ['caps', 'digest_md5', 'scram', 'jutil']
	e = executable(
	    f'test_jabber_@prog@', f'test_jabber_@prog@.c',
	    link_with : [jabber_prpl],
	    dependencies : [libxml, libpurple_dep, libsoup, glib],
	    override_options : ['c_std=c99', 'warning_level=2'])

	jabberenv = environment()
	jabberenv.set('XDG_CONFIG_DIR', meson.current_build_dir() / 'config')

	test(f'jabber_@prog@', e, env: jabberenv)
endforeach
