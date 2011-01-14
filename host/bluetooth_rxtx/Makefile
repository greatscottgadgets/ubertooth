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
LIBBTBB = libbtbb
LIBBTBB_PATH = /home/mossmann/src/gr-bluetooth/src/libbtbb

all: ubertooth-dump ubertooth-lap test

ubertooth-dump:
	$(CC) -l $(LIBUSB) ubertooth-dump.c ubertooth.c -o ubertooth-dump

# static linking for now
ubertooth-lap:
	$(CC) -I $(LIBBTBB_PATH) -l $(LIBUSB) ubertooth-lap.c ubertooth.c $(LIBBTBB_PATH)/bluetooth_packet.c $(LIBBTBB_PATH)/bluetooth_piconet.c -o ubertooth-lap
test:
	$(CC) -I $(LIBBTBB_PATH) -l $(LIBUSB) test.c ubertooth.c $(LIBBTBB_PATH)/bluetooth_packet.c $(LIBBTBB_PATH)/bluetooth_piconet.c -o test

clean:
	rm -f ubertooth-dump ubertooth-lap test
