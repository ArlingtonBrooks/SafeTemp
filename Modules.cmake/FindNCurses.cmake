#Find path of ncurses.h
# -> sets NCURSES_INCLUDE_DIR variable
find_path(NCURSES_INCLUDE_DIR ncurses.h
	PATHS /usr/local/include /usr/include
	)

#Find ncurses library
# -> sets NCURSES_LIBRARIES variable
find_library(NCURSES_LIBRARIES ncurses
	/usr/lib
	/usr/local/lib
	/usr/lib64
	)

#Set relevant variables
# Sets NCURSES_FOUND to TRUE if the library was found
if (NCURSES_LIBRARIES)
	if (NCURSES_INCLUDE_DIR)
		set(NCURSES_FOUND TRUE)
	endif()
else()
	set(NCURSES_FOUND FALSE)
endif()

#Export variables
# see also: https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#find-modules
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NCurses
	FOUND_VAR NCURSES_FOUND
	REQUIRED_VARS NCURSES_LIBRARIES NCURSES_INCLUDE_DIR
)
