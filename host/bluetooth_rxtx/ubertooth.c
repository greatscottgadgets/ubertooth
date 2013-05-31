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
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include <pcap.h>

#include <bluetooth_le_packet.h>

#include "ubertooth.h"
#include "ubertooth_control.h"

/* this stuff should probably be in a struct managed by the calling program */
usb_pkt_rx packets[NUM_BANKS];
char symbols[NUM_BANKS][BANK_LEN];
u8 *empty_buf = NULL;
u8 *full_buf = NULL;
u8 really_full = 0;
struct libusb_transfer *rx_xfer = NULL;
char Quiet = false;
FILE *infile = NULL;
FILE *dumpfile = NULL;
pcap_t *pcap_dumpfile = NULL;
pcap_dumper_t *dumper = NULL;
int max_ac_errors = 2;
uint32_t systime;
u8 usb_retry = 1;
u8 stop_ubertooth = 0;
btbb_piconet *follow_pn = NULL; // currently following this piconet

void stop_transfers(int sig) {
	sig = sig; // Unused parameter
	stop_ubertooth = 1;
}

void set_timeout(int seconds) {
	/* Upon SIGALRM, call stop_transfers() */
	if (signal(SIGALRM, stop_transfers) == SIG_ERR) {
	  perror("Unable to catch SIGALRM");
	  exit(1);
	}
	alarm(seconds);
}

// CACE PPI headers
typedef struct ppi_packetheader {
	uint8_t pph_version;
	uint8_t pph_flags;
	uint16_t pph_len;
	uint32_t pph_dlt;
} __attribute__((packed)) ppi_packet_header_t;

typedef struct ppi_fieldheader {
	u_int16_t pfh_type;       /* Type */
	u_int16_t pfh_datalen;    /* Length of data */
} ppi_fieldheader_t;

typedef struct ppi_btle {
	uint8_t btle_version; // 0 for now
	uint16_t btle_channel;
	uint8_t btle_clkn_high;
	uint32_t btle_clk100ns;
	int8_t rssi_max;
	int8_t rssi_min;
	int8_t rssi_avg;
	uint8_t rssi_count;
} __attribute__((packed)) ppi_btle_t;



#define PPI_BTLE		30006

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
		}
		else
			libusb_open(usb_list[ubertooth_devs[ubertooth_device]], &devh);
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

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		rx_xfer_status(xfer->status);
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

	while (usb_retry) {
		r = libusb_submit_transfer(rx_xfer);
		if (r < 0)
			fprintf(stderr, "rx_xfer submission from callback: %d\n", r);
		else
			break;
	}
}

int handle_events_wrapper() {
	int r = LIBUSB_ERROR_INTERRUPTED;
	while (r == LIBUSB_ERROR_INTERRUPTED) {
		r = libusb_handle_events(NULL);
		if (r < 0) {
			if (r != LIBUSB_ERROR_INTERRUPTED) {
				fprintf(stderr, "libusb_handle_events: %s\n", libusb_error_name(r));
				return -1;
			}
		} else
			return r;
	}
	return 0;
}

int stream_rx_usb(struct libusb_device_handle* devh, int xfer_size,
		uint16_t num_blocks, rx_callback cb, void* cb_args)
{
	int r;
	int i;
	int xfer_blocks;
	int num_xfers;
	usb_pkt_rx* rx;
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

	/*
	fprintf(stderr, "rx %d blocks of 64 bytes in %d byte transfers\n",
		num_blocks, xfer_size);
	*/

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
			handle_events_wrapper();
		}

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			rx = (usb_pkt_rx *)(full_buf + PKT_LEN * i);
			if(rx->pkt_type != KEEP_ALIVE) 
				(*cb)(cb_args, rx, bank);
			bank = (bank + 1) % NUM_BANKS;
			if(stop_ubertooth) {
				printf("stop_ubertooth is set 2\n");
				stop_ubertooth = 0;
				really_full = 0;
				usb_retry = 0;
				handle_events_wrapper();
				usb_retry = 1;
				return 1;
			}
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
	size_t nitems;

	UNUSED(num_blocks);

        /*
	fprintf(stderr, "reading %d blocks of 64 bytes from file\n", num_blocks);
	*/

	while(1) {
		uint32_t systime_be;
		nitems = fread(&systime_be, sizeof(systime_be), 1, fp);
		if (nitems != 1)
			return 0;
		systime = (time_t)be32toh(systime_be);

		nitems = fread(buf, sizeof(buf[0]), PKT_LEN, fp);
		if (nitems != PKT_LEN)
			return 0;
		(*cb)(cb_args, (usb_pkt_rx *)buf, bank);
		bank = (bank + 1) % NUM_BANKS;
	}
}

