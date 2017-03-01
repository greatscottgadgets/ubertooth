#
# Copyright 2013 Dominic Spill
#
# This file is part of Ubertooth.
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
#top level cmake project for ubertooth lib + tools

cmake_minimum_required(VERSION 2.8)
project(ubertooth_all)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules
	CACHE STRING "CMake module path")

set(MAJOR_VERSION 1 CACHE INT "libUbertooth major version number")
set(MINOR_VERSION 0 CACHE INT "libUbertooth minor version number")
set( VERSION ${MAJOR_VERSION}.${MINOR_VERSION} )
add_definitions( -DVERSION="${VERSION}" )

# Comment the following out for releases.
set(CMAKE_C_FLAGS "$ENV{CFLAGS}" CACHE STRING "C Flags")

# Defaults: BUILD_SHARED=ON, BUILD_STATIC=OFF
set(BUILD_SHARED_LIB ON CACHE BOOL "Build shared library")
set(BUILD_STATIC_LIB OFF CACHE BOOL "Build static library")
set(BUILD_STATIC_BINS OFF CACHE BOOL "Build static library")
set(ENABLE_PYTHON ON CACHE BOOL "Build python tools")

# Check that we're building at least one library
if( NOT ${BUILD_SHARED_LIB} AND NOT ${BUILD_STATIC_LIB} )
	message( FATAL_ERROR "Not building static or shared library - at least one must be built")
endif( NOT ${BUILD_SHARED_LIB} AND NOT ${BUILD_STATIC_LIB} )

# Build static bins if we're only building the static lib
if( ${BUILD_STATIC_LIB} AND NOT ${BUILD_SHARED_LIB} )
	set(BUILD_STATIC_BINS ON)
	message(STATUS "Building static executables because only building static library")
endif( ${BUILD_STATIC_LIB} AND NOT ${BUILD_SHARED_LIB} )

# Complain if we're building the shared lib but static bins
if( ${BUILD_STATIC_BINS} AND NOT ${BUILD_STATIC_LIB} )
	message(FATAL "Building static executables not possible with shared library")
endif( ${BUILD_STATIC_BINS} AND NOT ${BUILD_STATIC_LIB} )

add_subdirectory(libubertooth)
add_subdirectory(ubertooth-tools)
add_subdirectory(misc)
if(${ENABLE_PYTHON})
	add_subdirectory(python)
endif()

# Create uninstall target
configure_file(
	${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
	${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
