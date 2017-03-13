#
# Copyright 2017 Dominic Spill
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

include(GNUInstallDirs)

install(FILES
	"ubertooth-btle.1"
	"ubertooth-dump.1"
	"ubertooth-rx.1"
	"ubertooth-specan.1"
	"ubertooth-afh.1"
	"ubertooth-dfu.1"
	"ubertooth-ego.1"
	"ubertooth-scan.1"
	"ubertooth-util.1"
	DESTINATION "${CMAKE_INSTALL_MANDIR}/man1" COMPONENT doc)

install(FILES
	"ubertooth.7"
	DESTINATION "${CMAKE_INSTALL_MANDIR}/man7" COMPONENT doc)
