# Try to find the libubertooth library
#
# Once done this defines:
#  LIBUBERTOOTH_FOUND - system has libubertooth
#  LIBUBERTOOTH_INCLUDE_DIR - the libubertooth include directory
#  LIBUBERTOOTH_LIBRARIES - Link these to use libubertooth
#
# Copyright (c) 2013  Dominic Spill


if (LIBUBERTOOTH_INCLUDE_DIR AND LIBUBERTOOTH_LIBRARIES)

  # in cache already
  set(LIBUBERTOOTH_FOUND TRUE)

else (LIBUBERTOOTH_INCLUDE_DIR AND LIBUBERTOOTH_LIBRARIES)
  IF (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBUBERTOOTH QUIET libubertooth)
  ENDIF(NOT WIN32)

  FIND_PATH(LIBUBERTOOTH_INCLUDE_DIR
    NAMES ubertooth.h
    HINTS $ENV{LIBUBERTOOTH_DIR}/include ${PC_LIBUBERTOOTH_INCLUDEDIR}
    PATHS /usr/include /usr/local/include /opt/local/include
    ${CMAKE_SOURCE_DIR}/../libubertooth/src
    ${LIBUBERTOOTH_INCLUDE_DIR}
  )

  set(libubertooth_library_names ubertooth)

  FIND_LIBRARY(LIBUBERTOOTH_LIBRARIES
    NAMES ${libubertooth_library_names}
    HINTS $ENV{LIBUBERTOOTH_DIR}/lib ${PC_LIBUBERTOOTH_LIBDIR}
    PATHS /usr/local/lib /usr/lib /opt/local/lib ${PC_LIBUBERTOOTH_LIBDIR} ${PC_LIBUBERTOOTH_LIBRARY_DIRS} ${CMAKE_SOURCE_DIR}/../libubertooth/src
  )

  if(LIBUBERTOOTH_INCLUDE_DIR)
    set(CMAKE_REQUIRED_INCLUDES ${LIBUBERTOOTH_INCLUDE_DIR})
  endif()

  if(LIBUBERTOOTH_LIBRARIES)
    set(CMAKE_REQUIRED_LIBRARIES ${LIBUBERTOOTH_LIBRARIES})
  endif()

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUBERTOOTH DEFAULT_MSG LIBUBERTOOTH_LIBRARIES LIBUBERTOOTH_INCLUDE_DIR)

  MARK_AS_ADVANCED(LIBUBERTOOTH_INCLUDE_DIR LIBUBERTOOTH_LIBRARIES)

endif (LIBUBERTOOTH_INCLUDE_DIR AND LIBUBERTOOTH_LIBRARIES)