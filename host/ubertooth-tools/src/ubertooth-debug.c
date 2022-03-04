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
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arglist.h"
#include "cc2400.h"

static void usage()
{
    printf
	("ubertooth-debug - command line utility for debugging Ubertooth One\n");
    printf("Usage:\n");
    printf("\t-h this message\n");
    printf("\t-r <reg>[,<reg>[,...]] read the contents of CC2400 register(s)\n");
    printf("\t-r <start>-<end> read a consecutive set of CC2400 register(s)\n");
    printf("\t-U <0-7> set ubertooth device to use (cannot be used with -D)\n");
    printf("\t-D <serial> set ubertooth serial to use (cannot be used with -U)\n");
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
    ubertooth_t *ut = NULL;
    int do_read_register;
    int ubertooth_device = -1;
    char serial_c[34] = {0};
    int device_index = 0, device_serial = 0;
    int *regList = NULL;
    int regListN = 0;
    int i;

    /* set command states to negative as a starter
     * setting to positive is value of specified argument */
    do_read_register = -1;

    while ((opt = getopt(argc, argv, "hU:D:r:v:")) != EOF) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case 'D':
            snprintf(serial_c, strlen(optarg), "%s", optarg);
            device_serial = 1;
            break;
        case 'U':
            ubertooth_device = atoi(optarg);
            device_index = 1;
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

    if (regListN == 0) {
        fprintf(stderr, "At least one register must be provided\n");
        usage();
        return 1;
    }

    if (device_serial && device_index) {
        printf("Error: Cannot use both index and serial simultaneously\n");
        usage();
        return 1;
    }

    /* initialise device */
    if (device_serial)
        ut = ubertooth_start_serial(serial_c);
    else
        ut = ubertooth_start(ubertooth_device);

    if (ut == NULL) {
        usage();
        return 1;
    }

    if (do_read_register >= 0) {
        for (i = 0; i < regListN; i++) {
            r = cmd_read_register(ut->devh, regList[i]);
            if (r >= 0)
                cc2400_decode(stdout, regList[i], r, verbose);
        }
    }

    return r;
}