static void unpack_symbols(uint8_t* buf, char* unpacked)
{
	int i, j;

	for (i = 0; i < SYM_LEN; i++) {
		/* output one byte for each received symbol (0x00 or 0x01) */
		for (j = 0; j < 8; j++) {
			unpacked[i * 8 + j] = (buf[i] & 0x80) >> 7;
			buf[i] <<= 1;
		}
	}
}

#define NUM_CHANNELS 79
#define RSSI_HISTORY_LEN NUM_BANKS
#define RSSI_BASE (-54)       /* CC2400 constant ... do not change */

/* Ignore packets with a SNR lower than this in order to reduce
 * processor load.  TODO: this should be a command line parameter. */

static char rssi_history[NUM_CHANNELS][RSSI_HISTORY_LEN] = {{INT8_MIN}};

/* Sniff for LAPs. If a piconet is provided, use the given LAP to
 * search for UAP.
 */
static void cb_rx(void* args, usb_pkt_rx *rx, int bank)
{
	btbb_packet *pkt = NULL;
	btbb_piconet *pn = (btbb_piconet *)args;
	char syms[BANK_LEN * NUM_BANKS];
	int i;
	char *channel_rssi_history;
	int8_t signal_level;
	int8_t noise_level;
	int8_t snr;
	int offset;
	uint32_t clkn;
	uint32_t lap;

	/* Sanity check */
	if (rx->channel > (NUM_CHANNELS-1))
		goto out;

	/* Copy packet (for dump) */
	memcpy(&packets[bank], rx, sizeof(usb_pkt_rx));

	unpack_symbols(rx->data, symbols[bank]);

	/* Do analysis based on oldest packet */
	rx = &packets[ (bank+1) % NUM_BANKS ];

	/* Shift rssi max history and append current max */
	channel_rssi_history = rssi_history[rx->channel];
	memmove(channel_rssi_history,
		channel_rssi_history+1,
		RSSI_HISTORY_LEN-1);
	channel_rssi_history[RSSI_HISTORY_LEN-1] = rx->rssi_max;

	/* Signal starts in oldest bank, but may cross into second
	 * oldest bank.  Take the max or the 2 maxs. */
/*
	signal_level = MAX(channel_rssi_history[0],
			   channel_rssi_history[1]) + RSSI_BASE;
*/

	/* Alternatively, use all banks in history. */
	signal_level = channel_rssi_history[0];
	for (i = 1; i < RSSI_HISTORY_LEN; i++)
		signal_level = MAX(signal_level, channel_rssi_history[i]);
	signal_level += RSSI_BASE;

	/* Noise is an IIR of averages */
	noise_level = rx->rssi_avg + RSSI_BASE;
	snr = signal_level - noise_level;

	/* WC4: use vm circbuf if target allows. This gets rid of this
	 * wrapped copy step. */

	/* Copy 2 oldest banks of symbols for analysis. Packet may
	 * cross a bank boundary. */
	for (i = 0; i < 2; i++)
		memcpy(syms + i * BANK_LEN,
		       symbols[(i + 1 + bank) % NUM_BANKS],
		       BANK_LEN);
	
	/* Look for packets with specified LAP, if given. Otherwise
	 * search for any packet. */
	if (pn && btbb_piconet_get_flag(pn, BTBB_LAP_VALID))
		lap = btbb_piconet_get_lap(pn);
	else
		lap = LAP_ANY;

	/* Pass packet-pointer-pointer so that
	 * packet can be created in libbtbb. */
	offset = btbb_find_ac(syms, BANK_LEN, lap, max_ac_errors, &pkt);
	if (offset < 0)
		goto out;

	/* Copy out remaining banks of symbols for full analysis. */
	for (i = 1; i < NUM_BANKS; i++)
		memcpy(syms + i * BANK_LEN,
		       symbols[(i + 1 + bank) % NUM_BANKS],
		       BANK_LEN);

	/* Once offset is known for a valid packet, copy in symbols
	 * and other rx data. CLKN here is the 312.5us CLK27-0. The
	 * btbb library can shift it be CLK1 if needed. */
	clkn = (rx->clkn_high << 20) + (le32toh(rx->clk100ns) + offset + 1562) / 3125;
	btbb_packet_set_data(pkt, syms + offset, NUM_BANKS * BANK_LEN - offset,
			   rx->channel, clkn);

	/* If dumpfile is specified, write out all banks to the
	 * file. There could be duplicate data in the dump if more
	 * than one LAP is found within the span of NUM_BANKS. */
	if (dumpfile) {
		for(i = 0; i < NUM_BANKS; i++) {
			uint32_t systime_be = htobe32(systime);
			if (fwrite(&systime_be, 
				   sizeof(systime_be), 1,
				   dumpfile)
			    != 1) {;}
			if (fwrite(&packets[(i + 1 + bank) % NUM_BANKS],
				   sizeof(usb_pkt_rx), 1, dumpfile)
			    != 1) {;}
		}
	}

	/* When reading from file, caller will read
	 * systime before calling this routine, so do
	 * not overwrite. Otherwise, get current time. */
	if ( infile == NULL )
		systime = time(NULL);
	
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

	i = btbb_process_packet(pkt, pn);
	if(i < 0) {
		follow_pn = pn;
		stop_ubertooth = 1;
	}

out:
	if (pkt)
		btbb_packet_unref(pkt);
}

