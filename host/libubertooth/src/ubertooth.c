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
FILE *infile = NULL;
FILE *dumpfile = NULL;
int max_ac_errors = 2;

unsigned int packet_counter_max;

void print_version() {
	printf("libubertooth %s (%s), libbtbb %s (%s)\n", VERSION, RELEASE,
	       btbb_get_version(), btbb_get_release());
}

ubertooth_t* cleanup_devh = NULL;
static void cleanup(int sig __attribute__((unused)))
{
	if (cleanup_devh) {
		ubertooth_stop(cleanup_devh);
	}
	exit(0);
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
	while(1) {
		ubertooth_bulk_wait(ut);
		r = ubertooth_bulk_receive(ut, cb, cb_args);
		if (r == 1)
			return 1;
	}
}

/* file should be in full USB packet format (ubertooth-dump -f) */
static int stream_rx_file(FILE* fp, rx_callback cb, void* cb_args)
{
	uint8_t buf[BUFFER_SIZE];
	size_t nitems;

	ubertooth_t* ut = ubertooth_init();
	if (ut == NULL)
		return -1;

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

static int8_t cc2400_rssi_to_dbm( const int8_t rssi )
{
	/* models the cc2400 datasheet fig 22 for 1M as piece-wise linear */
	if (rssi < -48) {
		return -120;
	}
	else if (rssi <= -45) {
		return 6*(rssi+28);
	}
	else if (rssi <= 30) {
		return (int8_t) ((99*((int)rssi-62))/110);
	}
	else if (rssi <= 35) {
		return (int8_t) ((60*((int)rssi-35))/11);
	}
	else {
		return 0;
	}
}

#define RSSI_HISTORY_LEN NUM_BANKS

/* Ignore packets with a SNR lower than this in order to reduce
 * processor load.  TODO: this should be a command line parameter. */

static int8_t rssi_history[NUM_BREDR_CHANNELS][RSSI_HISTORY_LEN] = {{INT8_MIN}};

static void determine_signal_and_noise( usb_pkt_rx *rx, int8_t * sig, int8_t * noise )
{
	int8_t * channel_rssi_history = rssi_history[rx->channel];
	int8_t rssi;
	int i;

	/* Shift rssi max history and append current max */
	memmove(channel_rssi_history,
	        channel_rssi_history+1,
	        RSSI_HISTORY_LEN-1);
	channel_rssi_history[RSSI_HISTORY_LEN-1] = rx->rssi_max;

#if 0
	/* Signal starts in oldest bank, but may cross into second
	 * oldest bank.  Take the max or the 2 maxs. */
	rssi = MAX(channel_rssi_history[0], channel_rssi_history[1]);
#else
	/* Alternatively, use all banks in history. */
	rssi = channel_rssi_history[0];
	for (i = 1; i < RSSI_HISTORY_LEN; i++)
		rssi = MAX(rssi, channel_rssi_history[i]);
#endif
	*sig = cc2400_rssi_to_dbm( rssi );

	/* Noise is an IIR of averages */
	/* FIXME: currently bogus */
	*noise = cc2400_rssi_to_dbm( rx->rssi_avg );
}

static uint64_t now_ns( void )
{
/* As per Apple QA1398 */
#if defined( __APPLE__ )
	static mach_timebase_info_data_t sTimebaseInfo;
	uint64_t ts = mach_absolute_time( );
	if (sTimebaseInfo.denom == 0) {
		(void) mach_timebase_info(&sTimebaseInfo);
	}
	return (ts*sTimebaseInfo.numer/sTimebaseInfo.denom);
#else
	struct timespec ts = { 0, 0 };
	(void) clock_gettime( CLOCK_REALTIME, &ts );
	return (1000000000ull*(uint64_t) ts.tv_sec) + (uint64_t) ts.tv_nsec;
#endif
}

static void track_clk100ns( ubertooth_t* ut, const usb_pkt_rx* rx )
{
	/* track clk100ns */
	if (!ut->start_clk100ns) {
		ut->last_clk100ns = ut->start_clk100ns = rx->clk100ns;
		ut->abs_start_ns = now_ns( );
	}
	/* detect clk100ns roll-over */
	if (rx->clk100ns < ut->last_clk100ns) {
		ut->clk100ns_upper += 1;
	}
	ut->last_clk100ns = rx->clk100ns;
}

static uint64_t now_ns_from_clk100ns( ubertooth_t* ut, const usb_pkt_rx* rx )
{
	track_clk100ns( ut, rx );
	return ut->abs_start_ns +
	       100ull*(uint64_t)((rx->clk100ns - ut->start_clk100ns) & 0xffffffff) +
	       ((100ull*ut->clk100ns_upper)<<32);
}

/* Sniff for LAPs. If a piconet is provided, use the given LAP to
 * search for UAP.
 */
static void cb_br_rx(ubertooth_t* ut, void* args)
{
	btbb_packet* pkt = NULL;
	btbb_piconet* pn = (btbb_piconet *)args;
	int8_t signal_level;
	int8_t noise_level;
	int8_t snr;
	int offset;
	uint32_t clkn;
	uint32_t lap = LAP_ANY;
	uint8_t uap = UAP_ANY;

	/* Do analysis based on oldest packet */
	usb_pkt_rx* rx = ringbuffer_top_usb(ut->packets);

	/* Sanity check */
	if (rx->channel > (NUM_BREDR_CHANNELS-1))
		goto out;

	uint64_t nowns = now_ns_from_clk100ns( ut, rx );

	determine_signal_and_noise( rx, &signal_level, &noise_level );
	snr = signal_level - noise_level;

	/* Look for packets with specified LAP, if given. Otherwise
	 * search for any packet.  Also determine if UAP is known. */
	if (pn) {
		lap = btbb_piconet_get_flag(pn, BTBB_LAP_VALID) ? btbb_piconet_get_lap(pn) : LAP_ANY;
		uap = btbb_piconet_get_flag(pn, BTBB_UAP_VALID) ? btbb_piconet_get_uap(pn) : UAP_ANY;
	}

	/* Pass packet-pointer-pointer so that
	 * packet can be created in libbtbb. */
	offset = btbb_find_ac(ringbuffer_top_bt(ut->packets), BANK_LEN - 64, lap, max_ac_errors, &pkt);
	if (offset < 0)
		goto out;

	btbb_packet_set_modulation(pkt, BTBB_MOD_GFSK);
	btbb_packet_set_transport(pkt, BTBB_TRANSPORT_ANY);

	/* Once offset is known for a valid packet, copy in symbols
	 * and other rx data. CLKN here is the 312.5us CLK27-0. The
	 * btbb library can shift it be CLK1 if needed. */
	clkn = (rx->clkn_high << 20) + (le32toh(rx->clk100ns) + offset*10) / 3125;
	btbb_packet_set_data(pkt, ringbuffer_top_bt(ut->packets) + offset, NUM_BANKS * BANK_LEN - offset,
	                     rx->channel, clkn);

	/* When reading from file, caller will read
	 * systime before calling this routine, so do
	 * not overwrite. Otherwise, get current time. */
	if (infile == NULL)
		systime = time(NULL);

	/* If dumpfile is specified, write out all banks to the
	 * file. There could be duplicate data in the dump if more
	 * than one LAP is found within the span of NUM_BANKS. */
	if (dumpfile) {
		uint32_t systime_be = htobe32(systime);
		fwrite(&systime_be, sizeof(systime_be), 1, dumpfile);
		fwrite(ringbuffer_top_usb(ut->packets), sizeof(usb_pkt_rx), 1, dumpfile);
		fflush(dumpfile);
	}

	printf("systime=%u ch=%2d LAP=%06x err=%u clk100ns=%u clk1=%u s=%d n=%d snr=%d\n",
	       (int)systime,
	       btbb_packet_get_channel(pkt),
	       btbb_packet_get_lap(pkt),
	       btbb_packet_get_ac_errors(pkt),
	       rx->clk100ns,
	       btbb_packet_get_clkn(pkt),
	       signal_level,
	       noise_level,
	       snr);

	/* Dump to PCAP/PCAPNG if specified */
#ifdef ENABLE_PCAP
	if (ut->h_pcap_bredr) {
		btbb_pcap_append_packet(ut->h_pcap_bredr, nowns,
		                        signal_level, noise_level,
		                        lap, uap, pkt);
	}
#endif
	if (ut->h_pcapng_bredr) {
		btbb_pcapng_append_packet(ut->h_pcapng_bredr, nowns,
		                          signal_level, noise_level,
		                          lap, uap, pkt);
	}

	int r = btbb_process_packet(pkt, pn);
	if(r < 0) {
		ut->stop_ubertooth = 1;
	}

out:
	if (pkt)
		btbb_packet_unref(pkt);
}

void cb_afh_initial(ubertooth_t* ut, void* args)
{
	btbb_piconet* pn = (btbb_piconet*)args;
	btbb_packet* pkt = NULL;
	uint8_t channel;


	if( btbb_find_ac(ringbuffer_top_bt(ut->packets), BANK_LEN - 64, btbb_piconet_get_lap(pn), max_ac_errors, &pkt) < 0 )
		goto out;

	/* detect AFH map
	 * set current channel as used channel and send updated AFH
	 * map to ubertooth */
	channel = ringbuffer_top_usb(ut->packets)->channel;
	if(btbb_piconet_get_channel_seen(pn, channel)) {

		/* Don't allow single unused channels */
		if (!btbb_piconet_get_channel_seen(pn, channel+1) &&
		    btbb_piconet_get_channel_seen(pn, channel+2))
		{
			printf("activating additional channel %d\n", channel+1);
		    btbb_piconet_set_channel_seen(pn, channel+1);
		}
		if (!btbb_piconet_get_channel_seen(pn, channel-1) &&
		    btbb_piconet_get_channel_seen(pn, channel-2))
		{
			printf("activating additional channel %d\n", channel-1);
			btbb_piconet_set_channel_seen(pn, channel-1);
		}

		cmd_set_afh_map(ut->devh, btbb_piconet_get_afh_map(pn));
		btbb_print_afh_map(pn);
	}
	cmd_hop(ut->devh);

out:
	if (pkt)
		btbb_packet_unref(pkt);
}

void cb_afh_monitor(ubertooth_t* ut, void* args)
{
	btbb_piconet* pn = (btbb_piconet*)args;
	btbb_packet* pkt = NULL;
	uint8_t channel;
	int i;

	static unsigned long last_seen[79] = {0};
	static unsigned long counter = 0;


	if( btbb_find_ac(ringbuffer_top_bt(ut->packets), BANK_LEN - 64, btbb_piconet_get_lap(pn), max_ac_errors, &pkt) < 0 )
		goto out;

	counter++;
	channel = ringbuffer_top_usb(ut->packets)->channel;
	last_seen[channel]=counter;

	if(btbb_piconet_get_channel_seen(pn, channel)) {
		printf("+ channel %2d is used now\n", channel);
		btbb_print_afh_map(pn);
	// } else {
	// 	printf("channel %d is already used\n", channel);
	}

	for(i=0; i<79; i++) {
		if((counter - last_seen[i] >= packet_counter_max)) {
			if(btbb_piconet_clear_channel_seen(pn, i)) {
				printf("- channel %2d is not used any more\n", i);
				btbb_print_afh_map(pn);
			}
		}
	}
	cmd_hop(ut->devh);

out:
	if (pkt)
		btbb_packet_unref(pkt);
}

void cb_afh_r(ubertooth_t* ut, void* args)
{
	btbb_piconet* pn = (btbb_piconet*)args;
	btbb_packet* pkt = NULL;
	uint8_t channel;
	int i;

	static unsigned long last_seen[79] = {0};
	static unsigned long counter = 0;

	if( btbb_find_ac(ringbuffer_top_bt(ut->packets), BANK_LEN - 64, btbb_piconet_get_lap(pn), max_ac_errors, &pkt) < 0 )
		goto out;


	counter++;
	channel = ringbuffer_top_usb(ut->packets)->channel;
	last_seen[channel]=counter;

	btbb_piconet_set_channel_seen(pn, channel);

	for(i=0; i<79; i++) {
		if((counter - last_seen[i] >= packet_counter_max)) {
			btbb_piconet_clear_channel_seen(pn, i);
		}
	}
	cmd_hop(ut->devh);

out:
	if (pkt)
		btbb_packet_unref(pkt);
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
	stream_rx_file(fp, cb_br_rx, pn);
}

/*
 * Sniff Bluetooth Low Energy packets.
 */
void cb_btle(ubertooth_t* ut, void* args)
{
	lell_packet* pkt;
	btle_options* opts = (btle_options*) args;
	int i;
	usb_pkt_rx* rx = ringbuffer_top_usb(ut->packets);
	// u32 access_address = 0; // Build warning

	static u32 prev_ts = 0;
	uint32_t refAA;
	int8_t sig, noise;

	// display LE promiscuous mode state changes
	if (rx->pkt_type == LE_PROMISC) {
		u8 state = rx->data[0];
		void *val = &rx->data[1];

		printf("--------------------\n");
		printf("LE Promisc - ");
		switch (state) {
			case 0:
				printf("Access Address: %08x\n", *(uint32_t *)val);
				break;
			case 1:
				printf("CRC Init: %06x\n", *(uint32_t *)val);
				break;
			case 2:
				printf("Hop interval: %g ms\n", *(uint16_t *)val * 1.25);
				break;
			case 3:
				printf("Hop increment: %u\n", *(uint8_t *)val);
				break;
			default:
				printf("Unknown %u\n", state);
				break;
		};
		printf("\n");

		return;
	}

	uint64_t nowns = now_ns_from_clk100ns( ut, rx );

	/* Sanity check */
	if (rx->channel > (NUM_BREDR_CHANNELS-1))
		return;

	if (infile == NULL)
		systime = time(NULL);

	/* Dump to sumpfile if specified */
	if (dumpfile) {
		uint32_t systime_be = htobe32(systime);
		fwrite(&systime_be, sizeof(systime_be), 1, dumpfile);
		fwrite(rx, sizeof(usb_pkt_rx), 1, dumpfile);
		fflush(dumpfile);
	}

	lell_allocate_and_decode(rx->data, rx->channel + 2402, rx->clk100ns, &pkt);

	/* do nothing further if filtered due to bad AA */
	if (opts &&
	    (opts->allowed_access_address_errors <
	     lell_get_access_address_offenses(pkt))) {
		lell_packet_unref(pkt);
		return;
	}

	/* Dump to PCAP/PCAPNG if specified */
	refAA = lell_packet_is_data(pkt) ? 0 : 0x8e89bed6;
	determine_signal_and_noise( rx, &sig, &noise );
#ifdef ENABLE_PCAP
	if (ut->h_pcap_le) {
		/* only one of these two will succeed, depending on
		 * whether PCAP was opened with DLT_PPI or not */
		lell_pcap_append_packet(ut->h_pcap_le, nowns,
					sig, noise,
					refAA, pkt);
		lell_pcap_append_ppi_packet(ut->h_pcap_le, nowns,
		                            rx->clkn_high,
		                            rx->rssi_min, rx->rssi_max,
		                            rx->rssi_avg, rx->rssi_count,
		                            pkt);
	}
#endif
	if (ut->h_pcapng_le) {
		lell_pcapng_append_packet(ut->h_pcapng_le, nowns,
		                          sig, noise,
		                          refAA, pkt);
	}

	// rollover
	u32 rx_ts = rx->clk100ns;
	if (rx_ts < prev_ts)
		rx_ts += 3276800000;
	u32 ts_diff = rx_ts - prev_ts;
	prev_ts = rx->clk100ns;
	printf("systime=%u freq=%d addr=%08x delta_t=%.03f ms rssi=%d\n",
	       systime, rx->channel + 2402, lell_get_access_address(pkt),
	       ts_diff / 10000.0, rx->rssi_min - 54);

	int len = (rx->data[5] & 0x3f) + 6 + 3;
	if (len > 50) len = 50;

	for (i = 4; i < len; ++i)
		printf("%02x ", rx->data[i]);
	printf("\n");

	lell_print(pkt);
	printf("\n");

	lell_packet_unref(pkt);

	fflush(stdout);
}
/*
 * Sniff E-GO packets
 */
void cb_ego(ubertooth_t* ut, void* args __attribute__((unused)))
{
	int i;
	static u32 prev_ts = 0;
	usb_pkt_rx* rx = ringbuffer_top_usb(ut->packets);

	u32 rx_time = rx->clk100ns;
	if (rx_time < prev_ts)
		rx_time += 3276800000; // rollover
	u32 ts_diff = rx_time - prev_ts;
	prev_ts = rx->clk100ns;
	printf("time=%u delta_t=%.06f ms freq=%d \n",
	       rx->clk100ns, ts_diff / 10000.0,
	       rx->channel + 2402);

	int len = 36; // FIXME

	for (i = 0; i < len; ++i)
		printf("%02x ", rx->data[i]);
	printf("\n\n");

	fflush(stdout);
}

void rx_btle_file(FILE* fp)
{
	stream_rx_file(fp, cb_btle, NULL);
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
