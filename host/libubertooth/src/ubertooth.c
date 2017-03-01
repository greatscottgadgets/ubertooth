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

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ubertooth.h"
#include "ubertooth_callback.h"
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

int do_exit = 1;
pthread_t poll_thread;

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

static void cleanup_exit(int sig __attribute__((unused)))
{
	if (cleanup_devh)
		ubertooth_stop(cleanup_devh);
	exit(0);
}

void register_cleanup_handler(ubertooth_t* ut, int do_exit) {
	cleanup_devh = ut;

	/* Clean up on ctrl-C. */
	if (do_exit) {
		signal(SIGINT, cleanup_exit);
		signal(SIGQUIT, cleanup_exit);
		signal(SIGTERM, cleanup_exit);
	} else {
		signal(SIGINT, cleanup);
		signal(SIGQUIT, cleanup);
		signal(SIGTERM, cleanup);
	}
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
			uint8_t serial[17], r;
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

	if(ut->stop_ubertooth)
		return;

	fifo_inc_write_ptr(ut->fifo);
	ut->rx_xfer->buffer = (uint8_t*)fifo_get_write_element(ut->fifo);

	r = libusb_submit_transfer(ut->rx_xfer);
	if (r < 0)
		fprintf(stderr, "Failed to submit USB transfer (%d)\n", r);
}

static void* poll_thread_main(void* arg __attribute__((unused)))
{
	int r = 0;

	while (!do_exit) {
		struct timeval tv = { 1, 0 };
		r = libusb_handle_events_timeout(NULL, &tv);
		if (r < 0) {
			do_exit = 1;
			break;
		}
		usleep(1);
	}

	return NULL;
}

int ubertooth_bulk_thread_start()
{
	do_exit = 0;

	return pthread_create(&poll_thread, NULL, poll_thread_main, NULL);
}

void ubertooth_bulk_thread_stop()
{
	do_exit = 1;

	pthread_join(poll_thread, NULL);
}

int ubertooth_bulk_init(ubertooth_t* ut)
{
	int r;

	ut->rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(ut->rx_xfer, ut->devh, DATA_IN, (uint8_t*)fifo_get_write_element(ut->fifo), PKT_LEN, cb_xfer, ut, TIMEOUT);

	r = libusb_submit_transfer(ut->rx_xfer);
	if (r < 0) {
		fprintf(stderr, "rx_xfer submission: %d\n", r);
		return -1;
	}
	return 0;
}

void ubertooth_bulk_wait(ubertooth_t* ut)
{
	while (fifo_empty(ut->fifo) && !ut->stop_ubertooth)
		usleep(1);
}

int ubertooth_bulk_receive(ubertooth_t* ut, rx_callback cb, void* cb_args)
{
	if (!fifo_empty(ut->fifo)) {
		(*cb)(ut, cb_args);
		if(ut->stop_ubertooth) {
			if(ut->rx_xfer)
				libusb_cancel_transfer(ut->rx_xfer);
			return 1;
		}
		fflush(stderr);
		return 0;
	} else {
		usleep(1);
		return -1;
	}
}

static int stream_rx_usb(ubertooth_t* ut, rx_callback cb, void* cb_args)
{
	// init USB transfer
	int r = ubertooth_bulk_init(ut);
	if (r < 0)
		return r;

	r = ubertooth_bulk_thread_start();
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
	}

	ubertooth_bulk_thread_stop();

	return 1;
}

/* file should be in full USB packet format (ubertooth-dump -f) */
int stream_rx_file(ubertooth_t* ut, FILE* fp, rx_callback cb, void* cb_args)
{
	uint8_t buf[PKT_LEN];
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
		fifo_push(ut->fifo, (usb_pkt_rx*)buf);
		(*cb)(ut, cb_args);
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
		ut->stop_ubertooth = 0;

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

	r = ubertooth_bulk_thread_start();
	if (r < 0)
		return;

	// tell ubertooth to send packets
	r = cmd_rx_syms(ut->devh);
	if (r < 0)
		return;

	// receive and process each packet
	while(!ut->stop_ubertooth) {
		ubertooth_bulk_receive(ut, cb_afh_r, pn);
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
	}

	ubertooth_bulk_thread_stop();
}

void rx_btle_file(FILE* fp)
{
	ubertooth_t* ut = ubertooth_init();
	if (ut == NULL)
		return;

	stream_rx_file(ut, fp, cb_btle, NULL);
}

