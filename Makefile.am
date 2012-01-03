SUBDIRS = src data help

SCOREFILES = 4.0 3.0 2.0 1.0 4.1 3.1 2.1 1.1

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in ${SCOREFILES} ; do \
	   touch $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	   chmod 664 $(DESTDIR)$(scoredir)/gnibbles.$$i.scores; \
	 done

-include $(top_srcdir)/git.mk
