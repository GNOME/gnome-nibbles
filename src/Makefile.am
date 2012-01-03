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
	sound.c \
	sound.h \
	worm.c \
	worm.h

gnibbles_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnibbles_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/gnibbles\" \
	-DSOUND_DIRECTORY=\"$(pkgdatadir)/sounds\" \
	$(GTK_CFLAGS) \
	$(CANBERRA_GTK_CFLAGS) \
	$(CLUTTER_GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(AM_CFLAGS)

gnibbles_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS) \
	$(CANBERRA_GTK_LIBS) \
	$(CLUTTER_GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(INTLLIBS)

if HAVE_RSVG
gnibbles_CFLAGS += $(RSVG_CFLAGS)
gnibbles_LDADD += $(RSVG_LIBS)
endif

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ;\
	fi

-include $(top_srcdir)/git.mk
