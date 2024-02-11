cmake_minimum_required (VERSION 3.5)
project(SafeTemp)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Modules.cmake/")

find_package(Sensors REQUIRED)
find_package(NCurses REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(NCURSES REQUIRED ncurses)
find_program(GNUPLOT_FOUND 
	NAMES gnuplot
	PATHS /usr/bin /usr/local/bin
	NO_CACHE
	REQUIRED)

if (SENSORS_FOUND)
	add_compile_definitions(HAVE_LIBSENSORS)
	message("Located lm_sensors")
else()
	message("Unable to locate lm_sensors (via sensors.h)")
endif()

if (NCURSES_FOUND)
	message("Located ncurses")
else()
	message("Unable to locate ncurses (via ncurses.h)")
endif()

if (GNUPLOT_FOUND)
	add_compile_definitions(HAVE_GNUPLOT)
	message("Located gnuplot")
else()
	message("Unable to locate gnuplot")
endif()

if (GTK_FOUND)
	add_compile_definitions(HAVE_GTK)
	message("Located GTK+ V3")
else()
	message("Unable to locate GTK V3")
endif()

if (SENSORS_FOUND AND GNUPLOT_FOUND AND GTK_FOUND)
	add_executable(SafeTemp SafeTemp.cpp)
else()
	message("Compile requirements not satisfied")
endif()

target_link_libraries(SafeTemp PRIVATE ${SENSORS_LIBRARIES})
target_link_libraries(SafeTemp PRIVATE ${NCURSES_LIBRARIES})
#target_link_libraries(SafeTemp PRIVATE ${GTK_LIBRARIES})
target_link_libraries(SafeTemp PRIVATE ${GTK_LDFLAGS})
target_compile_options(SafeTemp PRIVATE ${GTK_CFLAGS})
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build completed successfully!"
)
#target_include_directories(SafeTemp PRIVATE ${GTK_INCLUDE_DIRS})
