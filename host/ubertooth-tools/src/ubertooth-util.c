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
#include <unistd.h>
#include <stdlib.h>

const char* board_names[] = {
	"Ubertooth Zero",
	"Ubertooth One",
	"ToorCon 13 Badge"
};

static void usage(FILE *output)
{
	fprintf(output, "ubertooth-util - command line utility for Ubertooth Zero and Ubertooth One\n");
	fprintf(output, "Usage:\n");
	fprintf(output, "\t-a[0-7] get/set power amplifier level\n");
	fprintf(output, "\t-b get hardware board id number\n");
	fprintf(output, "\t-c[2400-2483] get/set channel in MHz\n");
	fprintf(output, "\t-C[0-78] get/set channel\n");
	fprintf(output, "\t-d[0-1] get/set all LEDs\n");
	fprintf(output, "\t-e start repeater mode\n");
	fprintf(output, "\t-f activate flash programming (DFU) mode\n");
	fprintf(output, "\t-h display this message\n");
	fprintf(output, "\t-i activate In-System Programming (ISP) mode\n");
	fprintf(output, "\t-I identify ubertooth device by flashing all LEDs\n");
	fprintf(output, "\t-l[0-1] get/set USR LED\n");
	fprintf(output, "\t-m display range test result\n");
	fprintf(output, "\t-n initiate range test\n");
	fprintf(output, "\t-p get microcontroller Part ID\n");
	fprintf(output, "\t-q[1-225 (RSSI threshold)] start LED spectrum analyzer\n");
	fprintf(output, "\t-r full reset\n");
	fprintf(output, "\t-s get microcontroller serial number\n");
	fprintf(output, "\t-S stop current operation\n");
	fprintf(output, "\t-t intitiate continuous transmit test\n");
	fprintf(output, "\t-U<0-7> set ubertooth device to use\n");
	fprintf(output, "\t-v get firmware revision number\n");
	fprintf(output, "\t-V get compile info\n");
	fprintf(output, "\t-z set squelch level\n");
}

#define MAX_VERSION_STRING_LEN 255

