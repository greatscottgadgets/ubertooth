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

extern char Ubertooth_Device;
extern FILE *dumpfile;
extern FILE *infile;
extern int max_ac_errors;

static void usage()
{
	printf("ubertooth-uap - passive UAP discovery for a particular LAP\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-i filename\n");
	printf("\t-l<LAP> (in hexadecimal)\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
	printf("\t-d filename\n");
	printf("\t-e max_ac_errors\n");
	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int have_lap = 0;
	char *end;
	struct libusb_device_handle *devh = NULL;
	piconet pn;

	init_piconet(&pn);

	while ((opt=getopt(argc,argv,"hi:l:U:d:e:")) != EOF) {
		switch(opt) {
		case 'i':
			infile = fopen(optarg, "r");
			if (infile == NULL) {
				printf("Could not open file %s\n", optarg);
				usage();
				return 1;
			}
			break;
		case 'l':
			pn.LAP = strtol(optarg, &end, 16);
			if (end != optarg)
				++have_lap;
			break;
		case 'U':
			Ubertooth_Device= atoi(optarg);
			break;
		case 'd':
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'e':
			max_ac_errors = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}
	if (have_lap != 1) {
		usage();
		return 1;
	}

	if (infile == NULL) {
		devh = ubertooth_start();
		if (devh == NULL) {
			usage();
			return 1;
		}
		rx_uap(devh, &pn);
		ubertooth_stop(devh);
	} else {
		rx_uap_file(infile, &pn);
		fclose(infile);
	}

	return 0;
}
