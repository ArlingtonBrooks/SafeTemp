#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tempsafe], [0.3])
AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADERS([config.h])

GTK_REQUIRED=3.10.0

# Checks for programs.
AC_PROG_CXX
#AC_CHECK_PROG(gnuplot,AC_DEFINE([HAVE_GNUPLOT],1,[yes]),AC_DEFINE([HAVE_GNUPLOT],0,[no]))
AC_CHECK_PROG(GNUPLOT,gnuplot,true,false)
if test $GNUPLOT = "true"; then
    AC_DEFINE([HAVE_GNUPLOT],[1],[Gnuplot])
else
    AC_DEFINE([HAVE_GNUPLOT],[0],[Gnuplot])
fi

# Checks for libraries.
PKG_PROG_PKG_CONFIG([0.29]) #check for pkg-config
PKG_CHECK_MODULES(GTK,[gtk+-3.0 >= $GTK_REQUIRED],AC_DEFINE([HAVE_GTK],[1],[GTK]),AC_DEFINE([HAVE_GTK],[0],[GTK]))

AC_CHECK_LIB([ncurses], [endwin])
AC_CHECK_LIB([sensors], [sensors_get_detected_chips])
AC_CHECK_LIB([nvidia-ml],[nvmlDeviceGetCount])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday strerror])

AM_INIT_AUTOMAKE()

#AC_CONFIG_FILES([Makefile])
AC_OUTPUT([Makefile])
