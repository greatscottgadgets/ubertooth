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

#include <stdio.h>
#include <stdlib.h>
#include <bluetooth_packet.h>

#include "ubertooth.h"

//this stuff should probably be in a struct managed by the calling program
char symbols[NUM_BANKS][BANK_LEN];
u8 *empty_buf = NULL;
u8 *full_buf = NULL;
u8 really_full = 0;
struct libusb_transfer *rx_xfer = NULL;

void show_libusb_error(int error_code)
{
    switch (error_code) {
	    case LIBUSB_ERROR_TIMEOUT:
	        fprintf(stderr, "libUSB Error: Timeout (%d)\n", error_code);
	        break;
	    case LIBUSB_ERROR_NO_DEVICE:
	        fprintf(stderr, "libUSB Error: No Device, did you disconnect the ubertooth? (%d)\n", error_code);
	        break;
	    case LIBUSB_ERROR_ACCESS:
	        fprintf(stderr, "libUSB Error: Insufficient Permissions (%d)\n", error_code);
	        break;
	    default:
	        fprintf(stderr, "command error %d\n", error_code);
	        break;
	}
}

static struct libusb_device_handle* find_ubertooth_device(void)
{
	struct libusb_device_handle *devh = NULL;
	devh = libusb_open_device_with_vid_pid(NULL, VENDORID, PRODUCTID);
	return devh;
}

static void cb_xfer(struct libusb_transfer *xfer)
{
	int r;
	uint8_t *tmp;

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

int stream_rx_usb(struct libusb_device_handle* devh, int xfer_size,
		uint16_t num_blocks, rx_callback cb, void* cb_args)
{
	int r;
	int i, j, k;
	int xfer_blocks;
	int num_xfers;
	uint8_t bank = 0;
	uint8_t rx_buf1[BUFFER_SIZE];
	uint8_t rx_buf2[BUFFER_SIZE];

	/*
	 * A block is 64 bytes transferred over USB (includes 50 bytes of rx symbol
	 * payload).  A transfer consists of one or more blocks.  Consecutive
	 * blocks should be approximately 400 microseconds apart (timestamps about
	 * 4000 apart in units of 100 nanoseconds).
	 */

	if (xfer_size > BUFFER_SIZE)
		xfer_size = BUFFER_SIZE;
	xfer_blocks = xfer_size / PKT_LEN;
	xfer_size = xfer_blocks * PKT_LEN;
	num_xfers = num_blocks / xfer_blocks;
	num_blocks = num_xfers * xfer_blocks;

	//fprintf(stderr, "rx %d blocks of 64 bytes in %d byte transfers\n",
			//num_blocks, xfer_size);

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
		//fprintf(stderr, "transfer completed\n");

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			(*cb)(cb_args, full_buf + PKT_LEN * i, bank);
			bank = (bank + 1) % NUM_BANKS;
		}
		really_full = 0;
		fflush(stderr);
	}
}

/* file should be in full USB packet format (ubertooth-dump -f) */
int stream_rx_file(FILE* fp, uint16_t num_blocks, rx_callback cb, void* cb_args)
{
	uint8_t bank = 0;
	uint8_t buf[BUFFER_SIZE];

	//fprintf(stderr, "reading %d blocks of 64 bytes from file\n", num_blocks);

	while (fread(buf, sizeof(buf[0]), PKT_LEN, fp)) {
		(*cb)(cb_args, buf, bank);
		bank = (bank + 1) % NUM_BANKS;
	}
}

static uint32_t extract_time(uint8_t* buf)
{
	return buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
}

static uint32_t extract_clkn_high(uint8_t* buf)
{
	return buf[3];
}

static void unpack_symbols(uint8_t* buf, char* unpacked)
{
	int i, j;

	buf += SYM_OFFSET;
	for (i = 0; i < SYM_LEN; i++) {
		/* output one byte for each received symbol (0x00 or 0x01) */
		for (j = 0; j < 8; j++) {
			unpacked[i * 8 + j] = (buf[i] & 0x80) >> 7;
			buf[i] <<= 1;
		}
	}
}

