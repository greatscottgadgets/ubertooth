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
#include "ubertooth_callback.h"
#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

ubertooth_t* ut = NULL;
int running = 1;

void quit(int signo __attribute__((unused)))
{
	running = 0;
}

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
	printf("\t-l <1-48> capture length (default: 18)\n");
	printf("\t-a <access_code> access code (default: 630f9ffe)\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int do_mode = -1;
	int do_channel = 2418;
	int ubertooth_device = -1;
	uint8_t len = 18;
	char *access_code_str = NULL;
	uint8_t access_code[4];
	int access_code_len;
	int r;

	while ((opt=getopt(argc,argv,"frijc:U:a:l:h")) != EOF) {
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
		case 'a':
			access_code_str = strdup(optarg);
			break;
		case 'l':
			len = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (len < 1 || len > 48) {
		printf("Length must be in [1,48]\n");
		usage();
	}

	// access code
	if (access_code_str != NULL) {
		int i;
		uint32_t ac[4];
		access_code_len = sscanf(access_code_str, "%02x%02x%02x%02x",
				&ac[0], &ac[1], &ac[2], &ac[3]);
		if (access_code_len < 1) {
			printf("Invalid access code\n");
			usage();
			return 1;
		}
		for (i = 0; i < access_code_len; ++i)
			access_code[i] = ac[i];
	}

	ut = ubertooth_start(ubertooth_device);
	if (ut == NULL) {
		usage();
		return 1;
	}

	r = ubertooth_check_api(ut);
	if (r < 0)
		return 1;

	// quit on ctrl-C
	signal(SIGINT, quit);
	signal(SIGQUIT, quit);
	signal(SIGTERM, quit);

	if (do_mode >= 0) {
		usb_pkt_rx rx;

		if (do_mode == 1) // FIXME magic number!
			cmd_set_channel(ut->devh, do_channel);

		if (access_code_str != NULL)
			cmd_rfcat_subcmd(ut->devh, RFCAT_SET_AA, access_code, access_code_len);

		cmd_rfcat_subcmd(ut->devh, RFCAT_CAP_LEN, &len, sizeof(len));

		r = cmd_ego(ut->devh, do_mode);
		if (r < 0) {
			if (do_mode == 0 || do_mode == 1)
				printf("Error: E-GO not supported by this firmware\n");
			else
				printf("Error: E-GO not supported by this firmware (or TX not enabled)\n");
			return 1;
		}

		// running set to 0 in signal handlers
		while (running) {
			int r = cmd_poll(ut->devh, &rx);
			if (r < 0) {
				printf("USB error\n");
				break;
			}
			if (r == sizeof(usb_pkt_rx)) {
				fifo_push(ut->fifo, &rx);
				cb_ego(ut, &len);
			}
			usleep(500);
		}
		ubertooth_stop(ut);
	}

	return 0;
}
