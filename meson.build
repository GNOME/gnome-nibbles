# Project
project('gnome-nibbles',
    ['c', 'vala'],
    license: 'GPL3+',
    meson_version: '>=1.5',
    version: '4.4.1',
    default_options: ['c_std=c99']) # Vala generated C code should be compatible with c99 (ISO/IEC 9899:1999)

# note, the version number is in meson.options and follows this sequence:
#  4.4.rc1
#  4.4.rc2 (if needed)
#  4.4.0 (next full release)
#  4.4.1 (bugfix release)
message(meson.project_name(), 'version', get_option('version') + get_option('append_to_version'))
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
glib_minimum_version = '2.76.0' # minimum version for GTK 4.14
glib_target_version = '2.80.0' # tested with version 2.80
target_glib = '--target-glib=' + '@0@'.format(glib_target_version)
gettext_package = 'gnome-nibbles'

valac = meson.get_compiler('vala')
if valac.version().version_compare('< ' + vala_minimum_version)
    error('Vala does not meet minimum required version of ' + vala_minimum_version)
endif
c = meson.get_compiler('c')

gee_dep = dependency('gee-0.8')
gio_dep = dependency('gio-2.0', version: '>= @0@'.format(glib_minimum_version))
glib_dep = dependency('glib-2.0', version: '>= @0@'.format(glib_minimum_version))
gsound_dep = dependency('gsound', version: '>= 1.0.2')
gtk_dep = dependency('gtk4', version: '>= 4.14') # needed for Gsk.PathBuilder()
libgnome_games_support = subproject ('libgnome-games-support', version: ['>= 3', '< 4'], 
    default_options: ['superproject_name='+gettext_package])
libgnome_games_support_dep = libgnome_games_support.get_variable ('lggs_dependency')
libadwaita_dep = dependency('libadwaita-1', version: '>= 1.5.0') # requires GTK 4.13.4
pango_dep = dependency('pango', version: '>= 1.8') # needed for Layout.get_font_description()
gtk5_dep = dependency('gtk5', required : false)
if gtk5_dep.found()
    message('GTK5 version', gtk5_dep.version() ,'is available on this system.')
endif

posix_dep = valac.find_library('posix')

# C compiler options
c_compiler_options = []
# Suppress all the C compiler warnings.
if c.has_argument('-w')
    c_compiler_options += '-w'
endif

# convert C compiler options to vala compiler options
vala_compiler_options = []
foreach i : range(0, c_compiler_options.length(), 1)
    vala_compiler_options += '--Xcc=' + c_compiler_options[i]
endforeach

# *** Tests ***
# For each test we add a --define= entry to vala_if_defines so the vala code
# can handle any issue or feature.
vala_if_defines = []

# This test is to support versions of vala(<0.56.15) that have compile issues
# with versions of gcc(>=14) or clang(>=16).
code0 = '''private const string [] strings = {"a","b"};
           private static string test_string (string [] s) {return s[0];}
           void main() {stdout.printf ("%s\n",test_string (strings));}'''
code0_links = valac.links(code0, 
    args : vala_compiler_options,
    name : 'an array of strings links without casting?')
if not code0_links
    message('Vala has a bug when passing a constant array of strings to a function, workaround applied.')
    vala_if_defines += '--define=VALA_NEEDS_ARRAY_STRING_CASTING'
endif

# Test if this commit is in vala. It is in 0.56.17 and above.
# https://gitlab.gnome.org/GNOME/vala/-/commit/f3288279b6f74cd4255b52ab78dddb5ee7bcb89c
# This test is to maintain support for vala 0.56 (Stable Long-term Support release).
code1 = '''class Property : Object {public int c;}
           void main () {
           static bool GetMappingFunction (Value value, Variant variant, void *data) {value.set_int (0); return true;}
           static Variant SetMappingFunction (Value value, VariantType type, void *data) {return 0;}
           var s = new GLib.Settings ("org.gnome.Nibbles"); var p = new Property ();
           s.bind_with_mapping ("c", p, "c", SettingsBindFlags.DEFAULT,GetMappingFunction, SetMappingFunction, null, null); }'''
