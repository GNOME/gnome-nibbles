pixmapdir=$(datadir)/pixmaps/gnibbles
configdir=$(datadir)/gnibbles
appicondir=$(datadir)/pixmaps
soundlistdir = $(sysconfdir)/sound/events
Gamesdir=$(datadir)/gnome/apps/Games

INCLUDES = 			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir)				 \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = gnibbles

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
	main.c

gnibbles_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)	\
	$(INTLLIBS)

pixmap_DATA = \
	gnibbles.png \
	gnibbles_logo.png

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

Games_DATA = gnibbles.desktop

soundlist_DATA = gnibbles.soundlist

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(Games_DATA) \
	$(config_DATA) \
	$(soundlist_DATA)

#EXTRA_DIST = README \
#             AUTHORS \
#             gnobots2.desktop \
#             gnobots2.soundlist \
#             $(appicon_DATA) \
#             $(pixmap_DATA)
