# Copyright (C) 2011 Michael Ossmann <mike@ossmann.com>
# Copyright (C) 2010 DJ Delorie <dj@redhat.com>

# This file is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3, or (at your option) any later
# version.
#
# This file is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.

CC = m32c-elf-gcc
LD = m32c-elf-gcc -nostartfiles
AR = m32c-elf-ar

# Default program to flash, and port to flash it to.
elf = toorcon
port=ttyUSB0

all : r8c2l.h toorcon.elf

# List of objects common to all R8C programs.
BSP_FILES = \
	ivects.o

bsp.a : $(BSP_FILES)
	$(AR) rvs bsp.a $(BSP_FILES)

%.elf : init.o %.o bsp.a tc13badge.o
	$(LD) $^ -o $@ bsp.a -TR5F212L4.ld

.PHONY: flash
flash : ${elf}.elf
	uflash -p /dev/${port} -3 -r -b 38400 ${elf}.elf

%.o : %.c
	$(CC) -Os -c $*.c tc13badge.c

r8c2l.h r8c2l.inc : r8c2l.io mkports
	./mkports r8c2l.io r8c2l.h r8c2l.inc

.PRECIOUS: %.o %.elf

clean :
	-rm -f *~ *.o bsp.a *.elf r8c2l.h r8c2l.inc