code1_links = valac.links(code1, 
    dependencies : [gio_dep], # needs meson version >= 1.5.0
    args : vala_compiler_options,
    name : 'static local functions can be used without casting?')
if not code1_links
    message('Vala static local functions need casting to static delegates.')
    vala_if_defines += '--define=VALA_NEEDS_LOCAL_FUCTION_TO_STATIC_DELEGATE_CASTING'
endif

# Test compiling & linking with Adw.AboutDialog.
# This test makes sure libadwaita is installed and working.
code2 = '''void main () {string[] s = {"a", "b"};
           var ad = new Adw.AboutDialog ();
           ad.set_developers (s);
           ad.set_documenters (s);
           ad.set_artists (s);}'''
code2_links = valac.links(code2, 
    dependencies : [libadwaita_dep], # needs meson version >= 1.5.0
    args : vala_compiler_options,
    name : 'we can link with libadwaita?')
if code2_links
    vala_if_defines += ['--define=USE_LIBADWAITA','--define=USE_PILL_BUTTON']
else
    error('Can not link with libadwaita.')
endif

# Test if this fix is in vala.
# https://gitlab.gnome.org/GNOME/vala/-/merge_requests/456
code3 = '''internal class CCC<T1,T2> {
               T1 a0;
               T2 a1;
               ulong sizeof_T1 = sizeof (T1);
               ulong sizeof_T2 = sizeof (T2);
               internal void f() {
                   a0 = 0xfe;
                   assert (a0 == 0xfe);
                   a1 = 0x7f; // segfault
                   assert (a1 == 0x7f);
                   var v = a0 & a1; // compile error
                   assert (v == 0x7e);
                   assert (sizeof_T1 == 1); // assert fail
               }
           }
           void main () {
               var test = new CCC<uint8, uint64?>();
               test.f ();
           }'''
code3_links = valac.links(code3, 
    args : vala_compiler_options,
    name : 'generic type fix is in vala?')
if not code3_links
    vala_if_defines += '--define=GENERIC_TYPE_BUG'
endif

# Test for show_uri function
# We use show_uri in preference to UriLauncher because of
# https://gitlab.gnome.org/GNOME/gtk/-/issues/6135
code4 = '''using Gtk;
           void main () {show_uri (null, "help:gnome-nibbles", Gdk.CURRENT_TIME);}'''
code4_links = valac.links(code4,
    dependencies : [gtk_dep], # needs meson version >= 1.5.0
    args : vala_compiler_options,
    name : 'we can call show_uri?')
if code4_links
    vala_if_defines += '--define=CAN_USE_SHOW_URI'
else
    message('We will use UriLauncher instead of show_uri.')
endif

# Test for 128 bit integer (__int128)
# If available __int128 allows us to do better nibbles tests.
code5 = '''
    typedef unsigned long long  uint64;
    struct _int128
    {
        uint64  hi;
        uint64  lo;
        int negative;
        uint64  remainder;
    };
    void divide_by(struct _int128 *i128, long long i64, struct _int128 *r)
    {
        unsigned __int128 n,a;
        n = ((__int128)i128->hi) << 64 | i128->lo;
        if(i64 < 0)
        {
            a = n / -(__int128)i64;
            r->remainder = n % -(__int128)i64;
        }
        else
        {
            a = n / i64;
            r->remainder = n % i64;
        }
        r->hi = a >> 64;
        r->lo = (uint64)a;
        r->negative = (i128->negative && i64 > 0) || (!i128->negative && i64 < 0);
    }'''
code5_compiles = c.compiles(code5,
    args : c_compiler_options,
    name : 'we can use 128 bit integers?')
if code5_compiles
    vala_if_defines += '--define=CAN_USE__INT128'
    conditional_sources = ['divide_by.c']
else
    conditional_sources = []
endif

message('valac #defines:',vala_if_defines)

# Configuration
conf = configuration_data()
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('SOUND_DIRECTORY', sounddir)
conf.set_quoted('LOCALEDIR', join_paths(prefix, localedir))
conf.set_quoted('VERSION', get_option('version') + get_option('append_to_version'))
conf.set_quoted('WEBSITE', get_option('website'))

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
