AC_PREREQ([2.63])
AC_INIT([GNOME Nibbles], [3.22.2.1], [https://bugzilla.gnome.org/],
        [gnome-nibbles], [https://gnome.org.wiki/Apps/Nibbles])
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

dnl srcdir != builddir from git is broken with vala, hitting
dnl issues with valac --gresources.
echo \#buildapi-variable-no-builddir >/dev/null

AC_PROG_CC

AM_PROG_VALAC([0.28.0])

GLIB_GSETTINGS

APPSTREAM_XML

AC_CONFIG_MACRO_DIR([m4])

dnl ###########################################################################
dnl Dependencies
dnl ###########################################################################

GLIB_REQUIRED=2.40.0
GTK_REQUIRED=3.18.0
RSVG_REQUIRED=2.32.0
CANBERRA_GTK_REQUIRED=0.26
CLUTTER_REQUIRED=1.22.0
CLUTTER_GTK_REQUIRED=1.4.0

PKG_CHECK_MODULES(GNOME_NIBBLES, [
  clutter-1.0 >= $CLUTTER_REQUIRED
  clutter-gtk-1.0 >= $CLUTTER_GTK_REQUIRED
  gee-0.8
  gio-2.0 >= $GLIB_REQUIRED
  glib-2.0 >= $GLIB_REQUIRED
  gtk+-3.0 >= $GTK_REQUIRED
  libcanberra-gtk3 >= $CANBERRA_GTK_REQUIRED
  libgnome-games-support-1
])

AC_SUBST([GLIB_REQUIRED])

dnl ###########################################################################
dnl GResources
dnl ###########################################################################

AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])

dnl ###########################################################################
dnl Internationalization
dnl ###########################################################################

IT_PROG_INTLTOOL([0.50.2])
AC_SUBST([GETTEXT_PACKAGE], [gnome-nibbles])

dnl ###########################################################################
dnl Documentation
dnl ###########################################################################

YELP_HELP_INIT

dnl ###########################################################################
dnl Files to generate
dnl ###########################################################################

AC_CONFIG_FILES([
Makefile
data/Makefile
data/icons/Makefile
data/icons/hicolor/Makefile
data/levels/Makefile
data/pix/Makefile
help/Makefile
po/Makefile.in
src/Makefile
])
AC_OUTPUT
