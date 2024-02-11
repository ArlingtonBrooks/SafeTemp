#Find path of sensors.h
# -> sets SENSORS_INCLUDE_DIR variable
find_path(SENSORS_INCLUDE_DIR sensors.h
	PATHS /usr/local/include /usr/include
	PATH_SUFFIXES sensors
	)

#Find sensors library
# -> sets SENSORS_LIBRARIES variable
find_library(SENSORS_LIBRARIES sensors
	/usr/lib
	/usr/local/lib
	/usr/lib64
	)

#Set relevant variables
# Sets SENSORS_FOUND to TRUE if the library was found
if (SENSORS_LIBRARIES)
	if (SENSORS_INCLUDE_DIR)
		set(SENSORS_FOUND TRUE)
	endif()
else()
	set(SENSORS_FOUND FALSE)
endif()

#Export variables
# see also: https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#find-modules
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sensors
	FOUND_VAR SENSORS_FOUND
	REQUIRED_VARS SENSORS_LIBRARIES SENSORS_INCLUDE_DIR
)
