# tests
nibbles_tests = executable(
	'nibbles_tests',
	[
		'tests.cpp',
		'worm.cpp',
		'game.cpp',
		'pseudo_random.cpp'
	],
	dependencies : [
		glibmm_dep,
		gtkmm_dep
	],
	cpp_args: cpp_compiler_options,
	link_args : '-lm'
)
test('nibbles-tests', nibbles_tests, timeout: 150)

# gnome-nibbles executable
gnome_nibbles_sources = [
	'nibbles.cpp',
	'nibbles-window.cpp',
	'view.cpp',
	'worm.cpp',
	'game.cpp',
	'pseudo_random.cpp',
	'critical.cpp',
	resources
]

gnome_nibbles_deps = [
	glibmm_dep,
	gtkmm_dep,
	gsound_dep 
]

executable(meson.project_name(), gnome_nibbles_sources,
	dependencies: gnome_nibbles_deps,
	cpp_args: cpp_compiler_options,
	link_args: '-lm',
	install: true
)
