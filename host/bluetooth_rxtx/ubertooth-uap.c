/*
 * Copyright 2010, 2011 Michael Ossmann
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
#include <bluetooth_packet.h>
#include <bluetooth_piconet.h>
#include <getopt.h>

static void usage()
{
	printf("ubertooth-uap - passive UAP discovery for a particular LAP\n");
	printf("Usage:\n");
	printf("\t-l LAP (in hexadecimal)\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int argcount = 0;
	char *end;
	int r = 0;
	struct libusb_device_handle *devh = ubertooth_start();
	piconet pn;

	if (devh == NULL) {
		usage();
		return 1;
	}

	init_piconet(&pn);

	while ((opt=getopt(argc,argv,"l:")) != EOF) {
		switch(opt) {
		case 'l':
			pn.LAP = strtol(optarg, &end, 16);
			if (end != optarg)
				++argcount;
			break;
		default:
			usage();
			return 1;
		}
	}
	if (argcount != 1) {
		usage();
		return 1;
	}

	rx_uap(devh, &pn);

	ubertooth_stop(devh);

	return 0;
}
