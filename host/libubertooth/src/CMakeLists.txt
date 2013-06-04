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
# Based slightly upon the libubertooth cmake setup.

# Targets
set(c_sources ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.c
				${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.c
				CACHE INTERNAL "List of C sources")
set(c_headers ${CMAKE_CURRENT_SOURCE_DIR}/ubertooth.h
				${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_control.h
				${CMAKE_CURRENT_SOURCE_DIR}/ubertooth_interface.h
				CACHE INTERNAL "List of C headers")

# Dynamic library
add_library(ubertooth SHARED ${c_sources})
set_target_properties(ubertooth PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.0 SOVERSION 0)

# Static library
add_library(ubertooth-static STATIC ${c_sources})
if(MSVC)
	set_target_properties(ubertooth-static PROPERTIES OUTPUT_NAME "ubertooth_static")
else()
	set_target_properties(ubertooth-static PROPERTIES OUTPUT_NAME "ubertooth")
endif()

set_target_properties(ubertooth PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(ubertooth-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Dependencies
target_link_libraries(ubertooth ${LIBUSB_LIBRARIES} ${LIBBTBB_LIBRARIES})
   
# For cygwin just force UNIX OFF and WIN32 ON
if( ${CYGWIN} )
  SET(UNIX OFF)
  SET(WIN32 ON)
endif( ${CYGWIN} )

if( ${UNIX} )
   install(TARGETS ubertooth
           LIBRARY DESTINATION lib${LIB_SUFFIX}
           COMPONENT sharedlibs
           )
   install(TARGETS ubertooth-static
           ARCHIVE DESTINATION lib${LIB_SUFFIX}
           COMPONENT staticlibs
           )
   install(FILES ${c_headers}
           DESTINATION include/${PROJECT_NAME}
           COMPONENT headers
           )
endif( ${UNIX} )

if( ${WIN32} )
   install(TARGETS ubertooth
           DESTINATION bin
           COMPONENT sharedlibs
           )
   install(TARGETS ubertooth-static
           DESTINATION bin
           COMPONENT staticlibs
           )
   install(FILES ${c_headers}
           DESTINATION include/${PROJECT_NAME}
           COMPONENT headers
           )
endif( ${WIN32} )
