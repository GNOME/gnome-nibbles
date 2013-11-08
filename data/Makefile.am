SUBDIRS = icons levels pix

gsettings_SCHEMAS = org.gnome.nibbles.gschema.xml
@GSETTINGS_RULES@

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
desktop_in_files = gnibbles.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appdatadir = $(datadir)/appdata
appdata_in_files = gnome-nibbles.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

man_MANS = gnome-nibbles.6

EXTRA_DIST = \
	$(appdata_in_files) \
	$(desktop_in_files) \
	$(sounds_DATA) \
	$(gsettings_SCHEMAS) \
	$(man_MANS)

CLEANFILES = \
	$(appdata_DATA) \
	$(desktop_DATA)

check-local: $(appdata_DATA) $(desktop_DATA)
	$(APPDATA_VALIDATE) $(appdata_DATA)
	$(DESKTOP_FILE_VALIDATE) $(desktop_DATA)

-include $(top_srcdir)/git.mk
