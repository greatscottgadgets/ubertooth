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
#include <signal.h>
#include <stdlib.h>

#ifdef USE_PCAP
#include <pcap.h>
extern pcap_t *pcap_dumpfile;
extern pcap_dumper_t *dumper;
#endif // USE_PCAP

extern FILE *dumpfile;
extern FILE *infile;
extern int max_ac_errors;

struct libusb_device_handle *devh = NULL;

static void usage()
{
	printf("ubertooth-rx - passive Bluetooth discovery/decode\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-i filename\n");
	printf("\t-l <LAP> to decode (6 hex), otherwise sniff all LAPs\n");
	printf("\t-u <UAP> to decode (2 hex), otherwise try to calculate (requires LAP)\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
#ifdef USE_PCAP
	printf("\t-c<filename> capture packets to PCAP file\n");
#endif // USE_PCAP
	printf("\t-d<filename> dump packets to binary file\n");
	printf("\t-e max_ac_errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\t-s reset channel scanning\n");
	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
}

void cleanup(int sig)
{
	sig = sig;
	if (devh) {
		cmd_stop(devh);
		ubertooth_stop(devh);
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	int opt, have_lap = 0, have_uap = 0;
	int reset_scan = 0;
	char *end;
	char ubertooth_device = -1;
	btbb_piconet *pn = NULL;
	uint32_t lap;
	uint8_t uap;

	while ((opt=getopt(argc,argv,"hi:l:u:U:d:e:s")) != EOF) {
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
#ifdef USE_PCAP
		case 'c':
			pcap_dumpfile = pcap_open_dead(DLT_PPI, 128);
			if (pcap_dumpfile == NULL)
				err(1, "pcap_open_dead: ");
			dumper = pcap_dump_open(pcap_dumpfile, optarg);
			pcap_dump_flush(dumper);
			if (dumper == NULL) {
				warn("pcap_dump_open");
				pcap_close(pcap_dumpfile);
				exit(1);
			}
			break;
#endif // USE_PCAP
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
		signal(SIGINT,cleanup);
		signal(SIGQUIT,cleanup);
		signal(SIGTERM,cleanup);

		rx_live(devh, pn, 0);

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
