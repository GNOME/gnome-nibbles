SUBDIRS = icons levels pix

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.gnome.Nibbles.service

org.gnome.Nibbles.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		     echo 'Name=org.gnome.Nibbles'; \
		     echo 'Exec=${bindir}/gnome-nibbles --gapplication-service') > $@.tmp && \
		    mv $@.tmp $@

desktopdir = $(datadir)/applications
desktop_in_files = org.gnome.Nibbles.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appstream_in_files = org.gnome.Nibbles.appdata.xml.in
appstream_XML = $(appstream_in_files:.appdata.xml.in=.appdata.xml)
@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@

gsettings_SCHEMAS = org.gnome.nibbles.gschema.xml
@GSETTINGS_RULES@

dist_man_MANS = gnome-nibbles.6

dist_noinst_DATA = \
	nibbles-menus.ui \
	nibbles.ui \
	nibbles.css \
	scoreboard.ui \
	player-score-box.ui \
	controls-grid.ui \
	preferences-dialog.ui

soundsdir = $(datadir)/gnome-nibbles/sounds
dist_sounds_DATA = \
    appear.ogg \
    bonus.ogg \
    crash.ogg \
    gameover.ogg \
    gobble.ogg \
    life.ogg \
    reverse.ogg \
    teleport.ogg

EXTRA_DIST = \
	$(appstream_in_files) \
	$(dbusservice_DATA) \
	$(desktop_in_files) \
	$(gsettings_SCHEMAS)

CLEANFILES = \
	$(appstream_XML) \
	$(dbusservice_DATA) \
	$(desktop_DATA)

AM_DISTCHECK_CONFIGURE_FLAGS == --enable-appdata-check

-include $(top_srcdir)/git.mk
