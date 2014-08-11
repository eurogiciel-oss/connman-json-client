#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(connman-json, 0.1)
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

PKG_PROG_PKG_CONFIG

AC_SUBST(abs_top_srcdir)
AC_SUBST(abs_top_builddir)

AC_LANG_C

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

CFLAGS=""

AC_ARG_ENABLE(optimization, AC_HELP_STRING([--disable-optimization],
			[disable code optimization through compiler]), [
	if (test "${enableval}" = "no"); then
		CFLAGS="$CFLAGS -O0 -U_FORTIFY_SOURCE"
	fi
])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],
			[enable compiling with debugging information]), [
	if (test "${enableval}" = "yes" &&
				test "${ac_cv_prog_cc_g}" = "yes"); then
		CFLAGS="$CFLAGS -g"
	fi
])

AC_CONFIG_HEADERS([config.h:config.h.in])

PKG_CHECK_MODULES(JSON, [json-c],,
	[PKG_CHECK_MODULES(JSON, [json],,
		[PKG_CHECK_MODULES(JSON, [json0],,
			[AC_MSG_WARN([No pkg-config for json-c (aka libjson0, libjson)])])
		])
	])

AC_CHECK_LIB([json], [json_object_new_string],,
	[AC_CHECK_LIB([json-c], [json_object_new_string],,
		[AC_MSG_ERROR([Cannot find required library: libjson-c (aka libjson0, libjson)])])
	])

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.4, dummy=yes,
				AC_MSG_ERROR(D-Bus >= 1.4 is required))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)


AC_CHECK_HEADER(ncurses/ncurses.h, [HAVE_NCURSES_H=1 CFLAGS="$CFLAGS -I/usr/include/ncurses"])
AC_CHECK_LIB(ncurses, initscr, HAVE_LIBNCURSES=1)
AC_CHECK_LIB(tinfo, cbreak, [LIBS="-ltinfo $LIBS"])
if test -n "$HAVE_NCURSES_H" -a -n "$HAVE_LIBNCURSES"; then
LIBS="-lncurses $LIBS"
else
PKG_CHECK_MODULES(NCURSESW, ncurses, [
LIBS="$NCURSES_LIBS $LIBS"
CFLAGS="$NCURSES_CFLAGS $CFLAGS"], [
echo -e "Error: ncurses is required." exit 1])
fi

AC_CHECK_HEADERS([ assert.h config.h ctype.h errno.h poll.h regex.h signal.h stdarg.h stdbool.h stdio.h stdlib.h string.h sys/types.h unistd.h])


AC_OUTPUT(Makefile)
