# gnome-nibbles executable

gnome_nibbles_sources = [
    'config.vapi',
    'boni.vala',
    'gnome-nibbles.vala',
    'nibbles-game.vala',
    'nibbles-view.vala',
    'preferences-dialog.vala',
    'warp.vala',
    'worm.vala',
    resources,
]

gnome_nibbles_deps = [
    gee_dep,
    gio_dep,
    glib_dep,
    gtk_dep,
    clutter_dep,
    clutter_gtk_dep,
    canberra_dep,
    canberra_gtk3_dep,
    libgnome_games_support_dep,
    posix_dep,
]

gnome_nibbles_vala_args = [
  '--gresources', resource_files,
]

executable(meson.project_name(), gnome_nibbles_sources,
    dependencies: gnome_nibbles_deps,
    vala_args: gnome_nibbles_vala_args,
    install: true
)