/* Receive and process packets. For now, returning from
 * stream_rx_usb() means that UAP and clocks have been found, and that
 * hopping should be started. A more flexible framework would be
 * nice. */
void rx_live(struct libusb_device_handle* devh, btbb_piconet* pn, int timeout)
{
	int r = btbb_init(max_ac_errors);
	if (r < 0)
		return;

	if (timeout)
		set_timeout(timeout);

	if (follow_pn)
		cmd_set_clock(devh, 0);
	else {
		stream_rx_usb(devh, XFER_LEN, 0, cb_rx, pn);
		/* Allow pending transfers to finish */
		sleep(1);
	}

	/* Used when follow_pn is preset OR set by stream_rx_usb above
	 * i.e. This cannot be rolled in to the above if...else
	 */
	if (follow_pn) {
		cmd_start_hopping(devh,
				  btbb_piconet_get_clk_offset(follow_pn));
		stream_rx_usb(devh, XFER_LEN, 0, cb_rx, follow_pn);
	}
}

/* sniff one target LAP until the UAP is determined */
void rx_file(FILE* fp, btbb_piconet* pn)
{
	int r = btbb_init(max_ac_errors);
	if (r < 0)
		return;
	stream_rx_file(fp, 0, cb_rx, pn);
}

#ifdef WC4
/* Dump packet to PCAP file */
static void log_packet(usb_pkt_rx *rx) {
	le_packet_t p;
	decode_le(rx->data, rx->channel + 2402, rx->clk100ns, &p);

	unsigned packet_length = 4 + 2 + p.length + 3;

	unsigned ppi_length = sizeof(ppi_fieldheader_t) + sizeof(ppi_btle_t);
	printf("size %u\n", ppi_length);

	void *logblob = malloc(sizeof(ppi_packet_header_t) + ppi_length + packet_length);
	ppi_packet_header_t *ppih = (ppi_packet_header_t *)logblob;
	ppih->pph_version = 0;
	ppih->pph_flags = 0;
	ppih->pph_len = htole16(sizeof(ppi_packet_header_t) + ppi_length);
	ppih->pph_dlt = htole32(DLT_USER0); //htole32(DLT_BTLE);

	// add PPI field
	ppi_fieldheader_t *ppifh = logblob + sizeof(ppi_packet_header_t);
	ppifh->pfh_type = htole16(PPI_BTLE);
	ppifh->pfh_datalen = htole16(sizeof(ppi_btle_t));

	ppi_btle_t *ppib = (void *)ppifh + sizeof(ppi_fieldheader_t);
	ppib->btle_version = 0;
	ppib->btle_channel = htole16(rx->channel + 2402);
	ppib->btle_clkn_high = rx->clkn_high;
	ppib->btle_clk100ns = htole32(rx->clk100ns);
	ppib->rssi_max = rx->rssi_max;
	ppib->rssi_min = rx->rssi_min;
	ppib->rssi_avg = rx->rssi_avg;
	ppib->rssi_count = rx->rssi_count;

	void *packet_data_out = (void *)ppib + sizeof(ppi_btle_t);

	// copy the data
	memcpy(packet_data_out, rx->data, packet_length);

	struct pcap_pkthdr wh;
	struct timeval ts;
	gettimeofday(&ts, NULL);

	wh.ts = ts;
	wh.caplen = wh.len = packet_length + sizeof(ppi_packet_header_t) + ppi_length;

	pcap_dump((unsigned char *)dumper, &wh, logblob);
	pcap_dump_flush(dumper);

	/* FIXME: don't force a flush
	 * Instead, write a signal handler to flush and close */

	free(logblob);
}

