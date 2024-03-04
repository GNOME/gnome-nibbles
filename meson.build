# Project & version number
project('gnome-nibbles', ['c', 'vala'],
  license: 'GPL3+',
  meson_version: '>= 0.62',
  version: '4.0.1'
)
# note, the version numbering now follows the following sequence:
#  4.1.alpha
#  4.1.beta
#  4.1.rc1
#  4.1.0 (next full release)
#  4.1.1 (bugfix release)

gnome = import('gnome')
i18n = import('i18n')

prefix      = get_option('prefix')
datadir     = join_paths(prefix, get_option('datadir'))
pkgdatadir  = join_paths(datadir, meson.project_name())
sounddir    = join_paths(pkgdatadir, 'sounds')
localedir   = join_paths(prefix, get_option('localedir'))
podir       = join_paths(meson.project_source_root(), 'po')
bindir      = join_paths(prefix, get_option ('bindir'))

# Dependencies
vala_minimum_version = '0.55.1' # for sub-functions; https://gitlab.gnome.org/GNOME/vala/-/commit/048aaba0cd88b784fe9fa421dbad494019abd42e
glib_minimum_version = '2.66.0' # minimum version for GTK 4
glib_target_version = '2.78.0' # most recent tested version
target_glib = '--target-glib=' + '@0@'.format(glib_target_version)

valac = meson.get_compiler('vala')
if valac.version().version_compare('< ' + vala_minimum_version)
  error('Vala does not meet minimum required version of ' + vala_minimum_version)
endif
gee_dep = dependency('gee-0.8')
gio_dep = dependency('gio-2.0', version: '>= @0@'.format(glib_minimum_version))
glib_dep = dependency('glib-2.0', version: '>= @0@'.format(glib_minimum_version))
gsound_dep = dependency('gsound', version: '>= 1.0.2')
libgnome_games_support_dep = dependency('libgnome-games-support-2', version: '>= 2.0.0') # requires GTK 4.2
gtk_dep = dependency('gtk4', version: '>= 4.6') # this code works on 4.6.9 (ubuntu 22.04 with updates), it hasn't been tried on anything earlier
pangocairo_dep = dependency('pangocairo')

if_defines = []
# version defines of GTK4 for #if e.g. #if GTK_4_6_or_above
gtk_version = gtk_dep.version()
foreach i : range(4, gtk_version.split('.')[0].to_int() + 1, 1)
  foreach o : range(0, gtk_version.split('.')[1].to_int() + 1, 1)
    if_defines += '--define=GTK_' + i.to_string() + '_' + o.to_string() + '_or_above'
  endforeach
endforeach

posix_dep = valac.find_library('posix')

# version defines of vala for #if e.g. #if VALA_0_56_15_or_above
vala_version = valac.version()
foreach i : range(56, vala_version.split('.')[1].to_int() + 1, 1)
  foreach o : range(0, vala_version.split('.')[2].to_int() + 1, 1)
    if_defines += '--define=VALA_0_' + i.to_string() + '_' + o.to_string() + '_or_above'
  endforeach
endforeach

# Configuration
conf = configuration_data()

conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('SOUND_DIRECTORY', sounddir)
conf.set_quoted('LOCALEDIR', join_paths(prefix, localedir))
conf.set_quoted('VERSION', meson.project_version())

configure_file(output: 'config.h', configuration: conf)
config_h_dir = include_directories('.')

add_project_arguments([
    '-include', 'config.h'
  ],
  language: 'c'
)

# Postinstall
gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)

# Subdirs
subdir('data')
subdir('help')
subdir('po')
subdir('src')
