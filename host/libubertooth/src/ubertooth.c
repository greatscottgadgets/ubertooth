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
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "ubertooth_callback.h"
#include "ubertooth.h"
#include "ubertooth_control.h"
#include "ubertooth_interface.h"

#ifndef RELEASE
#define RELEASE "unknown"
#endif
#ifndef VERSION
#define VERSION "unknown"
#endif


uint32_t systime;
FILE* infile = NULL;
FILE* dumpfile = NULL;
int max_ac_errors = 2;

unsigned int packet_counter_max;

void print_version() {
	printf("libubertooth %s (%s), libbtbb %s (%s)\n", VERSION, RELEASE,
	       btbb_get_version(), btbb_get_release());
}

ubertooth_t* cleanup_devh = NULL;
static void cleanup(int sig __attribute__((unused)))
{
	if (cleanup_devh)
		cleanup_devh->stop_ubertooth = 1;
}

void register_cleanup_handler(ubertooth_t* ut) {
	cleanup_devh = ut;

	/* Clean up on exit. */
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);
}

ubertooth_t* timeout_dev = NULL;
void stop_transfers(int sig __attribute__((unused))) {
	if (timeout_dev)
		timeout_dev->stop_ubertooth = 1;
}

void ubertooth_set_timeout(ubertooth_t* ut, int seconds) {
	/* Upon SIGALRM, call stop_transfers() */
	if (signal(SIGALRM, stop_transfers) == SIG_ERR) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}
	timeout_dev = ut;
	alarm(seconds);
}

static struct libusb_device_handle* find_ubertooth_device(int ubertooth_device)
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **usb_list = NULL;
	struct libusb_device_handle *devh = NULL;
	struct libusb_device_descriptor desc;
	int usb_devs, i, r, ret, ubertooths = 0;
	int ubertooth_devs[] = {0,0,0,0,0,0,0,0};

	usb_devs = libusb_get_device_list(ctx, &usb_list);
	for(i = 0 ; i < usb_devs ; ++i) {
		r = libusb_get_device_descriptor(usb_list[i], &desc);
		if(r < 0)
			fprintf(stderr, "couldn't get usb descriptor for dev #%d!\n", i);
		if ((desc.idVendor == TC13_VENDORID && desc.idProduct == TC13_PRODUCTID)
		    || (desc.idVendor == U0_VENDORID && desc.idProduct == U0_PRODUCTID)
		    || (desc.idVendor == U1_VENDORID && desc.idProduct == U1_PRODUCTID))
		{
			ubertooth_devs[ubertooths] = i;
			ubertooths++;
		}
	}
	if(ubertooths == 1) {
		ret = libusb_open(usb_list[ubertooth_devs[0]], &devh);
		if (ret)
			show_libusb_error(ret);
	}
	else if (ubertooths == 0)
		return NULL;
	else {
		if (ubertooth_device < 0) {
			fprintf(stderr, "multiple Ubertooth devices found! Use '-U' to specify device number\n");
			u8 serial[17], r;
			for(i = 0 ; i < ubertooths ; ++i) {
				libusb_get_device_descriptor(usb_list[ubertooth_devs[i]], &desc);
				ret = libusb_open(usb_list[ubertooth_devs[i]], &devh);
				if (ret) {
					fprintf(stderr, "  Device %d: ", i);
					show_libusb_error(ret);
				}
				else {
					r = cmd_get_serial(devh, serial);
					if(r==0) {
						fprintf(stderr, "  Device %d: ", i);
						print_serial(serial, stderr);
					}
					libusb_close(devh);
				}
			}
			devh = NULL;
		} else {
			ret = libusb_open(usb_list[ubertooth_devs[ubertooth_device]], &devh);
			if (ret) {
					show_libusb_error(ret);
					devh = NULL;
				}
		}
	}
	return devh;
}


/*
 * based on http://libusb.sourceforge.net/api-1.0/group__asyncio.html#ga9fcb2aa23d342060ebda1d0cf7478856
 */
static void rx_xfer_status(int status)
{
	char *error_name = "";

	switch (status) {
		case LIBUSB_TRANSFER_ERROR:
			error_name="Transfer error.";
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
			error_name="Transfer timed out.";
			break;
		case LIBUSB_TRANSFER_CANCELLED:
			error_name="Transfer cancelled.";
			break;
		case LIBUSB_TRANSFER_STALL:
			error_name="Halt condition detected, or control request not supported.";
			break;
		case LIBUSB_TRANSFER_NO_DEVICE:
			error_name="Device disconnected.";
			break;
		case LIBUSB_TRANSFER_OVERFLOW:
			error_name="Device sent more data than requested.";
			break;
	}
	fprintf(stderr,"rx_xfer status: %s (%d)\n",error_name,status);
}

