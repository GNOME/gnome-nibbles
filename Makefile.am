pixmapdir=$(datadir)/pixmaps/gnibbles
configdir=$(datadir)/gnibbles
icondir=$(datadir)/pixmaps
soundlistdir = $(sysconfdir)/sound/events
Gamesdir=$(datadir)/applications
scoredir=$(localstatedir)/games

INCLUDES = 			\
	$(GNOME_GAMES_CFLAGS)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DREAL_DATADIR=\""$(datadir)"\" \
	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED


bin_PROGRAMS = gnibbles

icon_DATA = gnome-nibbles.png

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
	properties.h \
	properties.c \
	preferences.h \
	preferences.c \
	scoreboard.h \
	scoreboard.c \
	main.c \
	main.h

gnibbles_LDADD = \
	$(GNOME_GAMES_LIBS) \
	$(INTLLIBS)

pixmap_DATA = \
	gnibbles.png \
	gnibbles-logo.png

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

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(Games_in_files) \
	$(config_DATA) \
	$(soundlist_in_files) \
	$(icon_DATA)

SUBDIRS=sounds help

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/gnibbles.4.0.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.4.0.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.4.0.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.3.0.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.3.0.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.3.0.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.2.0.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.2.0.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.2.0.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.1.0.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.1.0.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.1.0.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.4.1.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.4.1.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.4.1.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.3.1.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.3.1.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.3.1.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.2.1.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.2.1.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.2.1.scores
	-touch $(DESTDIR)$(scoredir)/gnibbles.1.1.scores
	-chown games.games $(DESTDIR)$(scoredir)/gnibbles.1.1.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gnibbles.1.1.scores
	-chgrp games $(DESTDIR)$(bindir)/gnibbles && chmod 2111 $(DESTDIR)$(bindir)/gnibbles
