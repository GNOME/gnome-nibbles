SUBDIRS = levels pix

desktopdir = $(datadir)/applications
desktop_in_files = gnome-nibbles.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appstream_in_files = gnome-nibbles.appdata.xml.in
appstream_XML = $(appstream_in_files:.appdata.xml.in=.appdata.xml)
@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@

gsettings_SCHEMAS = org.gnome.nibbles.gschema.xml
@GSETTINGS_RULES@

dist_man_MANS = gnome-nibbles.6

dist_noinst_DATA = \
	gnome-nibbles.ui

EXTRA_DIST = \
	$(gsettings_SCHEMAS)

CLEANFILES = \
	$(appstream_XML) \
	$(desktop_DATA)

DISTCHECK_CONFIGURE_FLAGS == --enable-appdata-check

-include $(top_srcdir)/git.mk
