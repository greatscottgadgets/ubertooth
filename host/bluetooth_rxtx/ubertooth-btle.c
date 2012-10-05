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
#include <unistd.h>

extern FILE *infile;
extern FILE *dumpfile;

static void usage(void)
{
	printf("ubertooth-btle - passive Bluetooth Low Energy monitoring\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-s sniff packets\n");
	printf("\t-p promiscuous: sniff active connections\n");
	printf("\t-i<filename> read packets from file\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
	printf("\t-d filename\n");
	printf("\t-a[address] get/set access address (example: -a8e89bed6)\n");
	printf("\t-v[01] verify CRC mode, get status or enable/disable\n");

	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
	printf("If get/set access address is specified, no capture occurs.\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int do_sniff, do_file, do_promisc;
	int do_get_aa, do_set_aa;
	int do_crc;
	char ubertooth_device = -1;
	struct libusb_device_handle *devh = NULL;

	u32 access_address;

	do_sniff = do_file = 0, do_promisc = 0;
	do_get_aa = do_set_aa = 0;
	do_crc = -1; // 0 and 1 mean set, 2 means get

	while ((opt=getopt(argc,argv,"a::d:hspi:U:v::")) != EOF) {
		switch(opt) {
		case 'a':
			if (optarg == NULL) {
				do_get_aa = 1;
			} else {
				do_set_aa = 1;
				sscanf(optarg, "%08x", &access_address);
			}
			break;
		case 's':
			do_sniff = 1;
			break;
		case 'p':
			do_promisc = 1;
			break;
		case 'i':
			do_file = 1;
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
		case 'd':
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'v':
			if (optarg)
				do_crc = atoi(optarg) ? 1 : 0;
			else
				do_crc = 2; // get
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (do_file) {
		rx_btle_file(infile);
		fclose(infile);
		return 0; // do file is the only command that doesn't open ubertooth
	}

	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}

	if (do_sniff || do_promisc) {
		usb_pkt_rx pkt;

		cmd_set_modulation(devh, MOD_BT_LOW_ENERGY);

		if (do_sniff) {
			cmd_set_channel(devh, 2402);
			cmd_btle_sniffing(devh, 2);
		} else {
			cmd_btle_promisc(devh);
		}

		while (1) {
			int r = cmd_poll(devh, &pkt);
			if (r < 0) {
				printf("USB error\n");
				break;
			}
			if (r == sizeof(usb_pkt_rx))
				cb_btle(NULL, &pkt, 0);
			usleep(500);
		}
		ubertooth_stop(devh);
	}

	if (do_get_aa) {
		access_address = cmd_get_access_address(devh);
		printf("Access address: %08x\n", access_address);
		return 0;
	}

	if (do_set_aa) {
		cmd_set_access_address(devh, access_address);
		printf("access address set to: %08x\n", access_address);
	}

	if (do_crc >= 0) {
		int r;
		if (do_crc == 2) {
			r = cmd_get_crc_verify(devh);
		} else {
			cmd_set_crc_verify(devh, do_crc);
			r = do_crc;
		}
		printf("CRC: %sverify\n", r ? "" : "DO NOT ");
	}

	return 0;
}
