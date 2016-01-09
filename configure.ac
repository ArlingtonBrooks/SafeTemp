#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tempsafe], [0.2])
AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
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
