# Desktop file
desktop_file = i18n.merge_file(
    input: 'org.gnome.Nibbles.desktop.in',
    output: 'org.gnome.Nibbles.desktop',
    type: 'desktop',
    po_dir: podir,
    install: true,
    install_dir: join_paths(datadir, 'applications')
)

# Validate desktop file
desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
    test(
        'validate-desktop',
        desktop_file_validate,
        args: [
            desktop_file.full_path()
        ],
        depends: desktop_file
    )
endif

# Sound
install_data([
    join_paths('sounds', 'appear.ogg'),
    join_paths('sounds', 'bonus.ogg'),
    join_paths('sounds', 'crash.ogg'),
    join_paths('sounds', 'gameover.ogg'),
    join_paths('sounds', 'gobble.ogg'),
    join_paths('sounds', 'life.ogg'),
    join_paths('sounds', 'reverse.ogg'),
    join_paths('sounds', 'teleport.ogg')
], install_dir: sounddir)

# Icons
install_data(join_paths('icons', 'hicolor', 'scalable', 'org.gnome.Nibbles.svg'),
             install_dir: join_paths (datadir, 'icons', 'hicolor', 'scalable', 'apps'))
install_data(join_paths('icons', 'hicolor', 'symbolic', 'org.gnome.Nibbles-symbolic.svg'),
             install_dir: join_paths (datadir, 'icons', 'hicolor', 'symbolic', 'apps'))

# Make D-Bus activatable
service_conf = configuration_data()
service_conf.set('bindir', bindir)

service = 'org.gnome.Nibbles.service'

configure_file (
    input: service + '.in',
    output: service,
    install: true,
    install_dir: join_paths (datadir, 'dbus-1', 'services'),
    configuration: service_conf
)

# Metainfo
metainfo_file = i18n.merge_file(
    input: 'org.gnome.Nibbles.metainfo.xml.in',
    output: 'org.gnome.Nibbles.metainfo.xml',
    type: 'xml',
    po_dir: podir,
    install: true,
    install_dir: join_paths(datadir, 'metainfo'),
)

# Validate AppData file
appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
    if meson.version().version_compare('>= 0.62.0')
        message(appstreamcli.full_path(),'version',appstreamcli.version())
    endif
    test(
        'validate-metainfo', appstreamcli,
        args: [
            'validate', '--no-net', metainfo_file.full_path()
        ],
        depends: metainfo_file
    )
endif

# Gschema
install_data('org.gnome.Nibbles.gschema.xml',
    install_dir: join_paths(datadir, 'glib-2.0', 'schemas')
)

# Gresource
resources = gnome.compile_resources('gnome-nibbles', 'gnome-nibbles.gresource.xml')

# Manpage
install_man('gnome-nibbles.6')

# Subdirs
subdir('levels')
subdir('pix')
