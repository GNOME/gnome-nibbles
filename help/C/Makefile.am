gnibbles_helpdir = $(datadir)/gnome/help/gnibbles/C

figs = \
	game.png \
	graphics.png \
	intro.png \
	playing.png \
	worm_op.png

gnibbles_help_DATA = 	\
	index.html		\
	topic.dat

EXTRA_DIST = gnibbles.sgml $(gnibbles_help_DATA) $(figs)

all: index.html

# No-op if db2html failed
index.html: gnibbles/index.html
	-cd $(srcdir) && cp gnibbles/index.html index.html

gnibbles/index.html: gnibbles.sgml
	-cd $(srcdir) && db2html gnibbles.sgml

dist-hook:
	-mkdir $(distdir)/gnibbles
	-mkdir $(distdir)/gnibbles/stylesheet-images
	cp $(srcdir)/gnibbles/*.html $(distdir)/gnibbles
	cp -R $(srcdir)/gnibbles/stylesheet-images/*.png $(distdir)/gnibbles/stylesheet-images
	cp $(srcdir)/gnibbles/*.css $(distdir)/gnibbles

# No-op if db2html failed
install-data-local: index.html
	-$(mkinstalldirs) $(DESTDIR)$(gnibbles_helpdir)
	files=`echo $(srcdir)/gnibbles/*.html $(srcdir)/gnibbles/*.css $(srcdir)/*.png`; \
	if test "$$files" != '$(srcdir)/gnibbles/*.html $(srcdir)/gnibbles/*.css $(srcdir)/*.png'; then \
	  for file in $$files; do \
	    basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $$file $(DESTDIR)$(gnibbles_helpdir)/$$basefile; \
	  done; \
	fi
	-$(mkinstalldirs) $(DESTDIR)$(gnibbles_helpdir)/stylesheet-images
	files=`echo $(srcdir)/gnibbles/stylesheet-images/*.png`; \
	if test "$$files" != '$(srcdir)/gnibbles/stylesheet-images/*.png'; then \
	  for file in $$files; do \
	    basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $$file $(DESTDIR)$(gnibbles_helpdir)/stylesheet-images/$$basefile; \
	  done; \
	fi

gnibbles.ps: gnibbles.sgml
	db2ps $<

gnibbles.rtf: gnibbles.sgml
	db2rtf $<
