# Tests
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
		gtkmm,
        glib_dep,
    ],
    cpp_args: cpp_if_defines,
    link_args : '-lm'
)
test('nibbles-tests', nibbles_tests)


# gnome-nibbles executable
gnome_nibbles_sources = [
    'nibbles.cpp',
    'nibbles-window.cpp',
    'view.cpp',
	'worm.cpp',
    'game.cpp',
    'pseudo_random.cpp',
	resources
]

gnome_nibbles_deps = [
	glibmm_dep,
	gtkmm,
#    gee_dep,
#    gio_dep,
    glib_dep,
    gsound_dep,
#    libgnome_games_support_dep,
    gtk_dep,
    pango_dep,
#    posix_dep,
#    libadwaita_dep
]

executable(meson.project_name(), gnome_nibbles_sources,
    dependencies: gnome_nibbles_deps,
    cpp_args: cpp_if_defines,
    link_args: '-lm',
    install: true
)
