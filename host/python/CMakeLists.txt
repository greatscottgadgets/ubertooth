# Copyright 2013 Dominic Spill
#
# This file is part of Ubertooth
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

project(ubertooth-python)

# Check for python installation
find_package(PythonInterp)
if(NOT PYTHONINTERP_FOUND)
	return()
else()
	add_subdirectory(specan_ui)
#	add_subdirectory(usb_dfu)
endif()

# Create uninstall target
if(NOT ubertooth_all_SOURCE_DIR)
	configure_file(
		${PROJECT_SOURCE_DIR}/../cmake/cmake_uninstall.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
	@ONLY)

	add_custom_target(uninstall
		${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
	)
endif()
