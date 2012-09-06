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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "ubertooth.h"
#include <bluetooth_packet.h>
#include <bluetooth_piconet.h>
#include <getopt.h>

extern int max_ac_errors;
extern FILE *dumpfile;

static void usage()
{
	printf("ubertooth-follow - active(bluez) CLK discovery and follow for a particular UAP/LAP\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-l<LAP> (in hexadecimal)\n");
	printf("\t-u<UAP> (in hexadecimal)\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
	printf("\t-e max_ac_errors\n");
	printf("\t-d filename\n");
	printf("\t-b Bluetooth device (hci0)\n");
	printf("\t-w USB delay in 625us timeslots (default:5)\n");
	printf("\nLAP and UAP are both required, if not given they are read from the local device, in some cases this may give the incorrect address.\n");
//	printf("If an input file is not specified, an Ubertooth device is used for live capture.\n");
}

int main(int argc, char *argv[])
{
	int opt, bt_dev_id, delay = 5;
	int have_lap = 0;
	int have_uap = 0;
	char *end, ubertooth_device = -1;
	char *bt_dev = "hci0";
	struct libusb_device_handle *devh = NULL;
	uint32_t clock;
	uint16_t accuracy;
	bdaddr_t bdaddr;
	piconet pn;

	init_piconet(&pn);

	while ((opt=getopt(argc,argv,"hl:u:U:e:d:b:w:")) != EOF) {
		switch(opt) {
		case 'l':
			pn.LAP = strtol(optarg, &end, 16);
			if (end != optarg)
				++have_lap;
			break;
		case 'u':
			pn.UAP = strtol(optarg, &end, 16);
			if (end != optarg) {
				++have_uap;
				pn.have_UAP = 1;
			}
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'e':
			max_ac_errors = atoi(optarg);
			break;
		case 'd':
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'b':
			bt_dev = optarg;
			if (bt_dev == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'w': //wait
			delay = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}
	
	bt_dev_id = hci_open_dev(hci_devid(bt_dev));

	if ((have_lap != 1) || (have_uap != 1)) {
		printf("No address given, reading address from device\n");
		hci_read_bd_addr(bt_dev_id, &bdaddr, 0);
		pn.LAP = bdaddr.b[0] | bdaddr.b[1] << 8 | bdaddr.b[2] << 16;
		pn.UAP = bdaddr.b[3];
		printf("LAP=%06x UAP=%02x\n", pn.LAP, pn.UAP);
		//usage();
		//return 1;
	}

	hci_read_clock(bt_dev_id, 0, 0, &clock, &accuracy, 0);
	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}
	rx_follow(devh, &pn, clock, delay);
	ubertooth_stop(devh);

	return 0;
}
