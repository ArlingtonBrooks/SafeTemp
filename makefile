BUILDFLAGS=`pkg-config --libs --cflags gtk+-3.0`
#SANFLAGS=-fsanitize=address -fsanitize=undefined
#WARNFLAGS=-Wall -Wextra -Wpedantic
main:
	g++ $(WARNFLAGS) $(SANFLAGS) -g -O0 -DHAVE_GTK -DHAVE_GNUPLOT -DHAVE_LIBSENSORS -lsensors -lncurses $(BUILDFLAGS) SafeTemp.cpp

ui_test:
	g++ $(WARNFLAGS) $(SANFLAGS) -g -O0 -DUI_TEST -DHAVE_GTK -DHAVE_GNUPLOT -DHAVE_LIBSENSORS -lsensors -lncurses $(BUILDFLAGS) SafeTemp.cpp
