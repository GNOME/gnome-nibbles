# Project
project('gnome-nibbles',
	['cpp'],
	license: ['GPL3+'],
	meson_version: '>=1.1',
	version: '4.6.rc1',
	default_options: ['cpp_std=gnu++26']) # for std::ranges::contains

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

gettext_package = 'gnome-nibbles'

# Dependencies
gtkmm_dep = dependency('gtkmm-4.0')
gsound_dep = dependency('gsound', version: '>= 1.0.2')

cpp = meson.get_compiler('cpp')
cpp_compiler_options = ['-Wno-narrowing','-Wno-parentheses']

code0 = '''
#include <gtkmm.h>

class Box : public Gtk::Widget
{
public:
	Box() : Gtk::Widget()
	{
	}
	virtual ~Box() override = default;
protected:
 	void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& s) override
 	{
		auto pb = Gsk::PathBuilder::create();
		pb->add_rect({0,0,static_cast<float>(get_width()),static_cast<float>(get_height())});
		s->append_fill(pb->to_path(), Gsk::FillRule::EVEN_ODD, {1,0,0,1});
	}
};

int main(int argc, char* argv[])
{
	auto test=Gtk::make_managed<Box>();
}
'''
code0_links = cpp.links(code0, dependencies: [gtkmm_dep], name : 'Gsk::PathBuilder()')
if not code0_links
	error('Can not link with Gsk::PathBuilder (requires gtkmm-4.0 > 4.23.0)')
endif

code1 = '''
	#include <gtkmm.h>
	int main(int argc, char *argv[])
	{
		Gtk::ApplicationWindow window;
		Glib::RefPtr<Gdk::Display> pDisplay=window.get_display();
		auto keys=pDisplay->map_keyval(32);
		return 0;
	}'''
code1_links = cpp.links(code1, dependencies: [gtkmm_dep], name : 'Gdk::Display::map_keyval()')
if not code1_links
	error('Can not link with Gdk::Display::map_keyval (requires gtkmm-4.0 > 4.23.2)')
endif

code2 = '''
	int main(int argc, char *argv[])
	{
		_BitInt(128) i=0x1;
		i<<126;
		return i>0;
	}'''
code2_compiles = cpp.compiles(code2, name : '_BitInt(128)')
if code2_compiles
	cpp_compiler_options += '-DCAN_USE__BitInt'
endif

code3 = '''
	#include <inplace_vector>
	int main(int argc, char *argv[])
	{
		std::inplace_vector<long, 92*66> p;
		return 0;
	}'''
code3_links = cpp.links(code3, name : 'std::inplace_vector<>')
if not code3_links
	error('Can not use std::inplace_vector<> (requires C++26, -std=gnu++26)')
endif


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

add_project_arguments(['-include', 'config.h'], language: 'cpp')

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
