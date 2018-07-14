# Desktop file
i18n.merge_file(
    input: 'org.gnome.Nibbles.desktop.in',
    output: 'org.gnome.Nibbles.desktop',
    type: 'desktop',
    po_dir: podir,
    install: true,
    install_dir: join_paths(datadir, 'applications')
)

# Sound
foreach file: ['appear.ogg', 'bonus.ogg', 'crash.ogg', 'gameover.ogg', 'gobble.ogg', 'life.ogg', 'reverse.ogg', 'teleport.ogg']
    install_data (file, install_dir: sounddir)
endforeach

# UI files
ui_files = [
    'nibbles-menus.ui',
    'nibbles.ui',
    'nibbles.css',
    'scoreboard.ui',
    'player-score-box.ui',
    'controls-grid.ui',
    'preferences-dialog.ui'
]
foreach file: ui_files
    install_data(file, install_dir: pkgdatadir)
endforeach

# Icons
foreach size: [ '16x16', '22x22', '24x24', '32x32', '48x48' ]
    install_data(join_paths('icons', 'hicolor', size, 'gnome-nibbles.png'),
                 install_dir: join_paths (datadir, 'icons', 'hicolor', size, 'apps')
    )
endforeach

install_data(join_paths('icons', 'hicolor', 'scalable', 'gnome-nibbles.svg'),
             install_dir: join_paths (datadir, 'icons', 'hicolor', 'scalable', 'apps'))
install_data(join_paths('icons', 'hicolor', 'symbolic', 'gnome-nibbles-symbolic.svg'),
             install_dir: join_paths (datadir, 'icons', 'hicolor', 'symbolic', 'apps'))

# Appdata
i18n.merge_file(
    input: 'org.gnome.Nibbles.appdata.xml.in',
    output: 'org.gnome.Nibbles.appdata.xml',
    type: 'xml',
    po_dir: podir,
    install: true,
    install_dir: join_paths(datadir, 'metainfo'),
)

# Gschema
install_data('org.gnome.nibbles.gschema.xml',
    install_dir: join_paths(datadir, 'glib-2.0', 'schemas')
)

# Gresource
resource_files = files('gnome-nibbles.gresource.xml')
resources = gnome.compile_resources('gnome-nibbles', resource_files)

# Manpage
install_man('gnome-nibbles.6')

# Subdirs
subdir('levels')
subdir('pix')
