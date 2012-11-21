SUBDIRS = data help po src

SCOREFILES = 4.0 3.0 2.0 1.0 4.1 3.1 2.1 1.1

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gnome-nibbles.$$i.scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gnome-nibbles.$$i.scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gnome-nibbles.$$i.scores; \
	 done

uninstall-local:
	-for i in ${SCOREFILES} ; do \
	   rm -f $(DESTDIR)$(scoredir)/gnome-nibbles.$$i.scores; \
	 done

-include $(top_srcdir)/git.mk
