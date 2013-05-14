/*
 * Copyright 2013 Mike Ryan
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ubertooth.h"
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

const char* board_names[] = {
	"Ubertooth Zero",
	"Ubertooth One",
	"ToorCon 13 Badge"
};

static void usage()
{
	printf("ubertooth-debug - command line utility for debugging Ubertooth Zero and Ubertooth One\n");
	printf("Usage:\n");
	printf("\t-r<hex> read the contents of a 16 bit CC2400 register\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int r = 0;
	struct libusb_device_handle *devh= NULL;
	int do_read_register;
	char ubertooth_device = -1;

	/* set command states to negative as a starter
	 * setting to 0 means 'do it'
	 * setting to positive is value of specified argument */
	do_read_register= -1;

	while ((opt=getopt(argc,argv,"U:r:")) != EOF) {
		switch(opt) {
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'r':
			if (optarg)
				do_read_register = strtoul(optarg, NULL, 16);
			else
				do_read_register = 0;
			break;
		default:
			usage();
			return 1;
		}
	}

	/* initialise device */
	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}

	if (do_read_register >= 0) {
		if (do_read_register == 0) {
			printf("ERROR: -r requires an argument\n");
			usage();
			return 1;
		}
		else if (do_read_register > 0xff) {
			printf("ERROR: register address must be < 0xff\n");
			usage();
			return 1;
		}

		r = cmd_read_register(devh, do_read_register);
		if (r >= 0)
			printf("register 0x%02x value: %04x\n", do_read_register, r);
	}

	return r;
}