static void cb_lap(void* args, uint8_t* buf, int bank)
{
	char syms[BANK_LEN * NUM_BANKS];
	int i, j, k;
	int r;
	uint32_t time; /* in 100 nanosecond units */
	uint8_t clkn_high;
	packet pkt;
	uint8_t channel = 39; //FIXME hard coded

	time = extract_time(buf);
	clkn_high = extract_clkn_high(buf);
	unpack_symbols(buf, symbols[bank]);
	//fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);

	/* awfully repetitious */
	k = 0;
	for (i = 0; i < 2; i++)
		for (j = 0; j < BANK_LEN; j++)
			syms[k++] = symbols[(i + 1 + bank) % NUM_BANKS][j];

	r = sniff_ac(syms, BANK_LEN);

	if (r > -1) {
		for (i = 2; i < NUM_BANKS; i++)
			for (j = 0; j < BANK_LEN; j++)
				syms[k++] = symbols[(i + 1 + bank) % NUM_BANKS][j];

		init_packet(&pkt, &syms[r], BANK_LEN * NUM_BANKS - r);
		pkt.clkn = (clkn_high << 19) | ((time + r * 10) / 6250);
		pkt.channel = channel;

		printf("GOT PACKET on channel %d, LAP = %06x at time stamp %u, clkn %u\n",
				channel, pkt.LAP, time + r * 10, pkt.clkn);
	}
}

/* sniff all packets and identify LAPs */
void rx_lap(struct libusb_device_handle* devh)
{
	stream_rx_usb(devh, XFER_LEN, 0, cb_lap, NULL);
}

/* sniff all packets and identify LAPs */
void rx_lap_file(FILE* fp)
{
	stream_rx_file(fp, 0, cb_lap, NULL);
}

static void cb_uap(void* args, uint8_t* buf, int bank)
{
	char syms[BANK_LEN * NUM_BANKS];
	int i, j, k;
	int r;
	uint32_t time; /* in 100 nanosecond units */
	uint8_t clkn_high;
	packet pkt;
	uint8_t channel = 39; //FIXME hard coded
	piconet* pn = (piconet *)args;

	time = extract_time(buf);
	clkn_high = extract_clkn_high(buf);
	unpack_symbols(buf, symbols[bank]);
	//fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);

	/* awfully repetitious */
	k = 0;
	for (i = 0; i < 2; i++)
		for (j = 0; j < BANK_LEN; j++)
			syms[k++] = symbols[(i + 1 + bank) % NUM_BANKS][j];

	r = find_ac(syms, BANK_LEN, pn->LAP);

	if (r > -1) {
		for (i = 2; i < NUM_BANKS; i++)
			for (j = 0; j < BANK_LEN; j++)
				syms[k++] = symbols[(i + 1 + bank) % NUM_BANKS][j];

		init_packet(&pkt, &syms[r], BANK_LEN * NUM_BANKS - r);
		pkt.clkn = (clkn_high << 19) | ((time + r * 10) / 6250);
		pkt.channel = channel;

		if (pkt.LAP == pn->LAP && header_present(&pkt)) {
			printf("\nGOT PACKET on channel %d, LAP = %06x at time stamp %u, clkn %u\n",
					channel, pkt.LAP, time + r * 10, pkt.clkn);
			if (UAP_from_header(&pkt, pn))
				exit(0);
		}
	}
}

/* sniff one target LAP until the UAP is determined */
void rx_uap(struct libusb_device_handle* devh, piconet* pn)
{
	stream_rx_usb(devh, XFER_LEN, 0, cb_uap, pn);
}

/* sniff one target LAP until the UAP is determined */
void rx_uap_file(FILE* fp, piconet* pn)
{
	stream_rx_file(fp, 0, cb_uap, pn);
}

static void cb_dump(void* args, uint8_t* buf, int bank)
{
	int i;
	uint32_t time; /* in 100 nanosecond units */

	time = extract_time(buf);
	unpack_symbols(buf, symbols[bank]);
	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
	for (i = 0; i < BANK_LEN; i++)
		printf("%c", symbols[bank][i]);
}

static void cb_dump_full(void* args, uint8_t* buf, int bank)
{
	int i;
	uint32_t time; /* in 100 nanosecond units */

	time = extract_time(buf);
	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
	for (i = 0; i < PKT_LEN; i++)
		printf("%c", buf[i]);
}

/* dump received symbols to stdout */
void rx_dump(struct libusb_device_handle* devh, int full)
{
	if (full)
		stream_rx_usb(devh, XFER_LEN, 0, cb_dump_full, NULL);
	else
		stream_rx_usb(devh, XFER_LEN, 0, cb_dump, NULL);
}

