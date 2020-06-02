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
    ]
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
    'preferences-dialog.vala',
    'scoreboard.vala',
    'speed.vala',
    'warp.vala',
    'worm.vala',
    resources,
]

gnome_nibbles_deps = [
    clutter_dep,
    clutter_gtk_dep,
    gee_dep,
    gio_dep,
    glib_dep,
    gsound_dep,
    gtk_dep,
    libgnome_games_support_dep,
    posix_dep
]

gnome_nibbles_vala_args = [
    '--vapidir', join_paths(meson.current_source_dir(), 'vapi')
]

executable(meson.project_name(), gnome_nibbles_sources,
    dependencies: gnome_nibbles_deps,
    vala_args: gnome_nibbles_vala_args,
    install: true
)
