PURPLE_HISTORY_SOURCES = [
	'purplehistorycore.c',
]

purple_history = executable('purple-history',
	PURPLE_HISTORY_SOURCES,
	dependencies : [libpurple_dep, glib],
	install : true,
	override_options : ['c_std=c99', 'warning_level=2'])