int specan(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks,
		u16 low_freq, u16 high_freq)
{
	u8 buffer[BUFFER_SIZE];
	int r;
	int i, j;
	int xfer_blocks;
	int num_xfers;
	int transferred;
	int frequency;
	u32 time; /* in 100 nanosecond units */

	if (xfer_size > BUFFER_SIZE)
		xfer_size = BUFFER_SIZE;
	xfer_blocks = xfer_size / PKT_LEN;
	xfer_size = xfer_blocks * PKT_LEN;
	num_xfers = num_blocks / xfer_blocks;
	num_blocks = num_xfers * xfer_blocks;

	fprintf(stderr, "rx %d blocks of 64 bytes in %d byte transfers\n",
			num_blocks, xfer_size);

	cmd_specan(devh, low_freq, high_freq);

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
			time = buffer[4 + PKT_LEN * i]
					| (buffer[5 + PKT_LEN * i] << 8)
					| (buffer[6 + PKT_LEN * i] << 16)
					| (buffer[7 + PKT_LEN * i] << 24);
			fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			for (j = PKT_LEN * i + SYM_OFFSET; j < PKT_LEN * i + 62; j += 3) {
				frequency = (buffer[j] << 8) | buffer[j + 1];
				if (buffer[j + 2] > 150) //FIXME 
					printf("%f, %d, %d\n", ((double)time)/10000000, frequency, buffer[j + 2]);
				if (frequency == high_freq)
					printf("\n");
			}
		}
		fflush(stderr);
	}
}

void ubertooth_stop(struct libusb_device_handle *devh)
{
	//FIXME make sure xfers are not active
	libusb_free_transfer(rx_xfer);
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
		show_libusb_error(r);
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
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_specan(struct libusb_device_handle* devh, u16 low_freq, u16 high_freq)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SPECAN,
			low_freq, high_freq, NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_led_specan(struct libusb_device_handle* devh, u16 rssi_threshold)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_LED_SPECAN,
			rssi_threshold, 0, NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
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
		show_libusb_error(r);
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
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_set_rxled(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_RXLED, state, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_rxled(struct libusb_device_handle* devh)
{
	u8 state;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_RXLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_set_txled(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_TXLED, state, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_txled(struct libusb_device_handle* devh)
{
	u8 state;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_TXLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_get_modulation(struct libusb_device_handle* devh)
{
	u8 modulation;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_MOD, 0, 0,
			&modulation, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return modulation;
}

int cmd_get_channel(struct libusb_device_handle* devh)
{
	u8 result[2];
	int r;
	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_CHANNEL, 0, 0,
			result, 2, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return result[0] | (result[1] << 8);
}


int cmd_set_channel(struct libusb_device_handle* devh, u16 channel)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_CHANNEL, channel, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_partnum(struct libusb_device_handle* devh)
{
	u8 result[5];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_PARTNUM, 0, 0,
			result, 5, 1000);
	if (r < 0) {
		show_libusb_error(r);
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
		show_libusb_error(r);
		return r;
	}
	if (result[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", result[0]);
		return 0;
	}
	//FIXME shouldn't print to stdout, should return complete serial number
	printf("%08x", result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24));
	printf("%08x", result[5] | (result[6] << 8) | (result[7] << 16) | (result[8] << 24));
	printf("%08x", result[9] | (result[10] << 8) | (result[11] << 16) | (result[12] << 24));
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
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_set_isp(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_ISP, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE or LIBUSB_ERROR_OTHER is expected */
	if ((r != LIBUSB_ERROR_PIPE) && (r != LIBUSB_ERROR_OTHER)) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_reset(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RESET, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE or LIBUSB_ERROR_OTHER is expected */
	if ((r != LIBUSB_ERROR_PIPE) && (r != LIBUSB_ERROR_OTHER)) {
		show_libusb_error(r);
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
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
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
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
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
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_flash(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_FLASH, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE or LIBUSB_ERROR_OTHER is expected */
	if ((r != LIBUSB_ERROR_PIPE) && (r != LIBUSB_ERROR_OTHER)) {
	    show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_palevel(struct libusb_device_handle* devh)
{
	u8 level;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_PALEVEL, 0, 0,
			&level, sizeof(level), 3000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return level;
}

int cmd_set_palevel(struct libusb_device_handle* devh, u16 level)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_PALEVEL, level, 0,
			NULL, 0, 3000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_get_rangeresult(struct libusb_device_handle* devh,
		rangetest_result *rr)
{
	u8 result[5];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_RANGE_CHECK, 0, 0,
			result, sizeof(result), 3000);
	if (r < LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}

	rr->valid       = result[0];
	rr->request_pa  = result[1];
	rr->request_num = result[2];
	rr->reply_pa    = result[3];
	rr->reply_num   = result[4];

	return 0;
}

int cmd_range_test(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RANGE_TEST, 0, 0,
			NULL, 0, 1000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_repeater(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_REPEATER, 0, 0,
			NULL, 0, 1000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_get_rev_num(struct libusb_device_handle* devh)
{
	u8 result[2];
	int r;
	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_REV_NUM, 0, 0,
			result, 2, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return result[0] | (result[1] << 8);
}

int cmd_get_board_id(struct libusb_device_handle* devh)
{
	u8 board_id;
	int r;
	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_BOARD_ID, 0, 0,
			&board_id, 1, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return board_id;
}
