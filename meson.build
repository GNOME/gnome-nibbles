project('gnome-nibbles', ['c', 'vala'],
  license: 'GPL3+',
  meson_version: '>= 0.50.1',
  version: '3.38.2'
)

gnome = import('gnome')
i18n = import('i18n')

prefix      = get_option('prefix')
datadir     = join_paths(prefix, get_option('datadir'))
pkgdatadir  = join_paths(datadir, meson.project_name())
sounddir    = join_paths(pkgdatadir, 'sounds')
localedir   = join_paths(prefix, get_option('localedir'))
podir       = join_paths(meson.source_root(), 'po')
bindir      = join_paths(prefix, get_option ('bindir'))

# Dependencies
glib_version = '2.42.0'

clutter_dep = dependency('clutter-1.0', version: '>= 1.22.0')
clutter_gtk_dep = dependency('clutter-gtk-1.0', version: '>= 1.4.0')
gee_dep = dependency('gee-0.8')
gio_dep = dependency('gio-2.0', version: '>= @0@'.format(glib_version))
glib_dep = dependency('glib-2.0', version: '>= @0@'.format(glib_version))
gsound_dep = dependency('gsound', version: '>= 1.0.2')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.24.0')
libgnome_games_support_dep = dependency('libgnome-games-support-1', version: '>= 1.7.1')

valac = meson.get_compiler('vala')
posix_dep = valac.find_library('posix')

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

# Extra scripts
meson.add_install_script('meson_post_install.py')

# Subdirs
subdir('data')
subdir('help')
subdir('po')
subdir('src')
