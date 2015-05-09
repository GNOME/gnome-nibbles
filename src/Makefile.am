bin_PROGRAMS = gnome-nibbles

BUILT_SOURCES = gnome-nibbles-resources.c

gnome_nibbles_SOURCES = \
	config.vapi \
	gnome-nibbles.vala \
	$(BUILT_SOURCES)

gnome_nibbles_CFLAGS = -w `pkg-config --cflags glib-2.0` `pkg-config --cflags gtk+-3.0` `pkg-config --libs glib-2.0` `pkg-config --libs gtk+-3.0`


gnome_nibbles_CPPFLAGS = \
	-DVERSION=\"$(VERSION)\" \
	-DPKGDATADIR=\"$(datadir)/gnome-nibbles\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	$(GNOME_NIBBLES_CFLAGS)

gnome_nibbles_VALAFLAGS = \
	--pkg posix \
	--pkg gio-2.0 \
	--pkg gtk+-3.0 \
	--pkg gee-0.8 \
	--target-glib=$(GLIB_REQUIRED) \
	--gresources=$(builddir)/gnome-nibbles.gresource.xml

gnome-nibbles-resources.c: gnome-nibbles.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gnome-nibbles.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

CLEANFILES = \
	gnome-nibbles.gresource.xml \
	gnome-nibbles-resources.c

-include $(top_srcdir)/git.mk
