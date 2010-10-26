# Copyright 2010 Michael Ossmann
#
# This file is part of ubertooth.
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

all: ubertooth-dump

ubertooth-dump:
	$(CC) -g -O2 -Wall -MT ubertooth-dump.o -MD -MP -MF "./ubertooth-dump.Po" -c -o ubertooth-dump.o ubertooth-dump.c
	libtool --tag=blahbah --mode=link gcc -g -O2 -g -Wall -o ubertooth-dump  ubertooth-dump.o $(LIBUSB)

clean:
	rm -f ubertooth-dump *.o *.Po
