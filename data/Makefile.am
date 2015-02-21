SUBDIRS = icons levels pix

gsettings_SCHEMAS = org.gnome.nibbles.gschema.xml
@GSETTINGS_RULES@

dist_noinst_DATA = \
	nibbles-menus.ui

soundsdir = $(datadir)/gnome-nibbles/sounds
sounds_DATA = \
	appear.ogg \
	bonus.ogg \
	crash.ogg \
	gameover.ogg \
	gobble.ogg \
	life.ogg \
	reverse.ogg \
	teleport.ogg

desktopdir=$(datadir)/applications
desktop_in_files = gnome-nibbles.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appstream_in_files = gnome-nibbles.appdata.xml.in
appstream_XML = $(appdata_in_files:.xml.in=.xml)
@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@

man_MANS = gnome-nibbles.6

EXTRA_DIST = \
	$(appstream_in_files) \
	$(desktop_in_files) \
	$(sounds_DATA) \
	$(gsettings_SCHEMAS) \
	$(man_MANS)

CLEANFILES = \
	$(appstream_XML) \
	$(desktop_DATA)

check-local: $(desktop_DATA)
	$(DESKTOP_FILE_VALIDATE) $(desktop_DATA)

DISTCHECK_CONFIGURE_FLAGS = --enable-appdata-check

-include $(top_srcdir)/git.mk
