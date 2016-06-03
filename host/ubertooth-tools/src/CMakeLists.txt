#
# Copyright 2013 Dominic Spill
#
# This file is part of Ubertooth
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

set(INSTALL_DEFAULT_BINDIR "bin" CACHE STRING
	"Bin install path, relative to CMAKE_INSTALL_PREFIX")

find_package(BTBB REQUIRED)
find_package(BLUETOOTH)

if( ${BUILD_STATIC_BINS} )
	find_package(USB1 REQUIRED)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(BUILD_SHARED_LIBRARIES OFF)
	SET(CMAKE_EXE_LINKER_FLAGS "-static")
endif( ${BUILD_STATIC_BINS} )

if(USE_OWN_GNU_GETOPT)
	include_directories(../getopt)
	add_library(libgetopt_static STATIC ../getopt/getopt.c)
endif(USE_OWN_GNU_GETOPT)

if(NOT libubertooth_SOURCE_DIR)
	find_package(UBERTOOTH REQUIRED)
	include_directories(${LIBUBERTOOTH_INCLUDE_DIR})
	LIST(APPEND TOOLS_LINK_LIBS ${LIBUBERTOOTH_LIBRARIES})
else()
	include_directories(${libubertooth_SOURCE_DIR}/src)
	if( ${BUILD_SHARED_LIB} AND NOT ${BUILD_STATIC_BINS} )
		LIST(APPEND TOOLS_LINK_LIBS ubertooth)
	endif( ${BUILD_SHARED_LIB} AND NOT ${BUILD_STATIC_BINS} )
	if( ${BUILD_STATIC_LIB} OR ${BUILD_STATIC_BINS} )
		LIST(APPEND TOOLS_LINK_LIBS ubertooth-static)
	endif( ${BUILD_STATIC_LIB} OR ${BUILD_STATIC_BINS} )
endif()

include_directories(${LIBUSB_INCLUDE_DIR} ${LIBBTBB_INCLUDE_DIR})

LIST(APPEND TOOLS_LINK_LIBS ${LIBUSB_LIBRARIES} ${LIBBTBB_LIBRARIES})

if(USE_OWN_GNU_GETOPT)
	LIST(APPEND TOOLS_LINK_LIBS libgetopt_static)
endif(USE_OWN_GNU_GETOPT)

LIST(APPEND TOOLS ubertooth-rx ubertooth-tx ubertooth-dump ubertooth-util ubertooth-btle ubertooth-dfu ubertooth-specan ubertooth-ego ubertooth-afh)

if( USE_BLUEZ AND NOT ${LIBBLUETOOTH_FOUND} )
	message( FATAL_ERROR
		"Cannot find libbluetooth, which is required for USE_BLUEZ")
endif()

if ( ${LIBBLUETOOTH_FOUND} )
	LIST(APPEND TOOLS ubertooth-follow ubertooth-scan)
	include_directories(${LIBBLUETOOTH_INCLUDE_DIR})
	LIST(APPEND TOOLS_LINK_LIBS ${LIBBLUETOOTH_LIBRARIES})
endif()

foreach(tool ${TOOLS})
	add_executable(${tool} ${tool}.c)
	target_link_libraries(${tool} ${TOOLS_LINK_LIBS})
	install(TARGETS ${tool} RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})
endforeach(tool)

# ubertooth-debug is special because of the extra source file
add_executable(ubertooth-debug ubertooth-debug.c cc2400.c arglist.c)
install(TARGETS ubertooth-debug RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})
target_link_libraries(ubertooth-debug ${TOOLS_LINK_LIBS})
