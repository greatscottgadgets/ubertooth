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
#include "ubertooth_callback.h"
#include <err.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

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
	printf("\t-q<filename> capture packets to PCAP file\n");
	printf("\t-d<filename> dump packets to binary file\n");
	printf("\t-e max_ac_errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\t-c <BT Channel> set a fixed bluetooth channel [Default: 39]\n");
	printf("\t-s reset channel scanning\n");
	printf("\t-t <SECONDS> sniff timeout - 0 means no timeout [Default: 0]\n");
	printf("\t-z Survey mode - discover and list piconets (implies -s -t 20)\n");
	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
}

int main(int argc, char* argv[])
{
	int opt, have_lap = 0, have_uap = 0;
	int survey_mode = 0;
	int r;
	int timeout = 0;
	int reset_scan = 0;
	char* end;
	int ubertooth_device = -1;
	btbb_piconet* pn = NULL;
	uint32_t lap = 0;
	uint8_t uap = 0;
	uint8_t channel = 39;

	ubertooth_t* ut = ubertooth_init();

	while ((opt=getopt(argc,argv,"hVi:l:u:U:d:e:r:sq:t:zc:")) != EOF) {
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
			if (!ut->h_pcapng_bredr) {
				if (btbb_pcapng_create_file( optarg, "Ubertooth", &ut->h_pcapng_bredr )) {
					err(1, "create_bredr_capture_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
		case 'q':
			if (!ut->h_pcap_bredr) {
				if (btbb_pcap_create_file(optarg, &ut->h_pcap_bredr)) {
					err(1, "btbb_pcap_create_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
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
		case 's':
			++reset_scan;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'z':
			++survey_mode;
			break;
		case 'c':
			channel = atoi(optarg);
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

	if(survey_mode && (have_lap || have_uap)) {
		fprintf(stderr, "No address should be specified for survey mode\n");
		return 1;
	}

	if (infile == NULL) {
		r = ubertooth_connect(ut, ubertooth_device);
		if (r < 0) {
			usage();
			return 1;
		}

		r = ubertooth_check_api(ut);
		if (r < 0)
			return 1;
	}

	r = btbb_init(max_ac_errors);
	if (r < 0)
		return r;

	if(survey_mode) {
		// auto-flush stdout so that wrapper scripts work
		setvbuf(stdout, NULL, _IONBF, 0);
		btbb_init_survey();
	} else {
		if (have_lap) {
			pn = btbb_piconet_new();
			btbb_init_piconet(pn, lap);
			if (have_uap) {
				btbb_piconet_set_uap(pn, uap);
				if (infile == NULL)
					cmd_set_bdaddr(ut->devh, btbb_piconet_get_bdaddr(pn));
			}
			if (ut->h_pcapng_bredr) {
				btbb_pcapng_record_bdaddr(ut->h_pcapng_bredr,
							  (((uint32_t)uap)<<24)|lap,
							  have_uap ? 0xff : 0x00, 0);
			}
		} else if (have_uap) {
			fprintf(stderr, "Error: UAP but no LAP specified\n");
			usage();
			return 1;
		}
	}

	if (infile == NULL) {
		/* Scan all frequencies. Same effect as
		 * ubertooth-utils -c9999. This is necessary after
		 * following a piconet. */
		if (reset_scan) {
			cmd_set_channel(ut->devh, 9999);
		} else {
			cmd_set_channel(ut->devh, 2402 + channel);
		}

		/* Clean up on exit. */
		register_cleanup_handler(ut, 0);

		if (timeout)
			ubertooth_set_timeout(ut, timeout);

		// init USB transfer
		r = ubertooth_bulk_init(ut);
		if (r < 0)
			return r;

		r = ubertooth_bulk_thread_start();
		if (r < 0)
			return r;

		// tell ubertooth to send packets
		r = cmd_rx_syms(ut->devh);
		if (r < 0)
			return r;

		// receive and process each packet
		while(!ut->stop_ubertooth) {
			ubertooth_bulk_receive(ut, cb_rx, pn);
		}

		ubertooth_bulk_thread_stop();

		ubertooth_stop(ut);
	} else {
		stream_rx_file(ut, infile, cb_rx, pn);
		fclose(infile);
	}

	if(survey_mode) {
		printf("Survey Results\n");
		while((pn=btbb_next_survey_result()) != NULL) {
			lap = btbb_piconet_get_lap(pn);
			if (btbb_piconet_get_flag(pn, BTBB_UAP_VALID)) {
				uap = btbb_piconet_get_uap(pn);
				/* Printable version showing that the NAP is unknown */
				printf("??:??:%02X:%02X:%02X:%02X\n", uap,
						(lap >> 16) & 0xFF, (lap >> 8) & 0xFF, lap & 0xFF);
			} else
				printf("??:??:??:%02X:%02X:%02X\n", (lap >> 16) & 0xFF,
					   (lap >> 8) & 0xFF, lap & 0xFF);
			//btbb_print_afh_map(pn);
		}
	}
	if(dumpfile != NULL)
		fclose(dumpfile);

	return 0;
}
