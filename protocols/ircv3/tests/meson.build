TESTS = [
	'formatting',
	'parser',
	'source',
]

foreach prog : TESTS
	e = executable(
		f'test_ircv3_@prog@', f'test_ircv3_@prog@.c',
		dependencies : [birb_dep, libpurple_dep, glib, hasl],
		objects : ircv3_prpl.extract_all_objects(),
		c_args : ['-DPURPLE_IRCV3_COMPILATION'])

	test(f'ircv3_@prog@', e)
endforeach
