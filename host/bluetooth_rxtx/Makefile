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

CC      ?= gcc
AR      ?= ar
INSTALL ?= /usr/bin/install
LDCONFIG ?= /sbin/ldconfig

OPTFLAGS   = -O2
ANGRYFLAGS = -Wall #-Wextra -pedantic
CFLAGS    += $(OPTFLAGS) $(ANGRYFLAGS)

LIB_DIR ?= /usr/lib
INCLUDE_DIR ?= /usr/include
INSTALL_DIR ?= /usr/bin

OS = $(shell uname)
ifeq ($(OS), FreeBSD)
	LIBUSB = usb
	CFLAGS += -DFREEBSD
else
	LIBUSB = usb-1.0
endif
LIBBTBB  = btbb
LDFLAGS += -l$(LIBUSB) -l$(LIBBTBB)

ifeq ($(clock_debug), true)
	# Debug using locak clock
	LDFLAGS += -lbluetooth
endif

LIB_NAME   = libubertooth.so
SONAME     = $(LIB_NAME).0

LIB_FILE   = $(SONAME).1
STATIC_LIB_FILE = libubertooth.a
LIB_CFLAGS = $(CFLAGS) -shared -Wl,-soname,$(SONAME) -fPIC

LIB_SOURCE_FILES = ubertooth.c ubertooth_control.c
LIB_OBJECT_FILES = $(LIB_SOURCE_FILES:%.c=%.o)
LIB_HEADER_FILES = $(LIB_SOURCE_FILES:%.c=%.h) ubertooth_interface.h

TOOLS = ubertooth-dump ubertooth-lap ubertooth-specan ubertooth-util ubertooth-uap ubertooth-hop ubertooth-btle
ifeq ($(clock_debug), true)
	TOOLS+= ubertooth-follow
endif

TOOLS_SOURCE = $(TOOLS:%=%.c)

all: $(LIB_FILE) $(TOOLS)

$(LIB_FILE): $(LIB_SOURCE_FILES) $(LIB_HEADER_FILES)
	$(CC) $(CFLAGS) -fPIC -c $(LIB_SOURCE_FILES)
	$(CC) $(LIB_CFLAGS) -o $(LIB_FILE) $(LIB_OBJECT_FILES) $(LDFLAGS)

$(TOOLS): $(LIB_FILE) $(TOOLS_SOURCE)
		$(CC) $(CFLAGS) -o $@ $@.c $(LIB_FILE) $(LDFLAGS)

$(STATIC_LIB_FILE): $(LIB_FILE)
	$(AR) rcs $(STATIC_LIB_FILE) $(LIB_OBJECT_FILES)

install: $(LIB_FILE) $(TOOLS)
	$(INSTALL) -m 0644 $(LIB_HEADER_FILES) $(INCLUDE_DIR)
	$(INSTALL) -m 0644 $(LIB_FILE) $(LIB_DIR)
	ln -fs $(LIB_FILE) $(LIB_DIR)/$(LIB_NAME)
	ln -fs $(LIB_FILE) $(LIB_DIR)/$(SONAME)
	$(LDCONFIG)
	$(INSTALL) -m 0644 $(LIB_HEADER_FILES) $(INCLUDE_DIR)
	$(INSTALL) -m 0755 $(TOOLS) $(INSTALL_DIR)

cygwin-install: $(LIB_FILE) $(STATIC_LIB_FILE)
	$(INSTALL) -m 0644 $(LIB_FILE) $(INSTALL_DIR)
	$(INSTALL) -m 0644 $(HEADER_FILES) $(INCLUDE_DIR)
	$(INSTALL) -m 0644 $(STATIC_LIB_FILE) $(INSTALL_DIR)
	ln -fs $(LIB_FILE) $(INSTALL_DIR)/$(LIB_NAME)
	ln -fs $(LIB_FILE) $(INSTALL_DIR)/$(SONAME)
	$(INSTALL) -m 0755 $(TOOLS) $(INSTALL_DIR)

clean:
	rm -f $(LIB_NAME).* $(STATIC_LIB_FILE) $(TOOLS) $(LIB_OBJECT_FILES) ubertooth-follow

.PHONY: all clean install cygwin-install
