SUBDIRS = pix

if BUILD_HELP
SUBDIRS += help
endif

NULL =

bin_PROGRAMS = gnibbles
ggzexecmod_PROGRAMS =

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
	board.c \
	board.h \
	level.c \
	level.h \
	worm-clutter.c \
	worm-clutter.h \
	$(NULL)

gnibbles_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnibbles_CFLAGS = \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(CLUTTER_GTK_CFLAGS) \
	$(AM_CFLAGS)

gnibbles_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(CLUTTER_GTK_LIBS) \
	$(INTLLIBS)

if HAVE_GNOME
gnibbles_CFLAGS += $(GNOME_CFLAGS)
gnibbles_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
gnibbles_CFLAGS += $(RSVG_CFLAGS)
gnibbles_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
gnibbles_CFLAGS += $(GHTREAD_CFLAGS)
gnibbles_LDADD += $(GTHREAD_LIBS)
endif

if GGZ_CLIENT
gnibbles_SOURCES += \
	ggz-network.h \
	ggz-network.c \
	$(NULL)

gnibbles_CPPFLAGS += \
	$(GGZ_GTK_INCLUDES) \
	$(GGZMOD_INCLUDES) \
	$(GGZCORE_INCLUDES) \
	$(LIBGGZ_INCLUDES)

gnibbles_LDADD += \
	$(LIB_GGZMOD) \
	$(LIB_GGZ_GTK)
endif

pixmapdir = $(datadir)/gnome-games/gnibbles/pixmaps
pixmap_DATA = gnibbles-logo.svg

configdir = $(datadir)/gnome-games/gnibbles/games
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

schema_in_files = gnibbles.schemas.in
if HAVE_GNOME
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

man_MANS = gnibbles.6

if GGZ_SERVER
ggzexecmod_PROGRAMS += gnibblesd

gnibblesd_SOURCES = \
	server.h \
	server.c \
	$(NULL)

gnibblesd_CPPFLAGS = \
	-I$(top_srcdir) \
	$(GGZDMOD_INCLUDES)
	$(AM_CPPFLAGS)

gnibblesd_CFLAGS = \
	$(AM_CFLAGS)

gnibblesd_LDFLAGS = $(AM_LDFLAGS)

gnibblesd_LDADD = \
	$(INTLLIBS)			\
	$(LIB_GGZDMOD)

ggzroom_in_files = gnibbles.room.in
ggzroom_DATA = $(ggzroom_in_files:.room.in=.room)
ggzroomdir = ${ggzdconfdir}/rooms/

ggzserver_dscdir = ${ggzdconfdir}/games/
ggzserver_dsc_in_files = gnibbles-server.dsc.in
ggzserver_dsc_DATA = $(ggzserver_dsc_in_files:.dsc.in=.dsc)
endif # GGZ_SERVER

if GGZ_CLIENT
ggzclient_dsc_in_files = gnibbles-client.dsc.in
endif # GGZ_CLIENT

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(config_DATA) \
	$(schema_in_files) \
	$(man_MANS) \
	gnibbles-client.dsc.in \
	gnibbles-server.dsc.in \
	gnibbles.room.in

CLEANFILES = \
	$(ggzroom_DATA) \
	$(ggzserver_dsc_in_files:.dsc.in=.dsc) \
	$(ggzclient_dsc_in_files:.dsc.in=.dsc) \
	$(NULL)

DISTCLEANFILES = $(desktop_DATA) $(schema_DATA)

SCOREFILES = 4.0 3.0 2.0 1.0 4.1 3.1 2.1 1.1

install-data-local: $(ggzclient_dsc_in_files:.dsc.in=.dsc)
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

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ;\
	fi

uninstall-local:
if GGZ_CLIENT
	-$(GGZ_CONFIG) -D --remove --modfile=gnibbles-client.dsc
endif

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@
@GNOME_GAMES_GGZ_INTLTOOL_ROOM_RULE@
@GNOME_GAMES_GGZ_DSC_RULE@
