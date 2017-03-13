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
	printf("\n");
	printf("Determine the AFH map for piconet ??:??:22:44:66:88:\n");
	printf("    ubertooth-afh -u 22 -l 446688\n");
	printf("\n");
	printf("Main options:\n");
	printf("\t-l <LAP> LAP of target piconet (3 bytes / 6 hex digits)\n");
	printf("\t-u <UAP> UAP of target piconet (1 byte / 2 hex digits)\n");
	printf("\t-m <int> threshold for channel removal (default: 5)\n");
	printf("\t-r print AFH channel map once every second (default: print on update)\n");
	printf("\n");
	printf("Other options\n");
	printf("\t-t <seconds> timeout for initial AFH map detection (not required)\n");
	printf("\t-e maximum access code errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\t-V print version information\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
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

	// default value for '-m' channel timeout
	packet_counter_max = 5;

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
