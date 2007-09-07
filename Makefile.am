pixmapdir=$(datadir)/pixmaps/gnibbles
configdir=$(datadir)/gnibbles
Gamesdir=$(datadir)/applications

INCLUDES =		 			\
	$(GNOME_GAMES_CFLAGS)			\
	-I$(top_srcdir)/libgames-support	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DREAL_DATADIR=\""$(datadir)"\" 	\
	$(GGZMOD_INCLUDES) $(GGZ_GTK_INCLUDES) $(GGZCORE_INCLUDES)\
	$(LIBGGZ_INCLUDES) $(GGZCORE_INCLUDES) $(GGZDMOD_INCLUDES)

if GGZ_SERVER
bin_PROGRAMS = gnibbles gnibblesd 
gnibblesd_SOURCES = server.h   \
		 server.c   

gnibblesd_LDFLAGS = @GGZDMOD_LDFLAGS@

gnibblesd_LDADD = \
	$(GNOME_GAMES_LIBS)     \
	$(INTLLIBS) 		\
	@LIB_GGZDMOD@

else
bin_PROGRAMS = gnibbles 
endif

if GGZ_CLIENT
GGZ_FILES =	ggz-network.h \
		ggz-network.c \
		$(NULL)
endif

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
	main.h \
	$(GGZ_FILES)

gnibbles_LDADD = \
	../libgames-support/libgames-support.la \
	$(GNOME_GAMES_LIBS) \
	$(INTLLIBS) \
	$(LIB_GGZMOD) \
	$(LIB_GGZ_GTK)

gnibbles_LDFLAGS = $(GGZMOD_LDFLAGS)

pixmap_DATA = gnibbles-logo.svg

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

Games_in_files = gnibbles.desktop.in.in
Games_DATA = $(Games_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gnibbles.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(config_DATA) \
	$(schema_in_files) \
	gnibbles-client.dsc.in \
	gnibbles-server.dsc.in \
        gnibbles.room          

SUBDIRS= help pix

DISTCLEANFILES = $(Games_DATA) $(schema_DATA)

SCOREFILES = 4.0 3.0 2.0 1.0 4.1 3.1 2.1 1.1

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gnibbles/$(schema_DATA)
endif
if GGZ_CLIENT
	$(GGZ_CONFIG) -D --install --modfile=gnibbles-client.dsc --force
endif

	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	 done
	-if test "x$(setgid)" = "xtrue"; then chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ; fi
uninstall-local:
if GGZ_CLIENT
	-$(GGZ_CONFIG) -D --remove --modfile=gnibbles-client.dsc
endif


if GGZ_SERVER
ggzroom_DATA = gnibbles.room
ggzroomdir = ${prefix}/etc/ggzd/rooms/

ggzgame_DATA = gnibbles-server.dsc
ggzgamedir = ${prefix}/etc/ggzd/games/
endif


