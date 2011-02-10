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

#include <stdio.h>
#include <stdlib.h>

#include "ubertooth.h"

static struct libusb_device_handle* find_ubertooth_device(void)
{
	struct libusb_device_handle *devh = NULL;
	devh = libusb_open_device_with_vid_pid(NULL, VENDORID, PRODUCTID);
	return devh;
}

int stream_rx(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks)
{
	u8 buffer[BUFFER_SIZE];
	int r;
	int i, j, k;
	int xfer_blocks;
	int num_xfers;
	int transferred;
	u32 time; /* in 100 nanosecond units */

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

	cmd_rx_syms(devh, num_blocks);

	while (num_xfers--) {
		r = libusb_bulk_transfer(devh, DATA_IN, buffer, xfer_size,
				&transferred, TIMEOUT);
		if (r < 0) {
			fprintf(stderr, "bulk read returned: %d , failed to read\n", r);
			return -1;
		}
		if (transferred != xfer_size) {
			fprintf(stderr, "bad data read size (%d)\n", transferred);
			return -1;
		}
		fprintf(stderr, "transferred %d bytes\n", transferred);

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			time = buffer[4 + 64 * i]
					| (buffer[5 + 64 * i] << 8)
					| (buffer[6 + 64 * i] << 16)
					| (buffer[7 + 64 * i] << 24);
			fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			for (j = 64 * i + 14; j < 64 * i + 64; j++) {
				//printf("%02x", buffer[j]);
				/* output one byte for each received symbol (0 or 1) */
				for (k = 0; k < 8; k++) {
					printf("%c", (buffer[j] & 0x80) >> 7 );
					buffer[j] <<= 1;
				}
			}
		}
		fflush(stderr);
	}
}

void ubertooth_stop(struct libusb_device_handle *devh)
{
	if (devh != NULL)
		libusb_release_interface(devh, 0);
	libusb_close(devh);
	libusb_exit(NULL);
}

struct libusb_device_handle* ubertooth_start()
{
	int r;
	struct libusb_device_handle *devh = NULL;

	r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "libusb_init failed (got 1.0?)\n");
		return NULL;
	}

	devh = find_ubertooth_device();
	if (devh == NULL) {
		fprintf(stderr, "could not find Ubertooth device\n");
		ubertooth_stop(devh);
		return NULL;
	}

	r = libusb_claim_interface(devh, 0);
	if (r < 0) {
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		ubertooth_stop(devh);
		return NULL;
	}

	return devh;
}

int cmd_ping(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_PING, 0, 0,
			NULL, 0, 1000);
	if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_rx_syms(struct libusb_device_handle* devh, u16 num)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RX_SYMBOLS, num, 0,
			NULL, 0, 1000);
	if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_set_usrled(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_USRLED, state, 0,
			NULL, 0, 1000);
	if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_get_usrled(struct libusb_device_handle* devh)
{
	u8 state;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_USRLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return state;
}

int cmd_get_partnum(struct libusb_device_handle* devh)
{
	u8 result[5];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_PARTNUM, 0, 0,
			result, 5, 1000);
	if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	if (result[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", result[0]);
		return 0;
	}
	return result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24);
}

int cmd_get_serial(struct libusb_device_handle* devh)
{
	u8 result[17];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_SERIAL, 0, 0,
			result, 17, 1000);
	if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	if (result[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", result[0]);
		return 0;
	}
	printf("%08x\n", result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24));
	printf("%08x\n", result[5] | (result[6] << 8) | (result[7] << 16) | (result[8] << 24));
	printf("%08x\n", result[9] | (result[10] << 8) | (result[11] << 16) | (result[12] << 24));
	printf("%08x\n", result[13] | (result[14] << 8) | (result[15] << 16) | (result[16] << 24));
	return result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24);
}

int cmd_set_modulation(struct libusb_device_handle* devh, u16 mod)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_MOD, mod, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
	} else if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_set_isp(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_ISP, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_TIMEOUT is the error we expect to get */
	if (r != LIBUSB_ERROR_TIMEOUT) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_reset(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RESET, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE is the error we expect to get */
	if (r != LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_set_paen(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_PAEN, state, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
	} else if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_set_hgm(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_HGM, state, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
	} else if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_tx_test(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_TX_TEST, 0, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
	} else if (r < 0) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}

int cmd_flash(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_FLASH, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE is the error we expect to get */
	if (r != LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "command error %d\n", r);
		return r;
	}
	return 0;
}
