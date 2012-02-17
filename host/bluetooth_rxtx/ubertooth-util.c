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
#include <stdio.h>
#include <getopt.h>

const char* board_names[] = {
	"Ubertooth Zero",
	"Ubertooth One",
	"ToorCon 13 Badge"
};

extern char Ubertooth_Device;

static void usage()
{
	printf("ubertooth-util - command line utility for Ubertooth Zero and Ubertooth One\n");
	printf("Usage:\n");
	printf("\t-h display this message\n");
	printf("\t-u[0-7] specify ubertooth device to use (MUST be first option!)\n");
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
	printf("\t-c[2400-2483] set channel in MHz\n");
        printf("\t-C[1-79] set channel\n");
	printf("\t-r full reset\n");
	printf("\t-n initiate range test\n");
	printf("\t-m display range test result\n");
	printf("\t-e start repeater mode\n");
	printf("\t-d get status of all LEDs\n");
	printf("\t-d0 turn off all LED\n");
	printf("\t-d1 turn on all LED\n");
	printf("\t-v get firmware revision number\n");
	printf("\t-b get hardware board id number\n");
	printf("\t-q start LED spectrum analyzer\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int r = 0;
	struct libusb_device_handle *devh= NULL;
	rangetest_result rr;
	
	while ((opt=getopt(argc,argv,"u:hnmefiprstvbl::a::C::c::d::q::")) != EOF) {
		switch(opt) {
		// 'u' must go first as it may effect all other commands
		case 'u': 
			Ubertooth_Device = atoi(optarg);
			devh = ubertooth_start();
                        break;
		case 'f':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_flash(devh);
			break;
		case 'i':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_set_isp(devh);
			break;
		case 'l':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			if (optarg)
				r = cmd_set_usrled(devh, atoi(optarg));
			else
				printf("USR LED status: %d\n", r = cmd_get_usrled(devh));
			break;
		case 'd':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			if (optarg) {
				r = cmd_set_usrled(devh, atoi(optarg));
				r = cmd_set_rxled(devh, atoi(optarg));
				r = cmd_set_txled(devh, atoi(optarg));
			} else {
				printf("USR LED status: %d\n", r = cmd_get_usrled(devh));
				printf("RX LED status : %d\n", r = cmd_get_rxled(devh));
				printf("TX LED status : %d\n", r = cmd_get_txled(devh));
			}
			r = (r >= 0) ? 0 : r;
			break;
		case 'p':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			printf("Part ID: %X\n", r = cmd_get_partnum(devh));
			r = (r >= 0) ? 0 : r;
			break;
		case 'r':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_reset(devh);
			break;
		case 's':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_get_serial(devh);
			r = (r >= 0) ? 0 : r;
			break;
		case 't':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_tx_test(devh);
			break;
		case 'a':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			if (optarg) {
				r = cmd_set_palevel(devh, atoi(optarg));
			} else {
				r = cmd_get_palevel(devh);
				if (r >= 0) {
					printf("PA level: %d\n", r);
				}
			}
			break;
		case 'C':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
                        if (optarg) {
                                r = cmd_set_channel(devh, atoi(optarg) +2402);
                        } else {
                                r = cmd_get_channel(devh);
                                if (r >= 0) {
                                        printf("Current frequency: %d MHz (Bluetooth channel %d)\n", r, r - 2402);
                                }
                        }
                        break;
	
		case 'c':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			if (optarg) {
				r = cmd_set_channel(devh, atoi(optarg));
			} else {
				r = cmd_get_channel(devh);
				if (r >= 0) {
					printf("Current frequency: %d MHz (Bluetooth channel %d)\n", r, r - 2402);
				}
			}
			break;
		case 'q':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			if (optarg) {
				r = cmd_led_specan(devh, atoi(optarg));
			} else {
				r = cmd_led_specan(devh, 225);
			}
			break;
		case 'n':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_range_test(devh);
			break;
		case 'm':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_get_rangeresult(devh, &rr);
			if (r == 0) {
				if (rr.valid) {
					printf("request PA level : %d\n", rr.request_pa);
					printf("request number   : %d\n", rr.request_num);
					printf("reply PA level   : %d\n", rr.reply_pa);
					printf("reply number     : %d\n", rr.reply_num);
				} else {
					printf("invalid range test result\n");
				}
			}
			break;
		case 'e':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_repeater(devh);
			break;
		case 'v':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_get_rev_num(devh);
			if (r >= 0) {
				printf("firmware revision number: %d\n", r);
			}
			break;
		case 'b':
			if (devh == NULL) {
				devh = ubertooth_start();
			}
			r = cmd_get_board_id(devh);
			if (r >= 0) {
				printf("board id number: %d (%s)\n", r, board_names[r]);
			}
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	if (devh == NULL) {
		devh = ubertooth_start();
	}

	if (devh == NULL) {
		usage();
		return 1;
	}


	return r;
}
