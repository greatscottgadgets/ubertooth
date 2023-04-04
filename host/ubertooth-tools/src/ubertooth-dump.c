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
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

static void usage(void)
{
	printf("ubertooth-dump - output a continuous stream of received bits\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-b only dump received bitstream (GnuRadio style)\n");
	printf("\t-c classic modulation\n");
	printf("\t-l LE modulation\n");
	printf("\t-U <0-7> set ubertooth device to use (cannot be used with -D)\n");
	printf("\t-D <serial> set ubertooth serial to use (cannot be used with -U)\n");
	printf("\t-d filename\n");
	printf("\nThis program sends binary data to stdout.  You probably don't want to\n");
	printf("run it from a terminal without redirecting the output.\n");
}

/*
 * The normal output format is in chunks of 64 bytes in the USB RX packet format
 * 50 of those 64 bytes contain the received symbols (packed 8 per byte).
 *
 * The -b output format is a stream of bytes, each either 0x00 or 0x01
 * representing the symbol determined by the demodulator (GnuRadio style)
 */

int main(int argc, char *argv[])
{
	int opt;
	int bitstream = 0;
	int modulation = MOD_BT_BASIC_RATE;
	int ubertooth_device = -1;
	char serial_c[34] = {0};
	int device_index = 0, device_serial = 0;

	ubertooth_t* ut = NULL;
	int r;

	while ((opt=getopt(argc,argv,"bhclU:D:d:")) != EOF) {
		switch(opt) {
		case 'b':
			bitstream = 1;
			break;
		case 'c':
			modulation = MOD_BT_BASIC_RATE;
			break;
		case 'l':
			modulation = MOD_BT_LOW_ENERGY;
			break;
		case 'D':
			snprintf(serial_c, strlen(optarg), "%s", optarg);
			device_serial = 1;
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			device_index = 1;
			break;
		case 'd':
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (device_serial && device_index) {
		printf("Error: Cannot use both index and serial simultaneously\n");
		usage();
		return 1;
	}

	if (device_serial)
		ut = ubertooth_start_serial(serial_c);
	else
		ut = ubertooth_start(ubertooth_device);

	if (ut == NULL) {
		usage();
		return 1;
	}

	r = ubertooth_check_api(ut);
	if (r < 0)
		return 1;

	/* Clean up on exit. */
	register_cleanup_handler(ut, 0);

	cmd_set_modulation(ut->devh, modulation);
	rx_dump(ut, bitstream);

	ubertooth_stop(ut);
	return 0;
}
