library('libhelloworld', 'hello_world.c',
	c_args : [
		'-DG_LOG_USE_STRUCTURED',
		'-DG_LOG_DOMAIN="PurplePlugin-HelloWorld"',
		'-Wno-unused-parameter',
	],
	dependencies : [libpurple_dep],
	name_prefix : '',
	install : false)
