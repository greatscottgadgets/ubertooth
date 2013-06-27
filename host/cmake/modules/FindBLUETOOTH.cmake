# - Try to find the freetype library
# Once done this defines
#
#  LIBBLUETOOTH_FOUND - system has libbluetooth
#  LIBBLUETOOTH_INCLUDE_DIR - the libbluetooth include directory
#  LIBBLUETOOTH_LIBRARIES - Link these to use libbluetooth

# Copyright (c) 2013  Dominic Spill, <dominicgs@gmail.com>
# Copyright (c) 2006, 2008  Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LIBBLUETOOTH_INCLUDE_DIR AND LIBBLUETOOTH_LIBRARIES)

  # in cache already
  set(LIBBLUETOOTH_FOUND TRUE)

else (LIBBLUETOOTH_INCLUDE_DIR AND LIBBLUETOOTH_LIBRARIES)
  IF (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBBLUETOOTH bluez)
  ENDIF(NOT WIN32)

  FIND_PATH(LIBBLUETOOTH_INCLUDE_DIR hci.h
    PATHS ${PC_LIBBLUETOOTH_INCLUDEDIR} ${PC_LIBBLUETOOTH_INCLUDE_DIRS}
    PATHS /usr/include /usr/local/include /usr/include/bluetooth
    /usr/local/include/bluetooth)

  FIND_LIBRARY(LIBBLUETOOTH_LIBRARIES NAMES bluetooth
    PATHS ${PC_LIBBLUETOOTH_LIBDIR} ${PC_LIBBLUETOOTH_LIBRARY_DIRS})

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBBLUETOOTH DEFAULT_MSG LIBBLUETOOTH_LIBRARIES LIBBLUETOOTH_INCLUDE_DIR)

  MARK_AS_ADVANCED(LIBBLUETOOTH_INCLUDE_DIR LIBBLUETOOTH_LIBRARIES)

endif (LIBBLUETOOTH_INCLUDE_DIR AND LIBBLUETOOTH_LIBRARIES)