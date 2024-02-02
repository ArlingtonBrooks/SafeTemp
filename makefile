BUILDFLAGS=`pkg-config --libs --cflags gtk+-3.0`
all:
	g++ -g -O0 -fsanitize=undefined -fsanitize=address -DHAVE_GTK -DHAVE_GNUPLOT -DHAVE_LIBSENSORS -lsensors -lncurses $(BUILDFLAGS) SafeTemp.cpp
