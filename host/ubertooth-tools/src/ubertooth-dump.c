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
	printf("\t-U<0-7> set ubertooth device to use\n");
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

extern FILE *dumpfile;

int main(int argc, char *argv[])
{
	int opt;
	int bitstream = 0;
	int modulation = MOD_BT_BASIC_RATE;
	char ubertooth_device = -1;
	struct libusb_device_handle *devh = NULL;

	while ((opt=getopt(argc,argv,"bhclU:d:")) != EOF) {
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
		case 'U':
			ubertooth_device = atoi(optarg);
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

	devh = ubertooth_start(ubertooth_device);

	if (devh == NULL) {
		usage();
		return 1;
	}

	/* Clean up on exit. */
	register_cleanup_handler(devh);

	cmd_set_modulation(devh, modulation);
	rx_dump(devh, bitstream);

	ubertooth_stop(devh);
	return 0;
}
