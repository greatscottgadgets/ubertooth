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
#include <err.h>
#include <getopt.h>
#include <stdlib.h>

extern FILE *dumpfile;
extern FILE *infile;
extern int max_ac_errors;

struct libusb_device_handle *devh = NULL;

static void usage()
{
	printf("ubertooth-rx - passive Bluetooth discovery/decode\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-V print version information\n");
	printf("\t-i filename\n");
	printf("\t-l <LAP> to decode (6 hex), otherwise sniff all LAPs\n");
	printf("\t-u <UAP> to decode (2 hex), otherwise try to calculate (requires LAP)\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
	printf("\t-r<filename> capture packets to PCAPNG file\n");
#ifdef ENABLE_PCAP
	printf("\t-q<filename> capture packets to PCAP file\n");
#endif
	printf("\t-d<filename> dump packets to binary file\n");
	printf("\t-e max_ac_errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\t-s reset channel scanning\n");
	printf("\t-t <SECONDS> sniff timeout - 0 means no timeout [Default: 0]\n");
	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
}

int main(int argc, char *argv[])
{
	int opt, have_lap = 0, have_uap = 0;
	int timeout = 0;
	int reset_scan = 0;
	char *end;
	char ubertooth_device = -1;
	btbb_piconet *pn = NULL;
	uint32_t lap = 0;
	uint8_t uap = 0;

	while ((opt=getopt(argc,argv,"hVi:l:u:U:d:e:r:sq:t:")) != EOF) {
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
			lap = strtol(optarg, &end, 16);
			have_lap++;
			break;
		case 'u':
			uap = strtol(optarg, &end, 16);
			have_uap++;
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'r':
			if (!h_pcapng_bredr) {
				if (btbb_pcapng_create_file( optarg, "Ubertooth", &h_pcapng_bredr )) {
					err(1, "create_bredr_capture_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
#ifdef ENABLE_PCAP
		case 'q':
			if (!h_pcap_bredr) {
				if (btbb_pcap_create_file(optarg, &h_pcap_bredr)) {
					err(1, "btbb_pcap_create_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
#endif
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
		case 's':
			++reset_scan;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'V':
			print_version();
			return 0;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (have_lap) {
		pn = btbb_piconet_new();
		btbb_init_piconet(pn, lap);
		if (have_uap)
			btbb_piconet_set_uap(pn, uap);
		if (h_pcapng_bredr) {
			btbb_pcapng_record_bdaddr(h_pcapng_bredr,
						  (((uint32_t)uap)<<24)|lap,
						  have_uap ? 0xff : 0x00, 0);
		}
	} else if (have_uap) {
		printf("Error: UAP but no LAP specified\n");
		usage();
		return 1;
	}

	if (infile == NULL) {
		devh = ubertooth_start(ubertooth_device);
		if (devh == NULL) {
			usage();
			return 1;
		}

		/* Scan all frequencies. Same effect as
		 * ubertooth-utils -c9999. This is necessary after
		 * following a piconet. */
		if (reset_scan) {
			cmd_set_channel(devh, 9999);
		}

		/* Clean up on exit. */
		register_cleanup_handler(devh);

		rx_live(devh, pn, timeout);

		// Print AFH map from piconet if we have one
		if (pn)
			btbb_print_afh_map(pn);

		ubertooth_stop(devh);
	} else {
		rx_file(infile, pn);
		fclose(infile);
	}

	return 0;
}
