AC_PREREQ([2.63])
AC_INIT([GNOME Nibbles], [3.17.90], [https://bugzilla.gnome.org/],
        [gnome-nibbles], [https://gnome.org.wiki/Apps/Nibbles])
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])
GNOME_MAINTAINER_MODE_DEFINES

AC_PROG_CC
AC_PROG_CXX

AM_PROG_VALAC([0.26.0.91])
AM_PROG_CC_C_O

LT_PREREQ([2.4])
LT_INIT

GLIB_GSETTINGS

AC_CONFIG_MACRO_DIR([m4])

dnl ###########################################################################
dnl Dependencies
dnl ###########################################################################

GLIB_REQUIRED=2.40.0
GTK_REQUIRED=3.14.0
RSVG_REQUIRED=2.32.0
CANBERRA_GTK_REQUIRED=0.26
CLUTTER_REQUIRED=1.0.0
CLUTTER_GTK_REQUIRED=0.91.6

PKG_CHECK_MODULES(GNOME_NIBBLES, [
  glib-2.0 >= $GLIB_REQUIRED
  gio-2.0 >= $GLIB_REQUIRED
  gtk+-3.0 >= $GTK_REQUIRED
  librsvg-2.0 >= $RSVG_REQUIRED
  libcanberra-gtk3 >= $CANBERRA_GTK_REQUIRED
  clutter-1.0 >= $CLUTTER_REQUIRED
  clutter-gtk-1.0 >= $CLUTTER_REQUIRED
  gee-0.8
])

AC_SUBST([GLIB_REQUIRED])

AC_CHECK_LIB([m],[floor])

dnl ###########################################################################
dnl GResources
dnl ###########################################################################

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)

dnl ###########################################################################
dnl Files to generate
dnl ###########################################################################

AC_CONFIG_FILES([
Makefile
data/Makefile
src/Makefile
src/gnome-nibbles.gresource.xml
])
AC_OUTPUT
