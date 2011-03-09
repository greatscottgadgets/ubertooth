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
#include <bluetooth_packet.h>
#include <bluetooth_piconet.h>
#include <getopt.h>

#define NUM_BANKS 10
#define BANK_LEN 400

char symbols[NUM_BANKS][BANK_LEN];
u8 bank = 0;
u8 channel = 39;
piconet pn;
u8 rx_buf1[BUFFER_SIZE];
u8 rx_buf2[BUFFER_SIZE];
u8 *empty_buf = NULL;
u8 *full_buf = NULL;
u8 really_full = 0;
struct libusb_transfer *rx_xfer = NULL;

static void cb_xfer(struct libusb_transfer *xfer)
{
	int r;
	u8 *tmp;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		fprintf(stderr, "rx_xfer status: %d\n", xfer->status);
		libusb_free_transfer(xfer);
		rx_xfer = NULL;
		return;
	}

	while (really_full)
		fprintf(stderr, "uh oh, full_buf not emptied\n");

	tmp = full_buf;
	full_buf = empty_buf;
	empty_buf = tmp;
	really_full = 1;

	rx_xfer->buffer = empty_buf;

	while (1) {
		r = libusb_submit_transfer(rx_xfer);
		if (r < 0)
			fprintf(stderr, "rx_xfer submission from callback: %d\n", r);
		else
			break;
	}
}

/* this should probably be moved to ubertooth.c and perhaps combined with stream_rx() */
int rx_uap(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks)
{
	int r;
	int i, j, k, m;
	int xfer_blocks;
	int num_xfers;
	//int transferred;
	u32 time; /* in 100 nanosecond units */
	u32 clkn; /* native (local) clock in 625 us */
	u8 clkn_high;
	packet pkt;
	char syms[BANK_LEN * NUM_BANKS];

	/*
	 * A block is 64 bytes transferred over USB (includes 50 bytes of rx symbol
	 * payload).  A transfer consists of one or more blocks.  Consecutive
	 * blocks should be approximately 400 microseconds apart (timestamps about
	 * 4000 apart in units of 100 nanoseconds).
	 */

	if (xfer_size > BUFFER_SIZE)
		xfer_size = BUFFER_SIZE;
	xfer_blocks = xfer_size / 64;
	xfer_size = xfer_blocks * 64;
	num_xfers = num_blocks / xfer_blocks;
	num_blocks = num_xfers * xfer_blocks;

	fprintf(stderr, "rx %d blocks of 64 bytes in %d byte transfers\n",
			num_blocks, xfer_size);

	empty_buf = &rx_buf1[0];
	full_buf = &rx_buf2[0];
	really_full = 0;
	rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(rx_xfer, devh, DATA_IN, empty_buf,
			xfer_size, cb_xfer, NULL, TIMEOUT);

	cmd_rx_syms(devh, num_blocks);

	r = libusb_submit_transfer(rx_xfer);
	if (r < 0) {
		fprintf(stderr, "rx_xfer submission: %d\n", r);
		return -1;
	}

	while (1) {
		while (!really_full) {
			r = libusb_handle_events(NULL);
			if (r < 0) {
				fprintf(stderr, "libusb_handle_events: %d\n", r);
				return -1;
			}
		}

		//fprintf(stderr, "transferred %d bytes\n", transferred);

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			time = full_buf[4 + 64 * i]
					| (full_buf[5 + 64 * i] << 8)
					| (full_buf[6 + 64 * i] << 16)
					| (full_buf[7 + 64 * i] << 24);
			clkn_high = full_buf[3 + 64 * i];
			//fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			//for (j = 64 * i + 14; j < 64 * i + 64; j++) {
			for (j = 0; j < 50; j++) {
				//printf("%02x", full_buf[j]);
				/* output one byte for each received symbol (0 or 1) */
				for (k = 0; k < 8; k++) {
					//printf("%c", (full_buf[j] & 0x80) >> 7 );
					symbols[bank][j * 8 + k] = (full_buf[j + 14 + i * 64] & 0x80) >> 7;
					full_buf[j + 14 + i * 64] <<= 1;
				}
			}

			/* awfully repetitious */
			m = 0;
			for (j = 0; j < 2; j++)
				for (k = 0; k < BANK_LEN; k++)
					syms[m++] = symbols[(j + 1 + bank) % NUM_BANKS][k];

			r = sniff_ac(syms, BANK_LEN);
			if  (r > -1) {

				for (j = 2; j < NUM_BANKS; j++)
					for (k = 0; k < BANK_LEN; k++)
						syms[m++] = symbols[(j + 1 + bank) % NUM_BANKS][k];

				clkn = (clkn_high << 19) | ((time + r * 10) / 6250);

				init_packet(&pkt, &syms[r], BANK_LEN * NUM_BANKS - r);
				pkt.clkn = clkn;
				pkt.channel = channel;

				if (pkt.LAP == pn.LAP && header_present(&pkt)) {
					printf("\nGOT PACKET on channel %d, LAP = %06x at time stamp %u, clkn %u\n",
							channel, pkt.LAP, time + r * 10, clkn);
					if (UAP_from_header(&pkt, &pn))
						exit(0);
				}
			}
			bank = (bank + 1) % NUM_BANKS;
		}
		really_full = 0;
		fflush(stderr);
	}
}

static void usage()
{
	printf("ubertooth-uap - passive UAP discovery for a particular LAP\n");
	printf("Usage:\n");
	printf("\t-l LAP (in hexadecimal)\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int argcount = 0;
	char *end;
	int r = 0;
	struct libusb_device_handle *devh = ubertooth_start();

	if (devh == NULL)
		return 1;

	init_piconet(&pn);

	while ((opt=getopt(argc,argv,"l:")) != EOF) {
		switch(opt) {
		case 'l':
			pn.LAP = strtol(optarg, &end, 16);
			if (end != optarg)
				++argcount;
			break;
		default:
			usage();
			return 1;
		}
	}
	if (argcount != 1) {
		usage();
		return 1;
	}

	rx_uap(devh, 512, 0);

	//FIXME make sure xfers are not active
	libusb_free_transfer(rx_xfer);
	ubertooth_stop(devh);

	return 0;
}
