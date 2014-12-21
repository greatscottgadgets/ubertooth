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
set(MAJOR_VERSION 0)
set(MINOR_VERSION 2)
project(ubertooth_all)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

set(INSTALL_DEFAULT_BINDIR "bin" CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

set( VERSION ${MAJOR_VERSION}.${MINOR_VERSION} )
add_definitions( -DVERSION="${VERSION}" )

# Comment the following out for releases.
set(CMAKE_C_FLAGS "$ENV{CFLAGS}")

add_subdirectory(libubertooth)
add_subdirectory(ubertooth-tools)
add_subdirectory(misc)
if(NOT DISABLE_PYTHON)
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
