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

#include <getopt.h>
#include <stdlib.h>
#include "ubertooth.h"

extern u8 debug;
extern FILE *dumpfile;

static void usage(void)
{
	printf("ubertooth-specan - output a continuous stream of signal strengths\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-v verbose (print debug information to stderr)\n");
	printf("\t-g output suitable for feedgnuplot\n");
	printf("\t-G output suitable for 3D feedgnuplot\n");
	printf("\t-d <filename> output to file\n");
	printf("\t-l lower frequency (default 2402)\n");
	printf("\t-u upper frequency (default 2480)\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
}

int main(int argc, char *argv[])
{
	int opt, output_mode = SPECAN_STDOUT;
	int lower= 2402, upper= 2480;
	char ubertooth_device = -1;

	struct libusb_device_handle *devh = NULL;

	while ((opt=getopt(argc,argv,"vhgGd:l::u::U:")) != EOF) {
		switch(opt) {
		case 'v':
			debug++;
			break;
		case 'g':
			output_mode = SPECAN_GNUPLOT_NORMAL;
			break;
		case 'G':
			output_mode = SPECAN_GNUPLOT_3D;
			break;
		case 'd':
			output_mode = SPECAN_FILE;
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'l':
			if (optarg)
				lower= atoi(optarg);
			else
				printf("lower: %d\n", lower);
			break;
		case 'u':
			if (optarg)
				upper= atoi(optarg);
			else
				printf("upper: %d\n", upper);
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	devh = ubertooth_start(ubertooth_device);

	if (devh == NULL) {
		usage();
		return 1;
	}

	while (1)
		specan(devh, 512, 0xFFFF, lower, upper, output_mode);

	ubertooth_stop(devh);
	return 0;
}
