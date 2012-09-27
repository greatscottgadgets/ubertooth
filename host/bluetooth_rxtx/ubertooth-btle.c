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
#include <getopt.h>

static void usage(void)
{
	printf("ubertooth-btle - passive Bluetooth Low Energy monitoring\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-i filename\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
	printf("\t-a[address] get/set access address (example: -a8e89bed6)\n");

	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
	printf("If get/set access address is specified, no capture occurs.\n");
}

int main(int argc, char *argv[])
{
	int opt;
	char ubertooth_device = -1;
	struct libusb_device_handle *devh = NULL;
	FILE* infile = NULL;

	int get_aa = 0;
	int set_aa = 0;
	u32 access_address;

	while ((opt=getopt(argc,argv,"a::hi:U:")) != EOF) {
		switch(opt) {
		case 'a':
			if (optarg == NULL) {
				get_aa = 1;
			} else {
				set_aa = 1;
				sscanf(optarg, "%08x", &access_address);
			}
			break;
		case 'i':
			infile = fopen(optarg, "r");
			if (infile == NULL) {
				printf("Could not open file %s\n", optarg);
				usage();
				return 1;
			}
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (get_aa) {
		devh = ubertooth_start(ubertooth_device);
		if (devh == NULL) {
			usage();
			return 1;
		}

		access_address = cmd_get_access_address(devh);
		printf("Access address: %08x\n", access_address);

		return 0;
	}

	if (set_aa) {
		devh = ubertooth_start(ubertooth_device);
		if (devh == NULL) {
			usage();
			return 1;
		}

		cmd_set_access_address(devh, access_address);
		printf("Access address set to: %08x\n", access_address);

		return 0;
	}

	if (infile == NULL) {
		devh = ubertooth_start(ubertooth_device);
		if (devh == NULL) {
			usage();
			return 1;
		}
		cmd_set_modulation(devh, MOD_BT_LOW_ENERGY);
		rx_btle(devh);
		ubertooth_stop(devh);
	} else {
		rx_btle_file(infile);
		fclose(infile);
	}

	return 0;
}