static void cb_xfer(struct libusb_transfer *xfer)
{
	int r;
	uint8_t *tmp;
	ubertooth_t* ut = (ubertooth_t*)xfer->user_data;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		if(xfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
			r = libusb_submit_transfer(ut->rx_xfer);
			if (r < 0)
				fprintf(stderr, "Failed to submit USB transfer (%d)\n", r);
			return;
		}
		if(xfer->status != LIBUSB_TRANSFER_CANCELLED)
			rx_xfer_status(xfer->status);
		libusb_free_transfer(xfer);
		ut->rx_xfer = NULL;
		return;
	}

	if(ut->usb_really_full) {
		/* This should never happen, but we'd prefer to error and exit
		 * than to clobber existing data
		 */
		fprintf(stderr, "uh oh, full_usb_buf not emptied\n");
		ut->stop_ubertooth = 1;
	}

	if(ut->stop_ubertooth)
		return;

	tmp = ut->full_usb_buf;
	ut->full_usb_buf = ut->empty_usb_buf;
	ut->empty_usb_buf = tmp;
	ut->usb_really_full = 1;
	ut->rx_xfer->buffer = ut->empty_usb_buf;

	r = libusb_submit_transfer(ut->rx_xfer);
	if (r < 0)
		fprintf(stderr, "Failed to submit USB transfer (%d)\n", r);
}

int ubertooth_bulk_init(ubertooth_t* ut)
{
	int r;
	uint8_t rx_buf1[BUFFER_SIZE];
	uint8_t rx_buf2[BUFFER_SIZE];

	ut->empty_usb_buf = &rx_buf1[0];
	ut->full_usb_buf = &rx_buf2[0];
	ut->usb_really_full = 0;
	ut->rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(ut->rx_xfer, ut->devh, DATA_IN, ut->empty_usb_buf,
	                          XFER_LEN, cb_xfer, ut, TIMEOUT);

	r = libusb_submit_transfer(ut->rx_xfer);
	if (r < 0) {
		fprintf(stderr, "rx_xfer submission: %d\n", r);
		return -1;
	}
	return 0;
}

void ubertooth_bulk_wait(ubertooth_t* ut)
{
	int r;

	while (!ut->usb_really_full) {
		r = libusb_handle_events(NULL);
		if (r < 0) {
			if (r == LIBUSB_ERROR_INTERRUPTED)
				break;
			show_libusb_error(r);
		}
	}
}

int ubertooth_bulk_receive(ubertooth_t* ut, rx_callback cb, void* cb_args)
{
	int i, r;
	usb_pkt_rx* rx;

	if (!ut->usb_really_full)
	{
		r = libusb_handle_events(NULL);
		if (r < 0 && r != LIBUSB_ERROR_INTERRUPTED)
			show_libusb_error(r);
	}

	if (ut->usb_really_full) {
		/* process each received block */
		for (i = 0; i < PKTS_PER_XFER; i++) {
			rx = (usb_pkt_rx*)(ut->full_usb_buf + PKT_LEN * i);
			if(rx->pkt_type != KEEP_ALIVE) {
				ringbuffer_add(ut->packets, rx);
				(*cb)(ut, cb_args);
			}
			if(ut->stop_ubertooth) {
				if(ut->rx_xfer)
					libusb_cancel_transfer(ut->rx_xfer);
				return 1;
			}
		}
		ut->usb_really_full = 0;
		fflush(stderr);
		return 0;
	} else {
		return -1;
	}
}

static int stream_rx_usb(ubertooth_t* ut, rx_callback cb, void* cb_args)
{
	// init USB transfer
	int r = ubertooth_bulk_init(ut);
	if (r < 0)
		return r;

	// tell ubertooth to send packets
	r = cmd_rx_syms(ut->devh);
	if (r < 0)
		return r;

	// receive and process each packet
	while(!ut->stop_ubertooth) {
		ubertooth_bulk_wait(ut);
		r = ubertooth_bulk_receive(ut, cb, cb_args);
		if (r == 1)
			return 1;
	}
	return 0;
}

/* file should be in full USB packet format (ubertooth-dump -f) */
int stream_rx_file(ubertooth_t* ut, FILE* fp, rx_callback cb, void* cb_args)
{
	uint8_t buf[BUFFER_SIZE];
	size_t nitems;

	while(1) {
		uint32_t systime_be;
		nitems = fread(&systime_be, sizeof(systime_be), 1, fp);
		if (nitems != 1)
			return 0;
		systime = (time_t)be32toh(systime_be);

		nitems = fread(buf, sizeof(buf[0]), PKT_LEN, fp);
		if (nitems != PKT_LEN)
			return 0;
		ringbuffer_add(ut->packets, (usb_pkt_rx*)buf);
		(*cb)(ut, cb_args);
	}
}

