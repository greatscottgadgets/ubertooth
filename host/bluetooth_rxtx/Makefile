# Copyright 2010 Michael Ossmann
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

CC = gcc
LIBUSB = usb-1.0
LIBBTBB = btbb

all: ubertooth-dump ubertooth-lap ubertooth-specan ubertooth-util ubertooth-uap

ubertooth-dump: ubertooth-dump.c ubertooth.c ubertooth.h
	$(CC) -l $(LIBUSB) -l $(LIBBTBB) ubertooth-dump.c ubertooth.c -o ubertooth-dump

ubertooth-lap: ubertooth-lap.c ubertooth.c ubertooth.h
	$(CC) -l $(LIBUSB) -l $(LIBBTBB) ubertooth-lap.c ubertooth.c -o ubertooth-lap

ubertooth-specan: ubertooth-specan.c ubertooth.c ubertooth.h
	$(CC) -l $(LIBUSB) -l $(LIBBTBB) ubertooth-specan.c ubertooth.c -o ubertooth-specan

ubertooth-util: ubertooth-util.c ubertooth.c ubertooth.h
	$(CC) -l $(LIBUSB) -l $(LIBBTBB) ubertooth-util.c ubertooth.c -o ubertooth-util

ubertooth-uap: ubertooth-uap.c ubertooth.c ubertooth.h
	$(CC) -l $(LIBUSB) -l $(LIBBTBB) ubertooth-uap.c ubertooth.c -o ubertooth-uap

clean:
	rm -f ubertooth-dump ubertooth-lap ubertooth-specan ubertooth-util ubertooth-uap