/*
 * Sniff Bluetooth Low Energy packets.  So far this is just a proof of concept
 * that only captures advertising packets.
 */
void cb_btle(void* args, usb_pkt_rx *rx, int bank)
{
	int i;
	u32 access_address = 0;

	static u32 prev_ts = 0;

	UNUSED(args);
	UNUSED(bank);

	/* Sanity check */
	if (rx->channel > (NUM_CHANNELS-1))
		return;

	if (infile == NULL)
		systime = time(NULL);

	/* Dump to sumpfile if specified */
	if (dumpfile) {
		uint32_t systime_be = htobe32(systime);
		if (fwrite(&systime_be, sizeof(systime_be), 1, dumpfile) != 1) {;}
		if (fwrite(rx, sizeof(usb_pkt_rx), 1, dumpfile) != 1) {;}
	}

	/* Dump to PCAP if specified */
	if (pcap_dumpfile) {
		log_packet(rx);
	}

	for (i = 0; i < 4; ++i)
		access_address |= rx->data[i] << (i * 8);

	u32 ts_diff = rx->clk100ns - prev_ts;
	prev_ts = rx->clk100ns;
	printf("systime=%u freq=%d addr=%08x delta_t=%.03f ms\n",
		   systime, rx->channel + 2402, access_address, ts_diff / 10000.0);

	int len = (rx->data[5] & 0x3f) + 6 + 3;
	if (len > 50) len = 50;

	for (i = 4; i < len; ++i)
		printf("%02x ", rx->data[i]);
	printf("\n");

	le_packet_t p;
	decode_le(rx->data, rx->channel + 2402, rx->clk100ns, &p);
	le_print(&p);
	printf("\n");

	fflush(stdout);
}

void rx_btle_file(FILE* fp)
{
	stream_rx_file(fp, 0, cb_btle, NULL);
}
#endif //WC4

