/*
 * Copyright 2013 Mike Ryan
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

/* formated with: indent -kr */

#include "ubertooth.h"
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "cc2400.h"
#include "arglist.h"

const char *board_names[] = {
    "Ubertooth Zero",
    "Ubertooth One",
    "ToorCon 13 Badge"
};

static void usage()
{
    printf
	("ubertooth-debug - command line utility for debugging Ubertooth Zero and Ubertooth One\n");
    printf("Usage:\n");
    printf("\t-h this message\n");
    printf("\t-r <name> read the contents of a 16 bit CC2400 register\n");
    printf
	("\t-r <number,number,low-high> read the contents of a 16 bit CC2400 register(s)\n");
    printf("\t-U<0-7> set ubertooth device to use\n");
    printf("\t-v<0-2> verbosity (default=1)\n");
}

int token_to_int(char *t, int *size)
{
    int r = 0;

    r = cc2400_name2reg(t);
    if (r >= 0)
	*size = strlen(cc2400_reg2name(r));
    else
	*size = -1;

    return r;
}

int main(int argc, char *argv[])
{
    int opt;
    int r = 0;
    int verbose = 1;
    struct libusb_device_handle *devh = NULL;
    int do_read_register;
    char ubertooth_device = -1;
    int *regList = NULL;
    int regListN = 0;
    int i;

    /* set command states to negative as a starter
     * setting to positive is value of specified argument */
    do_read_register = -1;

    while ((opt = getopt(argc, argv, "hU:r:v:")) != EOF) {
	switch (opt) {
	case 'h':
	    usage();
	    return 0;
	case 'U':
	    ubertooth_device = atoi(optarg);
	    break;
	case 'v':
	    verbose = atoi(optarg);
	    if (verbose < 0 || verbose > 2) {
		fprintf(stderr, "ERROR: verbosity out of range\n");
		return 1;
	    }
	    break;
	case 'r':
	    regList = listOfInts(optarg, &regListN, token_to_int);
	    if (regListN > 0) {
		do_read_register = 0;
		for (i = 0; i < regListN; i++)
		    if (regList[i] < 0 || regList[i] > 0xff)
			do_read_register = -1;
	    }
	    if (do_read_register < 0 || do_read_register > 0xff
		|| regListN == -1) {
		fprintf(stderr,
			"ERROR: register address must be > 0x00 and < 0xff\n");
		return 1;
	    }
	    break;
	default:
	    usage();
	    return 1;
	}
    }

    /* initialise device */
    devh = ubertooth_start(ubertooth_device);
    if (devh == NULL) {
	usage();
	return 1;
    }

    if (do_read_register >= 0) {
	for (i = 0; i < regListN; i++) {
	    r = cmd_read_register(devh, regList[i]);
	    if (r >= 0)
		cc2400_decode(stdout, regList[i], r, verbose);
	}
    }

    return r;
}
