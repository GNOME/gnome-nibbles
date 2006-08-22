pixmapdir=$(datadir)/pixmaps/gnibbles
configdir=$(datadir)/gnibbles
icondir=$(datadir)/icons/hicolor/48x48/apps/
svgicondir = $(datadir)/icons/hicolor/scalable/apps/
soundlistdir = $(sysconfdir)/sound/events
Gamesdir=$(datadir)/applications

INCLUDES =		 			\
	$(GNOME_GAMES_CFLAGS)			\
	-I$(top_srcdir)/libgames-support	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DREAL_DATADIR=\""$(datadir)"\"

bin_PROGRAMS = gnibbles

icon_DATA = gnome-nibbles.png
svgicon_DATA = gnome-nibbles.svg

gnibbles_SOURCES = \
	bonus.h \
	bonus.c \
	boni.h \
	boni.c \
	warp.h \
	warp.c \
	warpmanager.h \
	warpmanager.c \
	worm.h \
	worm.c \
	gnibbles.h \
	gnibbles.c \
	network.c \
	network.h \
	properties.h \
	properties.c \
	preferences.h \
	preferences.c \
	scoreboard.h \
	scoreboard.c \
	main.c \
	main.h

gnibbles_LDADD = \
	../libgames-support/libgames-support.la \
	$(GNOME_GAMES_LIBS) \
	$(AVAHI_LIBS) \
	$(BONJOUR_LIBS) \
	$(HOWL_LIBS) \
	$(INTLLIBS)

pixmap_DATA = \
	gnibbles.png \
	gnibbles-logo.svg

config_DATA = \
	level001.gnl \
	level002.gnl \
	level003.gnl \
	level004.gnl \
	level005.gnl \
	level006.gnl \
	level007.gnl \
	level008.gnl \
	level009.gnl \
	level010.gnl \
	level011.gnl \
	level012.gnl \
	level013.gnl \
	level014.gnl \
	level015.gnl \
	level016.gnl \
	level017.gnl \
	level018.gnl \
	level019.gnl \
	level020.gnl \
	level021.gnl \
	level022.gnl \
	level023.gnl \
	level024.gnl \
	level025.gnl \
	level026.gnl

Games_in_files = gnibbles.desktop.in
Games_DATA = $(Games_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

soundlist_in_files = gnibbles.soundlist.in
soundlist_DATA = $(soundlist_in_files:.soundlist.in=.soundlist)
@INTLTOOL_SOUNDLIST_RULE@

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gnibbles.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(Games_in_files) \
	$(config_DATA) \
	$(soundlist_in_files) \
	$(schema_in_files) \
	$(icon_DATA)

SUBDIRS=sounds help pix

DISTCLEANFILES = $(Games_DATA) $(soundlist_DATA) $(schema_DATA)

SCOREFILES = 4.0 3.0 2.0 1.0 4.1 3.1 2.1 1.1

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gnibbles/$(schema_DATA)
endif
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	 done
	-if test "x$(setgid)" = "xtrue"; then chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ; fi