int main(int argc, char *argv[])
{
	int opt;
	int r = 0;
	ubertooth_t* ut = NULL;
	rangetest_result rr;
	int do_stop, do_flash, do_isp, do_leds, do_part, do_reset;
	int do_serial, do_tx, do_palevel, do_channel, do_led_specan;
	int do_range_test, do_repeater, do_firmware, do_board_id;
	int do_range_result, do_all_leds, do_identify;
	int do_set_squelch, do_get_squelch, squelch_level;
	int do_something, do_compile_info;
	int ubertooth_device = -1;
	char version_string[MAX_VERSION_STRING_LEN];

	/* set command states to negative as a starter
	 * setting to 0 means 'do it'
	 * setting to positive is value of specified argument */
	do_stop= do_flash= do_isp= do_leds= do_part= do_reset= -1;
	do_serial= do_tx= do_palevel= do_channel= do_led_specan= -1;
	do_range_test= do_repeater= do_firmware= do_board_id= -1;
	do_range_result= do_all_leds= do_identify= -1;
	do_set_squelch= -1, do_get_squelch= -1; squelch_level= 0;
	do_something= 0; do_compile_info= -1;

	while ((opt=getopt(argc,argv,"U:hnmefiIprsStvbl::a::C::c::d::q::z::9V")) != EOF) {
		switch(opt) {
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'f':
			fprintf(stderr, "ubertooth-util -f is no longer required - use ubertooth-dfu instead\n");
			do_flash= 0;
			break;
		case 'i':
			do_isp= 0;
			break;
		case 'I':
			do_identify= 0;
			break;
		case 'l':
			if (optarg)
				do_leds= atoi(optarg);
			else
				do_leds= 2; /* can't use 0 as it's a valid option */
			break;
		case 'd':
			if (optarg)
				do_all_leds= atoi(optarg);
			else
				do_all_leds= 2; /* can't use 0 as it's a valid option */
			break;
		case 'p':
			do_part= 0;
			break;
		case 'r':
			do_reset= 0;
			break;
		case 's':
			do_serial= 0;
			break;
		case 'S':
			do_stop= 0;
			break;
		case 't':
			do_tx= 0;
			break;
		case 'a':
			if (optarg)
				do_palevel= atoi(optarg);
			else
				do_palevel= 0;
			break;
		case 'C':
			if (optarg)
				do_channel= atoi(optarg) +2402;
			else
				do_channel= 0;
			break;
		case 'c':
			if (optarg)
				do_channel= atoi(optarg);
			else
				do_channel= 0;
			break;
		case 'q':
			if (optarg)
				do_led_specan= atoi(optarg);
			else
				do_led_specan= 0;
			break;
		case 'n':
			do_range_test= 0;
			break;
		case 'm':
			do_range_result= 0;
			break;
		case 'e':
			do_repeater= 0;
			break;
		case 'v':
			do_firmware= 0;
			break;
		case 'b':
			do_board_id= 0;
			break;
		case 'z':
			if (optarg) {
				squelch_level = atoi(optarg);
				do_set_squelch = 1;
			}
			else {
				do_get_squelch = 1;
			}
			break;
		case '9':
			do_something= 1;
			break;
		case 'V':
			do_compile_info = 0;
			break;
		case 'h':
			usage(stdout);
			return 0;
		default:
			usage(stderr);
			return 1;
		}
	}

	/* initialise device */
	ut = ubertooth_start(ubertooth_device);
	if (ut == NULL) {
		usage(stderr);
		return 1;
	}
	if(do_reset == 0) {
		fprintf(stdout, "Resetting ubertooth device number %d\n", (ubertooth_device >= 0) ? ubertooth_device : 0);
		r = cmd_reset(ut->devh);
		sleep(2);
		ut = ubertooth_start(ubertooth_device);
	}
	if(do_stop == 0) {
		fprintf(stdout, "Stopping ubertooth device number %d\n", (ubertooth_device >= 0) ? ubertooth_device : 0);
		r = cmd_stop(ut->devh);
	}

	/* device configuration actions */
	if(do_all_leds == 0 || do_all_leds == 1) {
		cmd_set_usrled(ut->devh, do_all_leds);
		cmd_set_rxled(ut->devh, do_all_leds);
		r= cmd_set_txled(ut->devh, do_all_leds);
		r = (r >= 0) ? 0 : r;
	}
	if(do_channel > 0)
		r= cmd_set_channel(ut->devh, do_channel);
	if(do_leds == 0 || do_leds == 1)
		r= cmd_set_usrled(ut->devh, do_leds);
	if(do_palevel > 0)
		r= cmd_set_palevel(ut->devh, do_palevel);

	/* reporting actions */
	if(do_all_leds == 2) {
		fprintf(stdout, "USR LED status: %d\n", cmd_get_usrled(ut->devh));
		fprintf(stdout, "RX LED status : %d\n", cmd_get_rxled(ut->devh));
		fprintf(stdout, "TX LED status : %d\n", r= cmd_get_txled(ut->devh));
		r = (r >= 0) ? 0 : r;
	}
	if(do_board_id == 0) {
		r= cmd_get_board_id(ut->devh);
		fprintf(stdout, "Board ID Number: %d (%s)\n", r, board_names[r]);
	}
	if(do_channel == 0) {
		r= cmd_get_channel(ut->devh);
		fprintf(stdout, "Current frequency: %d MHz (Bluetooth channel %d)\n", r, r - 2402);
		}
	if(do_firmware == 0) {
		uint16_t usb_version;
		cmd_get_rev_num(ut->devh, version_string, MAX_VERSION_STRING_LEN);
		fprintf(stdout, "Firmware version: %s", version_string);
		r = ubertooth_get_api(ut, &usb_version);
		if(r >= 0)
			fprintf(stdout, " (API:%x.%02x)\n", (usb_version>>8)&0xFF, usb_version&0xFF);
		else
			fprintf(stdout, "\n");

	}
	if(do_compile_info == 0) {
		cmd_get_compile_info(ut->devh, version_string, MAX_VERSION_STRING_LEN);
		fprintf(stdout, "%s\n", version_string);
	}
	if(do_leds == 2)
		fprintf(stdout, "USR LED status: %d\n", r= cmd_get_usrled(ut->devh));
	if(do_palevel == 0)
		fprintf(stdout, "PA Level: %d\n", r= cmd_get_palevel(ut->devh));
	if(do_part == 0) {
		fprintf(stdout, "Part ID: %X\n", r = cmd_get_partnum(ut->devh));
		r = (r >= 0) ? 0 : r;
	}
	if(do_range_result == 0) {
		r = cmd_get_rangeresult(ut->devh, &rr);
		if (r == 0) {
			if (rr.valid==1) {
				fprintf(stdout, "request PA level : %d\n", rr.request_pa);
				fprintf(stdout, "request number   : %d\n", rr.request_num);
				fprintf(stdout, "reply PA level   : %d\n", rr.reply_pa);
				fprintf(stdout, "reply number     : %d\n", rr.reply_num);
			} else if (rr.valid>1) {
				fprintf(stdout, "Invalid range test: mismatch on byte %d\n", rr.valid-2);
			} else {
				fprintf(stdout, "invalid range test result\n");
			}
		}
	}
	if(do_serial == 0) {
		u8 serial[17];
		r= cmd_get_serial(ut->devh, serial);
		if(r==0) {
			print_serial(serial, NULL);
		}
		// FIXME: Why do we do this to non-zero results?
		r = (r >= 0) ? 0 : r;
	}

	/* final actions */
	if(do_flash == 0) {
		fprintf(stdout, "Entering flash programming (DFU) mode\n");
		return cmd_flash(ut->devh);
	}
	if(do_identify == 0) {
		fprintf(stdout, "Flashing LEDs on ubertooth device number %d\n", (ubertooth_device >= 0) ? ubertooth_device : 0);
		while(42) {
			do_identify= !do_identify;
			cmd_set_usrled(ut->devh, do_identify);
			cmd_set_rxled(ut->devh, do_identify);
			cmd_set_txled(ut->devh, do_identify);
			sleep(1);
		}
	}
	if(do_isp == 0) {
		fprintf(stdout, "Entering flash programming (ISP) mode\n");
		return cmd_set_isp(ut->devh);
	}
	if(do_led_specan >= 0) {
		do_led_specan= do_led_specan ? do_led_specan : 225;
		fprintf(stdout, "Entering LED specan mode (RSSI %d)\n", do_led_specan);
		return cmd_led_specan(ut->devh, do_led_specan);
	}
	if(do_range_test == 0) {
		fprintf(stdout, "Starting range test\n");
		return cmd_range_test(ut->devh);
	}
	if(do_repeater == 0) {
		fprintf(stdout, "Starting repeater\n");
		return cmd_repeater(ut->devh);
	}
	if(do_tx == 0) {
		fprintf(stdout, "Starting TX test\n");
		return cmd_tx_test(ut->devh);
	}
	if(do_set_squelch > 0) {
		fprintf(stdout, "Setting squelch to %d\n", squelch_level);
		cmd_set_squelch(ut->devh, squelch_level);
	}
	if(do_get_squelch > 0) {
		r = cmd_get_squelch(ut->devh);
		fprintf(stdout, "Squelch set to %d\n", (int8_t)r);
	}
	if(do_something) {
		unsigned char buf[4] = { 0x55, 0x55, 0x55, 0x55 };
		cmd_do_something(ut->devh, NULL, 0);
		cmd_do_something_reply(ut->devh, buf, 4);
		fprintf(stdout, "%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
		return 0;
	}

	return r;
}
