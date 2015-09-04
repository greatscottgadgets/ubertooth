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

struct libusb_device_handle *devh = NULL;

static void usage(FILE *file)
{
	fprintf(file, "ubertooth-specan - output a continuous stream of signal strengths\n");
	fprintf(file, "Usage:\n");
	fprintf(file, "\t-h this help\n");
	fprintf(file, "\t-v verbose (print debug information to stderr)\n");
	fprintf(file, "\t-g output suitable for feedgnuplot\n");
	fprintf(file, "\t-G output suitable for 3D feedgnuplot\n");
	fprintf(file, "\t-d <filename> output to file\n");
	fprintf(file, "\t-l lower frequency (default 2402)\n");
	fprintf(file, "\t-u upper frequency (default 2480)\n");
	fprintf(file, "\t-U<0-7> set ubertooth device to use\n");
}

int main(int argc, char *argv[])
{
	int opt, r = 0, output_mode = SPECAN_STDOUT;
	int lower= 2402, upper= 2480;
	char ubertooth_device = -1;

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
			if(*optarg == '-') {
				dumpfile = stdout;
			} else {
				dumpfile = fopen(optarg, "w");
				if (dumpfile == NULL) {
					perror(optarg);
					return 1;
				}
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
			usage(stdout);
			return 0;
		default:
			usage(stderr);
			return 1;
		}
	}

	devh = ubertooth_start(ubertooth_device);

	if (devh == NULL) {
		usage(stderr);
		return 1;
	}
	
	/* Clean up on exit. */
	register_cleanup_handler(devh);
	
	while (1) {
		r = specan(devh, 512, lower, upper, output_mode);
		if(r<0)
			break;
	}

	ubertooth_stop(devh);
	fprintf(stderr, "Ubertooth stopped\n");
	return r;
}
