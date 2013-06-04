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

# Based heavily upon the hackrf cmake setup.

set(INSTALL_DEFAULT_BINDIR "bin" CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

if(MSVC)
add_library(libgetopt_static STATIC
    ../getopt/getopt.c
)
endif()

add_executable(ubertooth-rx ubertooth-rx.c)
install(TARGETS ubertooth-rx RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(ubertooth-dump ubertooth-dump.c)
install(TARGETS ubertooth-dump RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(ubertooth-util ubertooth-util.c)
install(TARGETS ubertooth-util RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(ubertooth-debug ubertooth-debug.c cc2400.c)
install(TARGETS ubertooth-debug RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(ubertooth-btle ubertooth-btle.c)
install(TARGETS ubertooth-btle RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

if(NOT libubertooth_SOURCE_DIR)
include_directories(${LIBUBERTOOTH_INCLUDE_DIR})
LIST(APPEND TOOLS_LINK_LIBS ${LIBUBERTOOTH_LIBRARIES} ${LIBPCAP_LIBRARIES})
else()
LIST(APPEND TOOLS_LINK_LIBS ubertooth pcap)
endif()

if(MSVC)
LIST(APPEND TOOLS_LINK_LIBS libgetopt_static)
endif()


target_link_libraries(ubertooth-rx ${TOOLS_LINK_LIBS})
target_link_libraries(ubertooth-dump ${TOOLS_LINK_LIBS})
target_link_libraries(ubertooth-util ${TOOLS_LINK_LIBS})
target_link_libraries(ubertooth-debug ${TOOLS_LINK_LIBS})
target_link_libraries(ubertooth-btle ${TOOLS_LINK_LIBS})
