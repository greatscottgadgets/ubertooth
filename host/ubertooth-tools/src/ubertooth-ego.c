/*
 * Copyright 2015 Mike Ryan
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
#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

struct libusb_device_handle *devh = NULL;

static void usage(void)
{
	printf("ubertooth-ego - Yuneec E-GO skateboard sniffing\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\n");
	printf("    Major modes:\n");
	printf("\t-f follow connections\n");
	printf("\t-r continuous rx on a single channel\n");
	printf("\t-i interfere\n");
	printf("\n");
	printf("    Options:\n");
	printf("\t-c <2402-2480> set channel in MHz (for continuous rx)\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int do_mode = -1;
	int do_channel = 2418;
	char ubertooth_device = -1;
	int r;

	while ((opt=getopt(argc,argv,"frijc:U:h")) != EOF) {
		switch(opt) {
		case 'f':
			do_mode = 0;
			break;
		case 'r':
			do_mode = 1;
			break;
		case 'i':
		case 'j':
			do_mode = 2; // TODO take care of these magic numbers
			break;
		case 'c':
			do_channel = atoi(optarg);
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

	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}

	/* Clean up on exit. */
	register_cleanup_handler(devh);

	if (do_mode >= 0) {
		usb_pkt_rx pkt;

		if (do_mode == 1) // FIXME magic number!
			cmd_set_channel(devh, do_channel);

		r = cmd_ego(devh, do_mode);
		if (r < 0) {
			if (do_mode == 0 || do_mode == 1)
				printf("Error: E-GO not supported by this firmware\n");
			else
				printf("Error: E-GO not supported by this firmware (or TX not enabled)\n");
			return 1;
		}

		while (1) {
			int r = cmd_poll(devh, &pkt);
			if (r < 0) {
				printf("USB error\n");
				break;
			}
			if (r == sizeof(usb_pkt_rx))
				cb_ego(NULL, &pkt, 0);
			usleep(500);
		}
		ubertooth_stop(devh);
	}

	return 0;
}
