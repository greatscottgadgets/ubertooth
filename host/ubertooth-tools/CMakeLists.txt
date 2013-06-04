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

# Based heavily upon the ubertooth cmake setup.

cmake_minimum_required(VERSION 2.8)
project(ubertooth-tools C)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PACKAGE ubertooth-tools)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

if(MSVC)
include_directories(getopt)
else()
add_definitions(-Wall)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")
endif()


find_package(PCAP REQUIRED)

if(NOT libubertooth_SOURCE_DIR)
find_package(LIBUBERTOOTH REQUIRED)
include_directories(${LIBUBERTOOTH_INCLUDE_DIR})
else()
include_directories(${libubertooth_SOURCE_DIR}/src)
endif()

add_subdirectory(src)

########################################################################
# Create uninstall target
########################################################################

if(NOT ubertooth_all_SOURCE_DIR)
configure_file(
    ${PROJECT_SOURCE_DIR}/../cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)


add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
endif()
