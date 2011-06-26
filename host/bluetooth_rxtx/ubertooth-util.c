/*
 * Copyright 2010 Michael Ossmann
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
#include <stdio.h>
#include <getopt.h>

static void usage()
{
	printf("ubertooth-util - command line utility for Ubertooth Zero and Ubertooth One\n");
	printf("Usage:\n");
	printf("\t-f activate flash programming (DFU) mode\n");
	printf("\t-i activate In-System Programming (ISP) mode\n");
	printf("\t-l get status of USR LED\n");
	printf("\t-l0 turn off USR LED\n");
	printf("\t-l1 turn on USR LED\n");
	printf("\t-p get microcontroller Part ID\n");
	printf("\t-s get microcontroller serial number\n");
	printf("\t-t intitiate continuous transmit test\n");
	printf("\t-a get power amplifier level\n");
	printf("\t-a[0-7] set power amplifier level\n");
	printf("\t-c get channel in MHz\n");
	printf("\t-c[0-78] set channel in MHz\n");
	printf("\t-r full reset\n");
	printf("\t-n initiate range test\n");
	printf("\t-m display range test result\n");
	printf("\t-e start repeater mode\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int r = 0;
	struct libusb_device_handle *devh = ubertooth_start();
	rangetest_result rr;

	if (devh == NULL)
		return 1;

	while ((opt=getopt(argc,argv,"nmefiprstl::a::c::")) != EOF) {
		switch(opt) {
		case 'f':
			r = cmd_flash(devh);
			break;
		case 'i':
			r = cmd_set_isp(devh);
			break;
		case 'l':
			if (optarg)
				r = cmd_set_usrled(devh, atoi(optarg));
			else
				printf("USR LED status: %d\n", cmd_get_usrled(devh));
			break;
		case 'p':
			printf("Part ID: %X\n", cmd_get_partnum(devh));
			break;
		case 'r':
			r = cmd_reset(devh);
			break;
		case 's':
			cmd_get_serial(devh);
			break;
		case 't':
			r = cmd_tx_test(devh);
			break;
		case 'a':
			if (optarg) {
				r = cmd_set_palevel(devh, atoi(optarg));
			} else {
				r = cmd_get_palevel(devh);
				if (r >= 0) {
					printf("PA level: %d\n", r);
				}
			}
			break;
		case 'c':
			if (optarg) {
				r = cmd_set_channel(devh, atoi(optarg));
			} else {
				r = cmd_get_channel(devh);
				if (r >= 0) {
					printf("Current frequency: %d MHz (Bluetooth channel %d)\n", r, r - 2402);
				}
			}
			break;
		case 'n':
			r = cmd_range_test(devh);
			break;
		case 'm':
			r = cmd_get_rangeresult(devh, &rr);
			if (r == 0) {
				if (rr.valid) {
					printf("request PA level: %d\n", rr.request_pa);
					printf("request number  : %d\n", rr.request_num);
					printf("reply PA level  : %d\n", rr.reply_pa);
					printf("reply number    : %d\n", rr.reply_num);
				} else {
					printf("invalid range test result\n");
				}
			}
			break;
		case 'e':
			r = cmd_repeater(devh);
			break;
		default:
			usage();
			return 1;
		}
	}

	return r;
}
