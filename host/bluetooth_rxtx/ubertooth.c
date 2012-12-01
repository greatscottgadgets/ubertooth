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

#include <bluetooth_packet.h>
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
int max_ac_errors = 1;
uint32_t systime;
int clk_offset = -1;
u8 usb_retry = 1;
u8 stop_ubertooth = 0;
u8 live = 0;
u8 hopping = 0;
u8 following = 0;
struct libusb_device_handle *devh_for_commands;
time_t end_time;
pnet_list_item* pnet_list_head;

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
			for(i = 0 ; i < ubertooths ; ++i) {
				libusb_get_device_descriptor(usb_list[ubertooth_devs[i]], &desc);
				ret = libusb_open(usb_list[ubertooth_devs[i]], &devh);
				if (ret) {
					fprintf(stderr, "  Device %d: ", i);
					show_libusb_error(ret);
				}
				else {
					fprintf(stderr, "  Device %d: serial no: ", i);
					cmd_get_serial(devh);
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

	while (usb_retry) {
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
	int i;
	int xfer_blocks;
	int num_xfers;
	usb_pkt_rx* rx;
	uint8_t bank = 0;
	uint8_t rx_buf1[BUFFER_SIZE];
	uint8_t rx_buf2[BUFFER_SIZE];

	/* KLUDGE: allow callback to call command */
	devh_for_commands = devh;

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
			r = libusb_handle_events(NULL);
			if (r < 0) {
				fprintf(stderr, "libusb_handle_events: %d\n", r);
				return -1;
			}
		}
		/*
		fprintf(stderr, "transfer completed\n");
		*/

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			rx = (usb_pkt_rx *)(full_buf + PKT_LEN * i);
			if(rx->pkt_type != KEEP_ALIVE)
				(*cb)(cb_args, rx, bank);
			bank = (bank + 1) % NUM_BANKS;
			if(stop_ubertooth) {
				really_full = 0;
				usb_retry = 0;
				r = libusb_handle_events(NULL);
				if (r < 0) {
					fprintf(stderr, "libusb_handle_events: %d\n", r);
					return -1;
				}
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

	/* unused parameter */ num_blocks = num_blocks;

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

void try_hop(bt_packet *pkt, bt_piconet *pn)
{
	uint8_t filter_uap = pn->UAP;

	/* Decode packet - fixing clock drift in the process */
	decode(pkt, pn);

	if (pn->hop_reversal_inited) {
		//pn->winnowed = 0;
		pn->pattern_indices[pn->packets_observed] = pkt->clkn - pn->first_pkt_time;
		pn->pattern_channels[pn->packets_observed] = pkt->channel;
		pn->packets_observed++;
		pn->total_packets_observed++;
		winnow(pn);
		if (pn->have_clk27) {
			printf("got CLK1-27\n");
			printf("clock offset = %d.\n", pn->clk_offset);
			clk_offset = pn->clk_offset;
		}
	} else {
		if (pn->have_clk6) {
			bt_uap_from_header(pkt, pn);
			if (pn->have_clk27) {
				printf("got CLK1-27\n");
				printf("clock offset = %d.\n", pn->clk_offset);
				clk_offset = pn->clk_offset;
			}
		} else {
			if (bt_uap_from_header(pkt, pn)) {
				if (filter_uap == pn->UAP) {
					printf("got CLK1-6\n");
					init_hop_reversal(0, pn);
					winnow(pn);
				} else {
					printf("failed to confirm UAP\n");
				}
			}
		}
	}

	if(!pn->have_UAP) {
		pn->have_UAP = 1;
		pn->UAP = filter_uap;
	}
}

/* Sniff for LAPs. If a piconet is provided, use the given LAP to
 * search for UAP.
 */
static void cb_lap(void* args, usb_pkt_rx *rx, int bank)
{
	bt_piconet* pn = (bt_piconet *)args;
	char syms[BANK_LEN * NUM_BANKS];
	int i;
	bt_packet pkt;
	char *channel_rssi_history;
	int8_t signal_level;
	int8_t noise_level;
	int8_t snr;
	int offset;
	uint32_t clkn;
	uint32_t lap;

	/* Sanity check */
	if (rx->channel > (NUM_CHANNELS-1))
		return;

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
	if (pn && pn->have_LAP)
		lap = pn->LAP;
	else
		lap = LAP_ANY;
	offset =  bt_find_ac(syms, BANK_LEN, lap, max_ac_errors, &pkt);
	if (offset < 0)
		return;

	/* Copy out remaining banks of symbols for full analysis. */
	for (i = 1; i < NUM_BANKS; i++)
		memcpy(syms + i * BANK_LEN,
		       symbols[(i + 1 + bank) % NUM_BANKS],
		       BANK_LEN);

	/* Once offset is known for a valid packet, copy in symbols
	 * and other rx data. CLKN here is the 312.5us CLK27-0. The
	 * btbb library can shift it be CLK1 if needed. */
	clkn = (rx->clkn_high << 20) + (le32toh(rx->clk100ns) + offset) / 3125;
	bt_packet_set_data(&pkt, syms + offset, NUM_BANKS * BANK_LEN - offset,
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
	       (int)systime, pkt.channel, pkt.LAP, pkt.ac_errors,
	       rx->clk100ns, pkt.clkn, signal_level, noise_level, snr);
	
	/* If piconet structure is given, a LAP is given, and packet
	 * header is readable, do further analysis. If UAP has not yet
	 * been determined, attempt to calculate it from headers. Once
	 * UAP is known, try to determine clk6 and clk27. Once clocks
	 * are known, follow the piconet. */
	if (pn && pn->have_LAP && (header_present(&pkt))) {

		/* If following is set, decode packets. */
		if (following) {
			pkt.UAP = pn->UAP;
			pkt.have_clk6 = 1;
			pkt.have_clk27 = 1;
			
			if(decode(&pkt, pn))
				btbb_print_packet(&pkt);
			else
				printf("Failed to decode packet\n");
		}

		/* If hopping is set, UAP is known. Try to determine clk6. */
		else if (hopping) {
			try_hop(&pkt, pn);
			if (pn->have_clk6 && pn->have_clk27) {
				following = 1;
				if (live)
					cmd_start_hopping(devh_for_commands, pn->clk_offset);
			}
		}
		
		/* Otherwise, try to determine UAP. */
		else if (bt_uap_from_header(&pkt, pn))
			hopping = 1;
	}
}

/* sniff one target LAP until the UAP is determined */
void rx_uap(struct libusb_device_handle* devh, bt_piconet* pn)
{
	live = 1;
	stream_rx_usb(devh, XFER_LEN, 0, cb_lap, pn);
	live = 0;
}

/* sniff one target LAP until the UAP is determined */
void rx_uap_file(FILE* fp, bt_piconet* pn)
{
	stream_rx_file(fp, 0, cb_lap, pn);
}

#ifdef WC4
/*
 * Sniff Bluetooth Low Energy packets.  So far this is just a proof of concept
 * that only captures advertising packets.
 */
void cb_btle(void* args, usb_pkt_rx *rx, int bank)
{
	int i;
	u32 access_address = 0;

	static u32 prev_ts = 0;

	/* unused parameter */ args = args;
	/* unused parameter */ bank = bank;

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
	/* unused parameter */ args = args;

	unpack_symbols(rx->data, symbols[bank]);
	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", rx->clk100ns);
	if (dumpfile == NULL) {
		if (fwrite(symbols[bank], sizeof(u8), BANK_LEN, stdout) != 1) {;}
    } else {
		if (fwrite(symbols[bank], sizeof(u8), BANK_LEN, dumpfile) != 1) {;}
	}
}

static void cb_dump_full(void* args, usb_pkt_rx *rx, int bank)
{
	uint8_t *buf = (uint8_t*)rx;

	/* unused parameter */ args = args; bank = bank;

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

/* WC4: Integrate cb_scan into common callback. This mode looks for
 * and stores UAPs for seen LAPs. */
#ifdef WC4
/* Sniff for LAPs. and attempt to determine matching UAPs */
void cb_scan(void* args, usb_pkt_rx *rx, int bank)
{
	bt_piconet* pn = (bt_piconet *)args;
	char syms[BANK_LEN * NUM_BANKS];
	int i;
	access_code ac;
	bt_packet pkt;
	char *channel_rssi_history;
	uint32_t clk0, clk1;
	pnet_list_item* pnet_holder;

	/* Sanity check */
	if (rx->channel > (NUM_CHANNELS-1))
		return;

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

	/* Copy all banks for analysis */
	for (i = 0; i < NUM_BANKS; i++)
		memcpy(syms + i * BANK_LEN,
		       symbols[(i + 1 + bank) % NUM_BANKS],
		       BANK_LEN);
			
	if(!sniff_ac(syms, BANK_LEN, &ac))
		return;

	if (ac.error_count <= max_ac_errors) {
		/* Native (Ubertooth) clock with period 312.5 uS. */
		clk0 = (rx->clkn_high << 20)
			+ (le32toh(rx->clk100ns) + ac.offset * 10) / 3125;
		/* Bottom clkn bit not needed, clk1 period is 625 uS. */
		clk1 = clk0 / 2;

		//printf("systime=%u ch=%2d LAP=%06x err=%u clk100ns=%u clk1=%u\n",
		//       (int)systime, rx->channel, ac.LAP, ac.error_count,
		//       rx->clk100ns, clk1);

		pnet_holder = pnet_list_head;
		while(pnet_holder != NULL) {
			if(pnet_holder->pnet->LAP == ac.LAP) {
				//printf("Piconet found\n");
				break;
			}
			pnet_holder = pnet_holder->next;
		}
		if (pnet_holder == NULL) {
			//printf("Piconet not found\n");
			pnet_holder = (pnet_list_item*) malloc(sizeof(pnet_list_item));
			pn = (bt_piconet*) malloc(sizeof(bt_piconet));
			init_piconet(pn);
			pn->LAP = ac.LAP;
			pnet_holder->pnet = pn;
			pnet_holder->next = pnet_list_head;
			pnet_list_head = pnet_holder;
		} else
			pn = pnet_holder->pnet;

		init_packet(&pkt, &syms[ac.offset],
			    BANK_LEN * NUM_BANKS - ac.offset);
		pkt.LAP = ac.LAP;
		pkt.clkn = clk1;
		pkt.channel = rx->channel;
		if (!pn->have_UAP && header_present(&pkt)) {
			bt_uap_from_header(&pkt, pn);
		}
		pn->afh_map[rx->channel/8] |= 0x1 << (rx->channel % 8);
	}
	
	if (time(NULL) >= end_time)
		stop_ubertooth = 1;
}

pnet_list_item* ubertooth_scan(struct libusb_device_handle* devh, int timeout)
{
	end_time = time(NULL) + timeout;
	stream_rx_usb(devh, XFER_LEN, 0, cb_scan, NULL);
	return pnet_list_head;
}
#endif // WC4

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
