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
#include "bluetooth_packet.h"
#include "bluetooth_piconet.h"

#define NUM_BANKS 10
#define BANK_LEN 400

enum buffer_status {
	IDLE = 0,
	FILLING = 1,
	EMPTYING = 2
};

char symbols[NUM_BANKS][BANK_LEN];
u8 bank = 0;
u8 channel = 39;
piconet pn;
u8 rx_buf1[BUFFER_SIZE];
u8 rx_buf2[BUFFER_SIZE];
struct libusb_transfer *rx_xfer1 = NULL;
struct libusb_transfer *rx_xfer2 = NULL;
int rx_xfer1_state = IDLE;
int rx_xfer2_state = IDLE;

static void cb_xfer1(struct libusb_transfer *xfer)
{
	int r;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		fprintf(stderr, "rx_xfer1 status: %d\n", xfer->status);
		libusb_free_transfer(xfer);
		rx_xfer1 = NULL;
		return;
	}

	rx_xfer1_state = EMPTYING;

	if (rx_xfer2_state == IDLE) {
		rx_xfer2_state = FILLING;
		r = libusb_submit_transfer(rx_xfer2);
		if (r < 0)
			fprintf(stderr, "rx_xfer2 submission from callback: %d\n", r);
	} else {
		fprintf(stderr, "rx_xfer2 not yet idle: %u\n", rx_xfer2_state);
	}
}

static void cb_xfer2(struct libusb_transfer *xfer)
{
	int r;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		fprintf(stderr, "rx_xfer2 status: %d\n", xfer->status);
		libusb_free_transfer(xfer);
		rx_xfer2 = NULL;
		return;
	}

	rx_xfer2_state = EMPTYING;

	if (rx_xfer1_state == IDLE) {
		rx_xfer1_state = FILLING;
		r = libusb_submit_transfer(rx_xfer1);
		if (r < 0)
			fprintf(stderr, "rx_xfer1 submission from callback: %d\n", r);
	} else {
		fprintf(stderr, "rx_xfer1 not yet idle: %u\n", rx_xfer1_state);
	}
}

/* this should probably be moved to ubertooth.c and perhaps combined with stream_rx() */
int rx_lap(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks)
{
	u8 *buffer = NULL;
	int r;
	int i, j, k, m;
	int xfer_blocks;
	int num_xfers;
	int *state = NULL;
	//int transferred;
	u32 time; /* in 100 nanosecond units */
	u32 clkn; /* native (local) clock in 625 us */
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

	rx_xfer1 = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(rx_xfer1, devh, DATA_IN, rx_buf1,
			xfer_size, cb_xfer1, NULL, TIMEOUT);

	rx_xfer2 = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(rx_xfer2, devh, DATA_IN, rx_buf2,
			xfer_size, cb_xfer2, NULL, TIMEOUT);

	cmd_rx_syms(devh, num_blocks);

	rx_xfer1_state = FILLING;
	r = libusb_submit_transfer(rx_xfer1);
	if (r < 0) {
		fprintf(stderr, "rx_xfer1 submission: %d\n", r);
		return -1;
	}

	while (1) {
		while (1) {
			r = libusb_handle_events(NULL);
			if (r < 0) {
				fprintf(stderr, "libusb_handle_events: %d\n", r);
				return -1;
			}
			if (rx_xfer1_state == EMPTYING) {
				buffer = &rx_buf1[0];
				state = &rx_xfer1_state;
				break;
			}
			if (rx_xfer2_state == EMPTYING) {
				buffer = &rx_buf2[0];
				state = &rx_xfer2_state;
				break;
			}
		}

		//fprintf(stderr, "transferred %d bytes\n", transferred);

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			time = buffer[4 + 64 * i]
					| (buffer[5 + 64 * i] << 8)
					| (buffer[6 + 64 * i] << 16)
					| (buffer[7 + 64 * i] << 24);
			//fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			//for (j = 64 * i + 14; j < 64 * i + 64; j++) {
			for (j = 0; j < 50; j++) {
				//printf("%02x", buffer[j]);
				/* output one byte for each received symbol (0 or 1) */
				for (k = 0; k < 8; k++) {
					//printf("%c", (buffer[j] & 0x80) >> 7 );
					symbols[bank][j * 8 + k] = (buffer[j + 14 + i * 64] & 0x80) >> 7;
					buffer[j + 14 + i * 64] <<= 1;
				}
			}

			/* awfully repetitious */
			m = 0;
			for (j = 0; j < NUM_BANKS; j++)
				for (k = 0; k < BANK_LEN; k++)
					syms[m++] = symbols[(j + 1 + bank) % NUM_BANKS][k];
			bank = (bank + 1) % NUM_BANKS;

			r = sniff_ac(syms, BANK_LEN);
			if  (r > -1) {

				clkn = (time + r * 10) / 6250; //FIXME short wraparound

				//FIXME put into initializer in btbb
				for (j = 0; j < (BANK_LEN * NUM_BANKS - r); j++)
					pkt.symbols[j] = syms[r + j];
				pkt.LAP = air_to_host32(&pkt.symbols[38], 24);
				pkt.length = BANK_LEN * NUM_BANKS - r;
				pkt.whitened = 1;
				pkt.have_UAP = 0;
				pkt.have_NAP = 0;
				pkt.have_clk6 = 0;
				pkt.have_clk27 = 0;
				pkt.have_payload = 0;
				pkt.payload_length = 0;
				/* if clkn and channel are known: */
				pkt.clkn = clkn;
				pkt.channel = channel;

				if (pkt.LAP == pn.LAP && header_present(&pkt)) {
					printf("\nGOT PACKET on channel %d, LAP = %06x at time stamp %u, clkn %u\n",
							channel, pkt.LAP, time + r * 10, clkn);
					if (UAP_from_header(&pkt, &pn))
						exit(0);
				}
			}
		}
		*state = IDLE;
		fflush(stderr);
	}
}

int main()
{
	struct libusb_device_handle *devh = ubertooth_start();

	if (devh == NULL)
		return 1;

	pn.LAP = 0x4831dd;
	pn.got_first_packet = 0;
	pn.packets_observed = 0;
	pn.total_packets_observed = 0;
	pn.hop_reversal_inited = 0;
	pn.afh = 0;
	pn.looks_like_afh = 0;
	pn.have_UAP = 0;
	pn.have_NAP = 0;
	pn.have_clk6 = 0;
	pn.have_clk27 = 0;

	rx_lap(devh, 512, 0xFFFF);

	//FIXME make sure xfers are not active
	libusb_free_transfer(rx_xfer1);
	libusb_free_transfer(rx_xfer2);
	ubertooth_stop(devh);

	return 0;
}
