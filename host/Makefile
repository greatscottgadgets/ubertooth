#-*- makefile -*-
#
# Copyright 2012 Dominic Spill
#
# This file is part of Project Ubertooth.
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

DIRS = bluetooth_rxtx usb_dfu

all: $(DIRS)

clean: $(DIRS:%=%_clean)

install: $(DIRS:%=%_install)

$(DIRS):
	$(MAKE) -C $@

$(DIRS:%=%_clean):
	$(MAKE) -C $(@:%_clean=%) clean

$(DIRS:%=%_install):
	$(MAKE) -C $(@:%_install=%) install

.PHONY : clean all install $(DIRS)
