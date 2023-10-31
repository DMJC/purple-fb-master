PROGRAMS = [
	'text_buffer',
]

TEST_WRAPPER = find_program('./test-wrapper.py', required: true)

testenv.set('XDG_CONFIG_HOME', meson.current_build_dir() / 'config')

foreach program : PROGRAMS
    e = executable(f'test_@program@', f'test_@program@.c',
                   c_args : [
                       '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
                       '-DTEST_CACHE_DIR="@0@/cache"'.format(meson.current_build_dir()),
                   ],
                   dependencies : [libpurple_dep, libpidgin_dep, glib, gtk],
    )
    test(program, TEST_WRAPPER, args : e, is_parallel : false, env: testenv)
endforeach