void ubertooth_unpack_symbols(const uint8_t* buf, char* unpacked)
{
	int i, j;

	for (i = 0; i < SYM_LEN; i++) {
		/* output one byte for each received symbol (0x00 or 0x01) */
		for (j = 0; j < 8; j++) {
			unpacked[i * 8 + j] = ((buf[i] << j) & 0x80) >> 7;
		}
	}
}

static void cb_dump_bitstream(ubertooth_t* ut, void* args __attribute__((unused)))
{
	int i;
	char nl = '\n';

	usb_pkt_rx usb = fifo_pop(ut->fifo);
	usb_pkt_rx* rx = &usb;
	char bitstream[BANK_LEN];
	ubertooth_unpack_symbols((uint8_t*)rx->data, bitstream);

	// convert to ascii
	for (i = 0; i < BANK_LEN; ++i)
		bitstream[i] += 0x30;

	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n",
	        rx->clk100ns);
	if (dumpfile == NULL) {
		fwrite(bitstream, sizeof(uint8_t), BANK_LEN, stdout);
		fwrite(&nl, sizeof(uint8_t), 1, stdout);
	} else {
		fwrite(bitstream, sizeof(uint8_t), BANK_LEN, dumpfile);
		fwrite(&nl, sizeof(uint8_t), 1, dumpfile);
	}
}

static void cb_dump_full(ubertooth_t* ut, void* args __attribute__((unused)))
{
	usb_pkt_rx usb = fifo_pop(ut->fifo);
	usb_pkt_rx* rx = &usb;

	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", rx->clk100ns);
	uint32_t time_be = htobe32((uint32_t)time(NULL));
	if (dumpfile == NULL) {
		fwrite(&time_be, 1, sizeof(time_be), stdout);
		fwrite((uint8_t*)rx, sizeof(uint8_t), PKT_LEN, stdout);
	} else {
		fwrite(&time_be, 1, sizeof(time_be), dumpfile);
		fwrite((uint8_t*)rx, sizeof(uint8_t), PKT_LEN, dumpfile);
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

	if (ut->h_pcap_bredr) {
		btbb_pcap_close(ut->h_pcap_bredr);
		ut->h_pcap_bredr = NULL;
	}
	if (ut->h_pcap_le) {
		lell_pcap_close(ut->h_pcap_le);
		ut->h_pcap_le = NULL;
	}

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

	ut->fifo = fifo_init();
	if(ut->fifo == NULL)
		fprintf(stderr, "Unable to initialize ringbuffer\n");

	ut->devh = NULL;
	ut->rx_xfer = NULL;
	ut->stop_ubertooth = 0;
	ut->abs_start_ns = 0;
	ut->start_clk100ns = 0;
	ut->last_clk100ns = 0;
	ut->clk100ns_upper = 0;

	ut->h_pcap_bredr = NULL;
	ut->h_pcap_le = NULL;
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

int ubertooth_get_api(ubertooth_t *ut, uint16_t *version) {
	int result;
	libusb_device* dev;
	struct libusb_device_descriptor desc;
	dev = libusb_get_device(ut->devh);
	result = libusb_get_device_descriptor(dev, &desc);
	if (result < 0) {
		if (result == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(result);
		}
		return result;
	}
	*version = desc.bcdDevice;
	return 0;
}

int ubertooth_check_api(ubertooth_t *ut) {
	uint16_t version;
	int result;
	result = ubertooth_get_api(ut, &version);
	if (result < 0) {
		return result;
	}

	if (version < UBERTOOTH_API_VERSION) {
		fprintf(stderr, "Ubertooth API version %x.%02x found, libubertooth %s requires %x.%02x.\n",
				(version>>8)&0xFF, version&0xFF, VERSION,
				(UBERTOOTH_API_VERSION>>8)&0xFF, UBERTOOTH_API_VERSION&0xFF);
		fprintf(stderr, "Please upgrade to latest released firmware.\n");
		fprintf(stderr, "See: https://github.com/greatscottgadgets/ubertooth/wiki/Firmware\n");
		ubertooth_stop(ut);
		return -1;
	}
	else if (version > UBERTOOTH_API_VERSION) {
		fprintf(stderr, "Ubertooth API version %x.%02x found, newer than that supported by libubertooth (%x.%02x).\n",
				(version>>8)&0xFF, version&0xFF,
				(UBERTOOTH_API_VERSION>>8)&0xFF, UBERTOOTH_API_VERSION&0xFF);
		fprintf(stderr, "Things will still work, but you might want to update your host tools.\n");
	}
	return 0;
}
