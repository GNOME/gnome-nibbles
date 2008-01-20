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
	main.c \
	main.h \
	$(NULL)

gnibbles_CPPFLAGS = \
	-I$(top_srcdir)/libgames-support \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DREAL_DATADIR=\""$(datadir)"\" \
	$(AM_CPPFLAGS)

gnibbles_CFLAGS = \
	$(GNOME_GAMES_CFLAGS)			\
	$(AM_CFLAGS)

gnibbles_LDFLAGS = $(AM_LDFLAGS)

gnibbles_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GNOME_GAMES_LIBS) \
	$(INTLLIBS)

if GGZ_CLIENT
gnibbles_SOURCES += \
	ggz-network.h \
	ggz-network.c \
	$(NULL)

gnibbles_CPPFLAGS += \
	$(GGZMOD_INCLUDES) \
	$(GGZ_GTK_INCLUDES) \
	$(GGZCORE_INCLUDES) \
	$(LIBGGZ_INCLUDES) \
	$(GGZCORE_INCLUDES) \
	$(GGZDMOD_INCLUDES)

gnibbles_LDADD += \
	$(LIB_GGZMOD) \
	$(LIB_GGZ_GTK)

gnibbles_LDFLAGS += $(GGZMOD_LDFLAGS)
endif

pixmapdir = $(datadir)/pixmaps/gnibbles
pixmap_DATA = gnibbles-logo.svg

configdir = $(datadir)/gnibbles
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
	level026.gnl \
	$(NULL)

desktopdir=$(datadir)/applications
desktop_in_files = gnibbles.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = gnibbles.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GGZ_SERVER
bin_PROGRAMS += gnibblesd

gnibblesd_SOURCES =
	server.h \
	server.c \
	$(NULL)

gnibblesd_CPPFLAGS = \
	-I$(top_srcdir)/libgames-support \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DREAL_DATADIR=\""$(datadir)"\" \
	$(GGZMOD_INCLUDES) $(GGZ_GTK_INCLUDES) $(GGZCORE_INCLUDES)\
	$(LIBGGZ_INCLUDES) $(GGZCORE_INCLUDES) $(GGZDMOD_INCLUDES)
	$(AM_CPPFLAGS)

gnibblesd_CFLAGS = \
	$(GNOME_GAMES_CFLAGS)			\
	$(AM_CFLAGS)

gnibblesd_LDFLAGS = $(AM_LDFLAGS) $(GGZDMOD_LDFLAGS)

gnibblesd_LDADD = \
	$(GNOME_GAMES_LIBS)     \
	$(INTLLIBS) 		\
	$(LIB_GGZDMOD)

ggzroom_in_files = gnibbles.room.in
ggzroom_DATA = $(ggzroom_in_files:.room.in=.room)
ggzroomdir = ${prefix}/etc/ggzd/rooms/
@GNOME_GAMES_GGZ_INTLTOOL_ROOM_RULE@

ggzgame_DATA = gnibbles-server.dsc
ggzgamedir = ${prefix}/etc/ggzd/games/

endif # GGZ_SERVER

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(config_DATA) \
	$(schema_in_files) \
	gnibbles-client.dsc.in \
	gnibbles-server.dsc.in \
        gnibbles.room          

SUBDIRS= help pix

CLEANFILES = $(ggzroom_DATA)

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
