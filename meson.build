# Project & version number
project('gnome-nibbles',
    ['c', 'vala'],
    license: 'GPL3+',
    meson_version: '>= 0.60.0',
    version: '4.2.dev'
)
# note, the version numbering now follows the following sequence:
#  4.1.alpha
#  4.1.beta
#  4.1.rc1
#  4.1.0 (next full release)
#  4.1.1 (bugfix release)

gnome      = import('gnome')      # used to install help
i18n       = import('i18n')       # used for internationalization
prefix     = get_option('prefix') # base install directory
datadir    = join_paths(prefix, get_option('datadir'))
pkgdatadir = join_paths(datadir, meson.project_name())
sounddir   = join_paths(pkgdatadir, 'sounds')
localedir  = join_paths(prefix, get_option('localedir'))
podir      = join_paths(meson.project_source_root(), 'po')
bindir     = join_paths(prefix, get_option ('bindir'))

# Dependencies
vala_minimum_version = '0.55.1' # for sub-functions; https://gitlab.gnome.org/GNOME/vala/-/commit/048aaba0cd88b784fe9fa421dbad494019abd42e
glib_minimum_version = '2.66.0' # minimum version for GTK 4
glib_target_version = '2.78.0' # tested with versions 2.74 & 2.78
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
gtk_dep = dependency('gtk4') # this code works on 4.8.3 (debian 12 with updates), it hasn't been tried on anything earlier
libadwaita_dep = dependency('libadwaita-1', version: '>= 1.5.0', required : false) # requires GTK 4.13.4
pangocairo_dep = dependency('pangocairo')
gtk5_dep = dependency('gtk5', required : false)
if gtk5_dep.found()
    message('GTK5 version', gtk5_dep.version() ,'is available on this system.')
endif

# Set use_libadwaita to false to stop libadwaita from being used.
use_libadwaita = libadwaita_dep.found() # = false

# version defines of GTK4 for #if e.g. #if GTK_4_6_or_above
if_defines = []
gtk_version = gtk_dep.version()
gtk_big = gtk_version.split('.')[0].to_int()
foreach i : range(0, gtk_version.split('.')[1].to_int() + 1, 1)
    if_defines += '--define=GTK_' + gtk_big.to_string() + '_' + i.to_string() + '_or_above'
endforeach

posix_dep = valac.find_library('posix')

# Link tests
# This test is to support versions of vala(<0.56.15) that have compile issues
# with versions of gcc(>=14) or clang(>=16).
code0 =  '''private const string [] strings = {"a","b"};
            private static string test_string (string [] s) {return s[0];}
            void main() {stdout.printf ("%s\n",test_string (strings));}'''
if meson.version().version_compare('> 1.4.0')
    # The next line exposes a bug in meson: https://github.com/mesonbuild/meson/issues/12959
    code0_links = valac.links(code0, 
        args : ['--Xcc=-DVALA_STRICT_C'] + if_defines,
        name : 'an array of strings does not needs casting?')
    if not code0_links
        message('Vala has a bug when passing a constant array of strings to a function, workaround applied.')
        if_defines += '--define=VALA_NEEDS_ARRAY_STRING_CASTING'
    endif
else
    # we can't do the test because of the meson bug so be on the safe side
    if_defines += '--define=VALA_NEEDS_ARRAY_STRING_CASTING'
endif

# Test if this commit is in vala. It is in 0.56.17 and above.
# https://gitlab.gnome.org/GNOME/vala/-/commit/f3288279b6f74cd4255b52ab78dddb5ee7bcb89c
# This test is to maintain support for vala 0.56 (Stable Long-term Support release).
code1 =  '''class Property : Object {public int c;}
            void main () {
            static bool GetMappingFunction (Value value, Variant variant, void *data) {value.set_int (0); return true;}
            static Variant SetMappingFunction (Value value, VariantType type, void *data) {return 0;}
            var s = new GLib.Settings ("org.gnome.Nibbles"); var p = new Property ();
            s.bind_with_mapping ("c", p, "c", SettingsBindFlags.DEFAULT,GetMappingFunction, SetMappingFunction, null, null); }'''
if meson.version().version_compare('> 1.4.0')
    # The next line exposes a bug in meson: https://github.com/mesonbuild/meson/issues/12959
    code1_links = valac.links(code1, 
        dependencies : [gio_dep],
        args : ['--Xcc=-DVALA_STRICT_C'] + if_defines,
        name : 'local functions do not need casting to static delegates?')
    if not code1_links
        message('Vala local functions need casting to static delegates.')
        if_defines += '--define=VALA_NEEDS_LOCAL_FUCTION_TO_STATIC_DELEGATE_CASTING'
    endif
else
    # we can't do the test because of the meson bug so be on the safe side
    if_defines += '--define=VALA_NEEDS_LOCAL_FUCTION_TO_STATIC_DELEGATE_CASTING'
endif

# Test compiling & linking with Adw.AboutDialog.
if use_libadwaita
    code2 =  '''void main () {string[] s = {"a", "b"};
                var ad = new Adw.AboutDialog ();
                ad.set_developers (s);
                ad.set_documenters (s);
                ad.set_artists (s);}'''
    if meson.version().version_compare('> 1.4.0')
        # The next line exposes a bug in meson: https://github.com/mesonbuild/meson/issues/12959
        code2_links = valac.links(code2, 
            dependencies : [libadwaita_dep],
            args : ['--Xcc=-DVALA_STRICT_C'] + if_defines,
            name : 'we can compile & link with libadwaita?')
        if not code2_links
            message('Can not compile & link with libadwaita.')
            use_libadwaita = false
        endif
    else
        # we can't do the test because of the meson bug so be on the safe side
        use_libadwaita = false
    endif
endif

# Test for ApplicationFlags.DEFAULT_FLAGS
code3 =  '''using Gtk;
            void main () {var t=ApplicationFlags.DEFAULT_FLAGS;}'''
if meson.version().version_compare('> 1.4.0')
    # The next line exposes a bug in meson: https://github.com/mesonbuild/meson/issues/12959
    code3_links = valac.links(code3, 
        dependencies : [gtk_dep],
        args : ['--Xcc=-DVALA_STRICT_C'] + if_defines,
        name : 'Is ApplicationFlags.DEFAULT_FLAGS defined?')
    if not code3_links
        message('We will use ApplicationFlags.FLAGS_NONE.')
    else
        if_defines += '--define=CAN_USE_DEFAULT_FLAGS'
    endif
endif

# set if_defines for LIBADWAITA
if use_libadwaita
    if_defines += ['--define=USE_LIBADWAITA','--define=USE_PILL_BUTTON']
endif

message('valac #defines:',if_defines)

# Define VALA_STRICT_C so we can compile with the proper C code fixes not the workaround.
# see https://gitlab.gnome.org/GNOME/vala/-/commit/ae2cbcd0011081581bef406fcbfcd1e86b7dacd3
c_defines = ['-DVALA_STRICT_C']

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
