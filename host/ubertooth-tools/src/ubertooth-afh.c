/*
 * Copyright 2015 Hannes Ellinger
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
#include <unistd.h>
#include <string.h>

extern int max_ac_errors;
extern unsigned int packet_counter_max;

static void usage()
{
	printf("ubertooth-afh - passive detection of the AFH channel map\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-r print AFH channel map once every second\n");
	printf("\t-V print version information\n");
	printf("\t-l <LAP> to decode (6 hex), otherwise sniff all LAPs\n");
	printf("\t-u <UAP> to decode (2 hex), otherwise try to calculate (requires LAP)\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
	printf("\t-t <seconds> timeout for initial AFH map detection\n");
	printf("\t-m <int> threshold for channel removal\n");
	printf("\t-e max_ac_errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
}

int main(int argc, char* argv[])
{
	int opt, have_lap = 0, have_uap = 0, timeout = 0;//, have_initial_afh = 0;
	// uint8_t initial_afh[10];
	char* end;
	int ubertooth_device = -1;
	btbb_piconet* pn = NULL;
	uint32_t lap = 0;
	uint8_t uap = 0;
	uint8_t use_r_format = 0;

	ubertooth_t* ut = NULL;
	int r;

	while ((opt=getopt(argc,argv,"rhVl:u:U:e:a:t:m:")) != EOF) {
		switch(opt) {
		case 'l':
			lap = strtol(optarg, &end, 16);
			have_lap = 1;
			break;
		case 'u':
			uap = strtol(optarg, &end, 16);
			have_uap = 1;
			break;
		case 'a':
			// for(int i=0; i<10; i++) {
			// 	sscanf(2+optarg+2*i, "%2hhx", &initial_afh[i]);
			// }
			// have_initial_afh = 1;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'e':
			max_ac_errors = atoi(optarg);
			break;
		case 'm':
			packet_counter_max = atoi(optarg);
			break;
		case 'V':
			print_version();
			return 0;
		case 'r':
			use_r_format = 1;
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (have_lap && have_uap) {
		pn = btbb_piconet_new();
		btbb_init_piconet(pn, lap);
		btbb_piconet_set_uap(pn, uap);
		btbb_piconet_set_flag(pn, BTBB_IS_AFH, 1);
		btbb_piconet_set_flag(pn, BTBB_LOOKS_LIKE_AFH, 1);
		btbb_init_survey();
	} else {
		printf("Error: UAP or LAP not specified\n");
		usage();
		return 1;
	}

	if (packet_counter_max == 0) {
		printf("Error: Threshold for unused channels not specified\n");
		usage();
		return 1;
	}

	ut = ubertooth_start(ubertooth_device);
	if (ut->devh == NULL) {
		usage();
		return 1;
	}

	r = ubertooth_check_api(ut);
	if (r < 0)
		return 1;

	/* Clean up on exit. */
	register_cleanup_handler(ut, 0);

	if (use_r_format)
		rx_afh_r(ut, pn, timeout);
	else
		rx_afh(ut, pn, timeout);

	ubertooth_stop(ut);

	return 0;
}
