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
LIBUSB=/usr/lib/libusb.la

all: ubertooth-simple

ubertooth-simple:
	$(CC) -g -O2 -Wall -MT ubertooth-simple.o -MD -MP -MF "./ubertooth-simple.Po" -c -o ubertooth-simple.o ubertooth-simple.c
	libtool --tag=blahbah --mode=link gcc -g -O2 -g -Wall -o ubertooth-simple  ubertooth-simple.o $(LIBUSB)

clean:
	rm -f ubertooth-simple *.o *.Po
