# Tests

nibbles_tests = executable(
    'nibbles_tests',
    [
        'boni.vala',
        'nibbles-game.vala',
        'nibbles-test.vala',
        'worm.vala',
        'warp.vala',
    ],
    dependencies : [
        gee_dep,
        gio_dep,
        glib_dep,
    ],
    vala_args : if_defines + '--define=TEST_COMPILE' + target_glib,
    link_args : '-lm'
)
test('nibbles-tests', nibbles_tests)

# gnome-nibbles executable

gnome_nibbles_sources = [
    'vapi/config.vapi',
    'boni.vala',
    'controls.vala',
    'gnome-nibbles.vala',
    'nibbles-game.vala',
    'nibbles-view.vala',
    'nibbles-window.vala',
    'players.vala',
    'scoreboard.vala',
    'speed.vala',
    'warp.vala',
    'worm.vala',
    'sound.vala',
    resources,
]

gnome_nibbles_deps = [
    gee_dep,
    gio_dep,
    glib_dep,
    gsound_dep,
    libgnome_games_support_dep,
    gtk_dep,
    pangocairo_dep,
    posix_dep
]

gnome_nibbles_vala_args = [
    '--vapidir', join_paths(meson.current_source_dir(), 'vapi')
]

executable(meson.project_name(), gnome_nibbles_sources,
    dependencies: gnome_nibbles_deps,
    vala_args: gnome_nibbles_vala_args + if_defines + target_glib,
    link_args: '-lm',
    install: true
)
