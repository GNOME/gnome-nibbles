bin_PROGRAMS = gnome-nibbles

gnome_nibbles_SOURCES = \
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
	games-score.h			\
	games-score.c			\
	games-scores.c			\
	games-scores.h			\
	games-scores-dialog.c	\
	games-scores-dialog.h	\
	games-scores-backend.c	\
	games-scores-backend.h	\
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

gnome_nibbles_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

gnome_nibbles_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/gnome-nibbles\" \
	-DSOUND_DIRECTORY=\"$(datadir)/gnome-nibbles/sounds\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	$(GNOME_NIBBLES_CFLAGS)

gnome_nibbles_LDADD = \
	$(GNOME_NIBBLES_LIBS)

-include $(top_srcdir)/git.mk
