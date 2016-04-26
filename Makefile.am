AUTOMAKE_OPTIONS = foreign

CXXFLAGS=-std=c++11
ACLOCAL_AMFLAGS = -I config

bin_PROGRAMS = tempsafe
tempsafe_SOURCES = SafeTemp.cpp
LDFLAGS=-lsensors -lncurses -lpthread
AM_CPPFLAGS = ${GTK_CFLAGS}
tempsafe_LDADD = ${GTK_LIBS}