/* Receive and process packets. For now, returning from
 * stream_rx_usb() means that UAP and clocks have been found, and that
 * hopping should be started. A more flexible framework would be
 * nice. */
void rx_live(ubertooth_t* ut, btbb_piconet* pn, int timeout)
{
	int r = btbb_init(max_ac_errors);
	if (r < 0)
		return;

	if (timeout)
		ubertooth_set_timeout(ut, timeout);

	if (pn != NULL && btbb_piconet_get_flag(pn, BTBB_CLK27_VALID))
		cmd_set_clock(ut->devh, 0);
	else {
		stream_rx_usb(ut, cb_br_rx, pn);
		/* Allow pending transfers to finish */
		sleep(1);
	}
	/* Used when follow_pn is preset OR set by stream_rx_usb above
	 * i.e. This cannot be rolled in to the above if...else
	 */
	if (pn != NULL && btbb_piconet_get_flag(pn, BTBB_CLK27_VALID)) {
		ut->stop_ubertooth = 0;
		ut->usb_really_full = 0;
		// cmd_stop(ut->devh);
		cmd_set_bdaddr(ut->devh, btbb_piconet_get_bdaddr(pn));
		cmd_start_hopping(ut->devh, btbb_piconet_get_clk_offset(pn), 0);
		stream_rx_usb(ut, cb_br_rx, pn);
	}
}

void rx_afh(ubertooth_t* ut, btbb_piconet* pn, int timeout)
{
	int r = btbb_init(max_ac_errors);
	if (r < 0)
		return;

	cmd_set_channel(ut->devh, 9999);

	if (timeout) {
		ubertooth_set_timeout(ut, timeout);

		cmd_afh(ut->devh);
		stream_rx_usb(ut, cb_afh_initial, pn);

		cmd_stop(ut->devh);

		btbb_print_afh_map(pn);
	}

	/*
	 * Monitor changes in AFH channel map
	 */
	cmd_clear_afh_map(ut->devh);
	cmd_afh(ut->devh);
	stream_rx_usb(ut, cb_afh_monitor, pn);
}

void rx_afh_r(ubertooth_t* ut, btbb_piconet* pn, int timeout __attribute__((unused)))
{
	static uint32_t lasttime;

	int r = btbb_init(max_ac_errors);
	int i, j;
	if (r < 0)
		return;

	cmd_set_channel(ut->devh, 9999);

	cmd_afh(ut->devh);

	// init USB transfer
	r = ubertooth_bulk_init(ut);
	if (r < 0)
		return;

	// tell ubertooth to send packets
	r = cmd_rx_syms(ut->devh);
	if (r < 0)
		return;

	// receive and process each packet
	while(1) {
		// libusb_handle_events(NULL);
		ubertooth_bulk_wait(ut);
		r = ubertooth_bulk_receive(ut, cb_afh_r, pn);
		if(lasttime < time(NULL)) {
			lasttime = time(NULL);
			printf("%u ", (uint32_t)time(NULL));
			// btbb_print_afh_map(pn);

			uint8_t* afh_map = btbb_piconet_get_afh_map(pn);
			for (i=0; i<10; i++)
				for (j=0; j<8; j++)
					if (afh_map[i] & (1<<j))
						printf("1");
					else
						printf("0");
			printf("\n");
		}
		if (r == 1)
			return;
	}
}

/* sniff one target LAP until the UAP is determined */
void rx_file(FILE* fp, btbb_piconet* pn)
{
	int r = btbb_init(max_ac_errors);
	if (r < 0)
		return;

	ubertooth_t* ut = ubertooth_init();
	if (ut == NULL)
		return;

	stream_rx_file(ut, fp, cb_br_rx, pn);
}

void rx_btle_file(FILE* fp)
{
	ubertooth_t* ut = ubertooth_init();
	if (ut == NULL)
		return;

	stream_rx_file(ut, fp, cb_btle, NULL);
}

static void cb_dump_bitstream(ubertooth_t* ut, void* args __attribute__((unused)))
{
	int i;
	char nl = '\n';

	char bitstream[BANK_LEN];
	memcpy(bitstream, ringbuffer_top_bt(ut->packets), BANK_LEN);

	// convert to ascii
	for (i = 0; i < BANK_LEN; ++i)
		bitstream[i] += 0x30;

	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n",
	        ringbuffer_top_usb(ut->packets)->clk100ns);
	if (dumpfile == NULL) {
		fwrite(bitstream, sizeof(u8), BANK_LEN, stdout);
		fwrite(&nl, sizeof(u8), 1, stdout);
	} else {
		fwrite(bitstream, sizeof(u8), BANK_LEN, dumpfile);
		fwrite(&nl, sizeof(u8), 1, dumpfile);
	}
}

