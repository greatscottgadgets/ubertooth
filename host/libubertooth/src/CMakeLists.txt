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

if ( "${RELEASE}" STREQUAL "" )
	# automatic git version when working out of git
	include(GetGitRevisionDescription)
	get_git_head_revision(GIT_REFSPEC RELEASE)
endif()

add_definitions( -DRELEASE="${RELEASE}" )

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.c.in"
               "${CMAKE_CURRENT_BINARY_DIR}/version.c"
               @ONLY)

# Targets
set(c_sources ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.c
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.c
			  CACHE INTERNAL "List of C sources")
set(c_headers ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.h
              ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.h
			  ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_interface.h
			  CACHE INTERNAL "List of C headers")

# For cygwin just force UNIX OFF and WIN32 ON
if( ${CYGWIN} )
	SET(UNIX OFF)
	SET(WIN32 ON)
endif( ${CYGWIN} )

# FIXME: This may be a hack
# perhaps there should be separate ubertooth and ubertooth-static targets?
if( ${WIN32} )
	# Static library
	add_library(ubertooth STATIC ${c_sources})
	set_target_properties(ubertooth PROPERTIES OUTPUT_NAME "ubertooth_static")
else()
	# Dynamic library
	add_library(ubertooth SHARED ${c_sources})
	set_target_properties(ubertooth PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION} SOVERSION 0)
endif()

set_target_properties(ubertooth PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Include and link to libbtbb and libusb-1.0
find_package(BTBB REQUIRED)
find_package(USB1 REQUIRED)

include_directories(${LIBUSB_INCLUDE_DIR} ${LIBBTBB_INCLUDE_DIR})
LIST(APPEND LIBUBERTOOTH_LIBS ${LIBUSB_LIBRARIES} ${LIBBTBB_LIBRARIES})

target_link_libraries(ubertooth ${LIBUBERTOOTH_LIBS})

if( ${UNIX} )
   install(TARGETS ubertooth
           LIBRARY DESTINATION lib${LIB_SUFFIX}
           COMPONENT sharedlibs
           )
   install(FILES ${c_headers}
           DESTINATION include
           COMPONENT headers
           )
endif( ${UNIX} )

if( ${WIN32} )
   install(TARGETS ubertooth
           DESTINATION bin
           COMPONENT staticlibs
           )
   install(FILES ${c_headers}
           DESTINATION include
           COMPONENT headers
           )
endif( ${WIN32} )