static void cb_dump_bitstream(void* args, usb_pkt_rx *rx, int bank)
{
	int i;
	char nl = '\n';

	UNUSED(args);

	unpack_symbols(rx->data, symbols[bank]);

	// convert to ascii
	for (i = 0; i < BANK_LEN; ++i)
		symbols[bank][i] += 0x30;

	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", rx->clk100ns);
	if (dumpfile == NULL) {
		if (fwrite(symbols[bank], sizeof(u8), BANK_LEN, stdout) != 1) {;}
		fwrite(&nl, sizeof(u8), 1, stdout);
    } else {
		if (fwrite(symbols[bank], sizeof(u8), BANK_LEN, dumpfile) != 1) {;}
		fwrite(&nl, sizeof(u8), 1, dumpfile);
	}
}

static void cb_dump_full(void* args, usb_pkt_rx *rx, int bank)
{
	uint8_t *buf = (uint8_t*)rx;

	UNUSED(args);
	UNUSED(bank);

	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", rx->clk100ns);
	uint32_t time_be = htobe32((uint32_t)time(NULL));
	if (dumpfile == NULL) {
		if (fwrite(&time_be, 1, sizeof(time_be), stdout) != 1) {;}
		if (fwrite(buf, sizeof(u8), PKT_LEN, stdout) != 1) {;}
	} else {
		if (fwrite(&time_be, 1, sizeof(time_be), dumpfile) != 1) {;}
		if (fwrite(buf, sizeof(u8), PKT_LEN, dumpfile) != 1) {;}
	}
}

/* dump received symbols to stdout */
void rx_dump(struct libusb_device_handle* devh, int bitstream)
{
	if (bitstream)
		stream_rx_usb(devh, XFER_LEN, 0, cb_dump_bitstream, NULL);
	else
		stream_rx_usb(devh, XFER_LEN, 0, cb_dump_full, NULL);
}

int specan(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks,
		u16 low_freq, u16 high_freq)
{
	return do_specan(devh, xfer_size, num_blocks, low_freq, high_freq, false);
}

int do_specan(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks,
		u16 low_freq, u16 high_freq, char gnuplot)
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

	if(!Quiet)
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
		if(!Quiet)
			fprintf(stderr, "transferred %d bytes\n", transferred);

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			time = buffer[4 + PKT_LEN * i]
					| (buffer[5 + PKT_LEN * i] << 8)
					| (buffer[6 + PKT_LEN * i] << 16)
					| (buffer[7 + PKT_LEN * i] << 24);
			if(!Quiet)
				fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			for (j = PKT_LEN * i + SYM_OFFSET; j < PKT_LEN * i + 62; j += 3) {
				frequency = (buffer[j] << 8) | buffer[j + 1];
				if (buffer[j + 2] > 150) { /* FIXME  */
					if(gnuplot == GNUPLOT_NORMAL)
						printf("%d %d\n", frequency, buffer[j + 2]);
					else if(gnuplot == GNUPLOT_3D)
						printf("%f %d %d\n", ((double)time)/10000000, frequency, buffer[j + 2]);
					else
						printf("%f, %d, %d\n", ((double)time)/10000000, frequency, buffer[j + 2]);
				}
				if (frequency == high_freq && !gnuplot)
					printf("\n");
			}
		}
		fflush(stderr);
	}
	return 0;
}

void ubertooth_stop(struct libusb_device_handle *devh)
{
	/* FIXME make sure xfers are not active */
	libusb_free_transfer(rx_xfer);
	if (devh != NULL)
		libusb_release_interface(devh, 0);
	libusb_close(devh);
	libusb_exit(NULL);
}

struct libusb_device_handle* ubertooth_start(int ubertooth_device)
{
	int r;
	struct libusb_device_handle *devh = NULL;

	r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "libusb_init failed (got 1.0?)\n");
		return NULL;
	}

	devh = find_ubertooth_device(ubertooth_device);
	if (devh == NULL) {
		fprintf(stderr, "could not open Ubertooth device\n");
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
