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
    ]
  )
endif

# Sound
install_data([
    'appear.ogg',
    'bonus.ogg',
    'crash.ogg',
    'gameover.ogg',
    'gobble.ogg',
    'life.ogg',
    'reverse.ogg',
    'teleport.ogg'
], install_dir: sounddir)

# Icons
foreach size: [ '16x16', '22x22', '24x24', '32x32', '48x48' ]
    install_data(join_paths('icons', 'hicolor', size, 'org.gnome.Nibbles.png'),
                 install_dir: join_paths (datadir, 'icons', 'hicolor', size, 'apps')
    )
endforeach

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

# Appdata
appdata_file = i18n.merge_file(
    input: 'org.gnome.Nibbles.appdata.xml.in',
    output: 'org.gnome.Nibbles.appdata.xml',
    type: 'xml',
    po_dir: podir,
    install: true,
    install_dir: join_paths(datadir, 'metainfo'),
)

# Validate AppData file
appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test(
    'validate-appdata', appstream_util,
    args: [
      'validate-relax', '--nonet', appdata_file.full_path()
    ]
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
