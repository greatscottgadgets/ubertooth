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
# Based slightly upon the hackrf cmake setup.

# FIXME Set static release version here to avoid pulling from git
set(RELEASE "")
set(DIRTY_FLAG "")

if ( "${RELEASE}" STREQUAL "" )
	# automatic git version when working out of git
	include(GetGitRevisionDescription)
	get_git_head_revision(GIT_REFSPEC RELEASE)

	execute_process(COMMAND git status -s --untracked-files=no OUTPUT_VARIABLE DIRTY)
	if ( NOT "${DIRTY}" STREQUAL "" )
		set(DIRTY_FLAG "*")
	endif()
endif()

add_definitions( -DRELEASE="${RELEASE}${DIRTY_FLAG}" )

# Targets
set(c_sources ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.c
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_callback.c
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.c
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_fifo.c
			  CACHE INTERNAL "List of C sources")
set(c_headers ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.h
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_callback.h
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.h
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_fifo.h
			  ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_interface.h
			  CACHE INTERNAL "List of C headers")

# Include and link to libbtbb and libusb-1.0
find_package(BTBB REQUIRED)
find_package(USB1 REQUIRED)

include_directories(${LIBUSB_INCLUDE_DIR} ${LIBBTBB_INCLUDE_DIR})
LIST(APPEND LIBUBERTOOTH_LIBS ${LIBUSB_LIBRARIES} ${LIBBTBB_LIBRARIES})

if( ${BUILD_SHARED_LIB} )
	# Shared library
	message(STATUS "Building shared library")
	add_library(ubertooth SHARED ${c_sources})
	target_link_libraries(ubertooth ${LIBUBERTOOTH_LIBS})
	set_target_properties(ubertooth PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION} SOVERSION 0)
	set_target_properties(ubertooth PROPERTIES CLEAN_DIRECT_OUTPUT 1)
	install(TARGETS ubertooth
			LIBRARY DESTINATION lib${LIB_SUFFIX}
			COMPONENT sharedlibs)
endif( ${BUILD_SHARED_LIB} )

if( ${BUILD_STATIC_LIB} )
	# Static library
	message(STATUS "Building static library")
	add_library(ubertooth-static STATIC ${c_sources})
	target_link_libraries(ubertooth-static ${LIBUBERTOOTH_LIBS})
	set_target_properties(ubertooth-static PROPERTIES OUTPUT_NAME "ubertooth")
	set_target_properties(ubertooth-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
	install(TARGETS ubertooth-static
			DESTINATION lib${LIB_SUFFIX}
			COMPONENT staticlibs)
endif( ${BUILD_STATIC_LIB} )

install(FILES ${c_headers}
		DESTINATION include
		COMPONENT headers)
