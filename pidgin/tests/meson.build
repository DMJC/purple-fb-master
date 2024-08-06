BROADWAY_TESTS = [
]

TEST_WRAPPER = find_program('./test-wrapper.py', required: true)
BROADWAYD = find_program('gtk4-broadwayd', required: false)

testenv.set('XDG_CONFIG_HOME', meson.current_build_dir() / 'config')

if BROADWAYD.found()
	foreach program : BROADWAY_TESTS
		e = executable(f'test_@program@', f'test_@program@.c',
		               c_args : ['-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
		                         '-DTEST_CACHE_DIR="@0@/cache"'.format(meson.current_build_dir())],
		               dependencies : [libpurple_dep, libpidgin_dep, glib, gtk])
		test(program, TEST_WRAPPER,
		     args : [BROADWAYD.full_path(), e],
		     is_parallel : false,
		     env: testenv)
	endforeach
endif
