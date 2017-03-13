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
#include <unistd.h>  // sleep
#include <string.h>
#include <inttypes.h> // PRIx64

static void usage()
{
	printf("ubertooth-tx - retransmit symbols dumped with ubertooth-dump\n");
	printf("\n");
	printf("!!!!\n");
	printf("WARNING: This tool currently does nothing!\n");
	printf("!!!!\n");
	printf("\n");
	printf("Usage:\n");
	printf("\t-l <LAP> to transmit to (3 bytes / 6 hex digits)\n");
	printf("\t-u <UAP> to transmit to (1 byte / 2 hex digits)\n");
	printf("\t-t <SECONDS> timeout - 0 means no timeout [Default: 0]\n");
	printf("\t-V print version information\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
}

int main(int argc, char* argv[])
{
	int opt, have_lap = 0, have_uap = 0;
	int r;
	int timeout = 0;
	char* end;
	int ubertooth_device = -1;
	uint32_t lap = 0;
	uint8_t uap = 0;

	ubertooth_t* ut = ubertooth_init();

	while ((opt=getopt(argc,argv,"hVl:u:U:t:")) != EOF) {
		switch(opt) {
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

	r = ubertooth_connect(ut, ubertooth_device);
	if (r < 0) {
		usage();
		return 1;
	}

	r = ubertooth_check_api(ut);
	if (r < 0)
		return 1;

	if (have_lap && have_uap) {
		// set bt address
		r = cmd_set_bdaddr(ut->devh, (((uint32_t)uap)<<24)|lap);
		if (r < 0)
			return r;
		uint64_t syncword = btbb_gen_syncword(lap);
		//printf("Syncword: 0x%016lx\n", syncword);
		printf("Syncword: 0x%" PRIx64 "\n", syncword);
	} else {
		fprintf(stderr, "Error: LAP or UAP not specified\n");
		usage();
		return 1;
	}

	/* Clean up on exit. */
	register_cleanup_handler(ut, 0);

	if (timeout)
		ubertooth_set_timeout(ut, timeout);

	// tell ubertooth to send packets
	r = cmd_tx_syms(ut->devh);
	if (r < 0)
		return r;

	// TODO - send symbols to the bulk endpoint

	// wait
	while(!ut->stop_ubertooth)
		sleep(1);

	ubertooth_stop(ut);

	return 0;
}
