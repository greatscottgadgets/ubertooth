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
#include <stdbool.h>
#include "ubertooth.h"

extern char Quiet;
extern char Ubertooth_Device;

static void usage(void)
{
	printf("ubertooth-specan - output a continuous stream of signal strengths\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-g output suitable for feedgnuplot\n");
	printf("\t-G output suitable for 3D feedgnuplot\n");
	printf("\t-l lower frequency (default 2402)\n");
	printf("\t-q quiet (suppress stderr chatter)\n");
	printf("\t-u upper frequency (default 2480)\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
}


int main(int argc, char *argv[])
{
	int opt, gnuplot= false;
	int lower= 2402, upper= 2480;

	struct libusb_device_handle *devh = NULL;


	while ((opt=getopt(argc,argv,"hgGl::qu::U:")) != EOF) {
		switch(opt) {
		case 'g':
			gnuplot= GNUPLOT_NORMAL;
			break;
		case 'G':
			gnuplot= GNUPLOT_3D;
			break;
		case 'l':
			if (optarg)
				lower= atoi(optarg);
			else
				printf("lower: %d\n", lower);
			break;
		case 'q':
			Quiet= true;
			break;
		case 'u':
			if (optarg)
				upper= atoi(optarg);
			else
				printf("upper: %d\n", upper);
			break;
		case 'U':
			Ubertooth_Device= atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	devh = ubertooth_start();

	if (devh == NULL) {
		usage();
		return 1;
	}


	while (1)
		/* specan(devh, 512, 0xFFFF, 2268, 2794); */
		if(gnuplot)
			do_specan(devh, 512, 0xFFFF, lower, upper, gnuplot);
		else
			specan(devh, 512, 0xFFFF, lower, upper);

	ubertooth_stop(devh);
	return 0;
}