static void cb_dump_full(ubertooth_t* ut, void* args __attribute__((unused)))
{
	usb_pkt_rx* rx = ringbuffer_top_usb(ut->packets);

	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", rx->clk100ns);
	uint32_t time_be = htobe32((uint32_t)time(NULL));
	if (dumpfile == NULL) {
		fwrite(&time_be, 1, sizeof(time_be), stdout);
		fwrite((uint8_t*)rx, sizeof(u8), PKT_LEN, stdout);
	} else {
		fwrite(&time_be, 1, sizeof(time_be), dumpfile);
		fwrite((uint8_t*)rx, sizeof(u8), PKT_LEN, dumpfile);
		fflush(dumpfile);
	}
}

/* dump received symbols to stdout */
void rx_dump(ubertooth_t* ut, int bitstream)
{
	if (bitstream)
		stream_rx_usb(ut, cb_dump_bitstream, NULL);
	else
		stream_rx_usb(ut, cb_dump_full, NULL);
}

void ubertooth_stop(ubertooth_t* ut)
{
	/* make sure xfers are not active */
	if(ut->rx_xfer != NULL)
		libusb_cancel_transfer(ut->rx_xfer);
	if (ut->devh != NULL) {
		cmd_stop(ut->devh);
		libusb_release_interface(ut->devh, 0);
	}
	libusb_close(ut->devh);
	libusb_exit(NULL);

#ifdef ENABLE_PCAP
	if (ut->h_pcap_bredr) {
		btbb_pcap_close(ut->h_pcap_bredr);
		ut->h_pcap_bredr = NULL;
	}
	if (ut->h_pcap_le) {
		lell_pcap_close(ut->h_pcap_le);
		ut->h_pcap_le = NULL;
	}
#endif

	if (ut->h_pcapng_bredr) {
		btbb_pcapng_close(ut->h_pcapng_bredr);
		ut->h_pcapng_bredr = NULL;
	}
	if (ut->h_pcapng_le) {
		lell_pcapng_close(ut->h_pcapng_le);
		ut->h_pcapng_le = NULL;
	}
}

ubertooth_t* ubertooth_init()
{
	ubertooth_t* ut = (ubertooth_t*)malloc(sizeof(ubertooth_t));
	if(ut == NULL) {
		fprintf(stderr, "Unable to allocate memory\n");
		return NULL;
	}

	ut->packets = ringbuffer_init();
	if(ut->packets == NULL)
		fprintf(stderr, "Unable to initialize ringbuffer\n");

	ut->devh = NULL;
	ut->rx_xfer = NULL;
	ut->empty_usb_buf = NULL;
	ut->full_usb_buf = NULL;
	ut->usb_really_full = 0;
	ut->stop_ubertooth = 0;
	ut->abs_start_ns = 0;
	ut->start_clk100ns = 0;
	ut->last_clk100ns = 0;
	ut->clk100ns_upper = 0;

#ifdef ENABLE_PCAP
	ut->h_pcap_bredr = NULL;
	ut->h_pcap_le = NULL;
#endif

	ut->h_pcapng_bredr = NULL;
	ut->h_pcapng_le = NULL;

	return ut;
}

int ubertooth_connect(ubertooth_t* ut, int ubertooth_device)
{
	int r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "libusb_init failed (got 1.0?)\n");
		return -1;
	}

	ut->devh = find_ubertooth_device(ubertooth_device);
	if (ut->devh == NULL) {
		fprintf(stderr, "could not open Ubertooth device\n");
		ubertooth_stop(ut);
		return -1;
	}

	r = libusb_claim_interface(ut->devh, 0);
	if (r < 0) {
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		ubertooth_stop(ut);
		return -1;
	}

	return 1;
}

ubertooth_t* ubertooth_start(int ubertooth_device)
{
	ubertooth_t* ut = ubertooth_init();

	int r = ubertooth_connect(ut, ubertooth_device);
	if (r < 0)
		return NULL;

	return ut;
}

int ubertooth_check_api(ubertooth_t *ut) {
	int r;

	r = cmd_api_version(ut->devh);
	if (r < 0) {
		fprintf(stderr, "Ubertooth running very old firmware found.\n");
		fprintf(stderr, "Please upgrade to latest released firmware.\n");
		ubertooth_stop(ut);
		return -1;
	}
	else if (r < UBERTOOTH_API_VERSION) {
		fprintf(stderr, "Ubertooth API version %d found, libubertooth requires %d.\n",
				r, UBERTOOTH_API_VERSION);
		fprintf(stderr, "Please upgrade to latest released firmware.\n");
		ubertooth_stop(ut);
		return -1;
	}

	return 0;
}
