bin_PROGRAMS = gnome-nibbles

BUILT_SOURCES = gnome-nibbles-resources.c

gnome_nibbles_SOURCES = \
	config.vapi \
	gnome-nibbles.vala \
	nibbles-view.vala \
	nibbles-game.vala \
	worm.vala \
	boni.vala \
	warp.vala \
	preferences-dialog.vala \
	$(BUILT_SOURCES)

gnome_nibbles_CFLAGS = -w

gnome_nibbles_CPPFLAGS = \
	-DVERSION=\"$(VERSION)\" \
	-DPKGDATADIR=\"$(datadir)/gnome-nibbles\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DSOUND_DIRECTORY=\"$(datadir)/gnome-nibbles/sounds\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	$(GNOME_NIBBLES_CFLAGS)

gnome_nibbles_VALAFLAGS = \
	--pkg posix \
	--pkg gio-2.0 \
	--pkg gtk+-3.0 \
	--pkg clutter-1.0 \
	--pkg clutter-gtk-1.0 \
	--pkg libcanberra \
	--pkg libcanberra-gtk \
	--pkg libgnome-games-support-1 \
	--pkg gee-0.8 \
	--target-glib=$(GLIB_REQUIRED) \
	--gresources=$(top_srcdir)/data/gnome-nibbles.gresource.xml

gnome_nibbles_LDADD = \
	$(GNOME_NIBBLES_LIBS)

resources = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(top_srcdir)/data $(top_srcdir)/data/gnome-nibbles.gresource.xml)

gnome-nibbles-resources.c: $(top_srcdir)/data/gnome-nibbles.gresource.xml $(resources)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/data --generate-source $<

CLEANFILES = \
	gnome-nibbles-resources.c

-include $(top_srcdir)/git.mk
