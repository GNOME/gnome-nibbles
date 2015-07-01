bin_PROGRAMS = gnome-nibbles

BUILT_SOURCES = gnome-nibbles-resources.c

gnome_nibbles_SOURCES = \
	config.vapi \
	gnome-nibbles.vala \
	nibbles-view.vala \
	nibbles-game.vala \
	worm.vala \
	boni.vala \
	$(BUILT_SOURCES)

gnome_nibbles_CFLAGS = -w

gnome_nibbles_CPPFLAGS = \
	-DVERSION=\"$(VERSION)\" \
	-DPKGDATADIR=\"$(datadir)/gnome-nibbles\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	$(GNOME_NIBBLES_CFLAGS)

gnome_nibbles_VALAFLAGS = \
	--pkg posix \
	--pkg gio-2.0 \
	--pkg gtk+-3.0 \
	--pkg clutter-1.0 \
	--pkg clutter-gtk-1.0 \
	--pkg libcanberra \
	--pkg libcanberra-gtk \
	--pkg libgames-support-1.0 \
	--pkg gee-0.8 \
	--target-glib=$(GLIB_REQUIRED) \
	--gresources=$(builddir)/gnome-nibbles.gresource.xml

gnome_nibbles_LDADD = \
	$(GNOME_NIBBLES_LIBS)

gnome-nibbles-resources.c: gnome-nibbles.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gnome-nibbles.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

CLEANFILES = \
	gnome-nibbles.gresource.xml \
	gnome-nibbles-resources.c

-include $(top_srcdir)/git.mk
