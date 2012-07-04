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

CC ?= gcc
OPTFLAGS = -O2
ANGRYFLAGS = -Wall #-Wextra -pedantic
OS = $(shell uname)
ifeq ($(OS), FreeBSD)
	LIBUSB = usb
	CFLAGS += -DFREEBSD
else
	LIBUSB = usb-1.0
endif
LIBBTBB = btbb
CFLAGS += $(OPTFLAGS) $(ANGRYFLAGS)
LDFLAGS += -l$(LIBUSB) -l$(LIBBTBB)

all: ubertooth-dump ubertooth-lap ubertooth-specan ubertooth-util ubertooth-uap ubertooth-hop ubertooth-btle

ubertooth-dump: ubertooth-dump.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-dump.c ubertooth.c -o ubertooth-dump $(LDFLAGS)

ubertooth-lap: ubertooth-lap.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-lap.c ubertooth.c -o ubertooth-lap $(LDFLAGS)

ubertooth-specan: ubertooth-specan.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-specan.c ubertooth.c -o ubertooth-specan $(LDFLAGS)

ubertooth-util: ubertooth-util.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-util.c ubertooth.c -o ubertooth-util $(LDFLAGS)

ubertooth-uap: ubertooth-uap.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-uap.c ubertooth.c -o ubertooth-uap $(LDFLAGS)

ubertooth-hop: ubertooth-hop.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-hop.c ubertooth.c -o ubertooth-hop $(LDFLAGS)

ubertooth-btle: ubertooth-btle.c ubertooth.c ubertooth.h
	$(CC) $(CFLAGS) ubertooth-btle.c ubertooth.c -o ubertooth-btle $(LDFLAGS)

clean:
	rm -f ubertooth-dump ubertooth-lap ubertooth-specan ubertooth-util ubertooth-uap ubertooth-hop ubertooth-btle *.o
