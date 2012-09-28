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
	games-controls.h \
	games-controls.c \
	games-fullscreen-action.h \
	games-fullscreen-action.c \
	games-gridframe.h \
	games-gridframe.c \
	games-pause-action.h \
	games-pause-action.c \
	games-score.h			\
	games-score.c			\
	games-scores.c			\
	games-scores.h			\
	games-scores-dialog.c	\
	games-scores-dialog.h	\
	games-scores-backend.c	\
	games-scores-backend.h	\
	games-stock.h \
	games-stock.c \
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

if ENABLE_SETGID
gnibbles_SOURCES +=		\
	games-setgid-io.c		\
	games-setgid-io.h
endif # ENABLE_SETGID

gnibbles_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnibbles_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/gnibbles\" \
	-DSOUND_DIRECTORY=\"$(datadir)/gnibbles/sounds\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	-DSCORESDIR="\"$(scoredir)\""				\
	$(GTK_CFLAGS) \
	$(RSVG_CFLAGS) \
	$(CANBERRA_GTK_CFLAGS) \
	$(CLUTTER_GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(AM_CFLAGS)

gnibbles_LDADD = \
	$(GTK_LIBS) \
	$(RSVG_LIBS) \
	$(CANBERRA_GTK_LIBS) \
	$(CLUTTER_GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(INTLLIBS)

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ;\
	fi

-include $(top_srcdir)/git.mk
