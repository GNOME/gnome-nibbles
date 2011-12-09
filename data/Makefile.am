SUBDIRS = levels pix

pixmapdir = $(datadir)/gnome-games/gnibbles/pixmaps
pixmap_DATA = gnibbles-logo.svg

desktopdir=$(datadir)/applications
desktop_in_files = gnibbles.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

schema_in_files = gnibbles.schemas.in
if HAVE_GNOME
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

man_MANS = gnibbles.6

EXTRA_DIST = \
	$(pixmap_DATA) 	\
	$(schema_in_files) \
	$(man_MANS)

DISTCLEANFILES = $(desktop_DATA) $(schema_DATA)

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gnibbles/data/$(schema_DATA)
endif

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/gnibbles && chmod 2555 $(DESTDIR)$(bindir)/gnibbles ;\
	fi

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

-include $(top_srcdir)/git.mk
