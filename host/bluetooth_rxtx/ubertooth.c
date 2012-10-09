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
u8 hopping = 0;
u8 usb_retry = 1;

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
			(*cb)(cb_args, (usb_pkt_rx *)(full_buf + PKT_LEN * i), bank);
			bank = (bank + 1) % NUM_BANKS;
			if((clk_offset != -1) && !hopping) {
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

/* Sniff for LAPs. If a piconet is provided, use the given LAP to
 * search for UAP.
 */
static void cb_lap(void* args, usb_pkt_rx *rx, int bank)
{
	piconet* pn = (piconet *)args;
	char syms[BANK_LEN * NUM_BANKS];
	int i;
	access_code ac;
	packet pkt;
	char *channel_rssi_history;
	int8_t signal_level;
	int8_t noise_level;
	int8_t snr;
	uint32_t clk0, clk1;

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

	/* Copy 2 banks for analysis */
	for (i = 0; i < 2; i++)
		memcpy(syms + i * BANK_LEN,
		       symbols[(i + 1 + bank) % NUM_BANKS],
		       BANK_LEN);

	/* No piconet given, sniff for any LAP */
	if (pn == NULL) {
		if(!sniff_ac(syms, BANK_LEN, &ac))
			return;
	}
	/* Find packets for specified LAP.  */
	else {
		if(!find_ac(syms, BANK_LEN, pn->LAP, &ac))
			return;
	}

	if ((ac.offset > -1) && (ac.error_count <= max_ac_errors)) {
		/* If dumpfile is specified, write out all banks to
		 * the file. There could be duplicate data in the dump
		 * if more than one LAP is found within the span of
		 * NUM_BANKS. If experiment mode is selected, extra
		 * info is written out. For now, it just prepends
		 * capture time. */
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

		/* Native (Ubertooth) clock with period 312.5 uS. */
		clk0 = (rx->clkn_high << 20)
			+ (le32toh(rx->clk100ns) + ac.offset * 10) / 3125;
		/* Bottom clkn bit not needed, clk1 period is 625 uS. */
		clk1 = clk0 / 2;

		/* When reading from file, caller will read
		 * systime before calling this routine, so do
		 * not overwrite. Otherwise, get current time. */
		if ( infile == NULL )
			systime = time(NULL);

		printf("systime=%u ch=%2d LAP=%06x err=%u clk100ns=%u clk1=%u s=%d n=%d snr=%d\n",
		       (int)systime, rx->channel, ac.LAP, ac.error_count,
		       rx->clk100ns, clk1, signal_level, noise_level, snr);

		/* Found a packet with the requested LAP */
		if (pn != NULL && ac.LAP == pn->LAP) {

			/* Determining UAP requires more symbols. Copy
			 * remaining banks. */
			for (i = 2; i < NUM_BANKS; i++)
				memcpy(syms + i * BANK_LEN,
				       symbols[(i + 1 + bank) % NUM_BANKS],
				       BANK_LEN);
			
			init_packet(&pkt, &syms[ac.offset],
				    BANK_LEN * NUM_BANKS - ac.offset);
			pkt.LAP = ac.LAP;
			pkt.clkn = clk1;
			pkt.channel = rx->channel;
			if (header_present(&pkt)) {
				if (UAP_from_header(&pkt, pn))
					exit(0);
			}
		}
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

/* sniff one target LAP until the UAP is determined */
void rx_uap(struct libusb_device_handle* devh, piconet* pn)
{
	stream_rx_usb(devh, XFER_LEN, 0, cb_lap, pn);
}

/* sniff one target LAP until the UAP is determined */
void rx_uap_file(FILE* fp, piconet* pn)
{
	stream_rx_file(fp, 0, cb_lap, pn);
}

static void cb_hop(void* args, usb_pkt_rx *rx, int bank)
{
	char syms[BANK_LEN * NUM_BANKS];
	int i, j, k;
	access_code ac;
	uint8_t channel;
	uint32_t time; /* in 100 nanosecond units */
	uint8_t clkn_high;
	packet pkt;
	piconet* pn = (piconet *)args;
	uint8_t uap = pn->UAP;

	channel = rx->channel;
	time = le32toh(rx->clk100ns);  /* wire format is le32 */
	clkn_high = rx->clkn_high;
	unpack_symbols(rx->data, symbols[bank]);
	/*
	fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
	*/
	/* awfully repetitious */
	k = 0;
	for (i = 0; i < 2; i++)
		for (j = 0; j < BANK_LEN; j++)
			syms[k++] = symbols[(i + 1 + bank) % NUM_BANKS][j];

	if (find_ac(syms, BANK_LEN, pn->LAP, &ac)) {
		for (i = 2; i < NUM_BANKS; i++)
			for (j = 0; j < BANK_LEN; j++)
				syms[k++] = symbols[(i + 1 + bank) % NUM_BANKS][j];

		init_packet(&pkt, &syms[ac.offset], BANK_LEN * NUM_BANKS - ac.offset);
		pkt.LAP = ac.LAP;
		pkt.clkn = (clkn_high << 19) | ((time + ac.offset * 10) / 6250);
		pkt.channel = channel;

		if ((pkt.LAP == pn->LAP) && header_present(&pkt)) {
			printf("\nGOT PACKET on channel %d, LAP = %06x at time stamp %u, clkn %u\n",
			       channel, pkt.LAP, time + ac.offset * 10, pkt.clkn);

			/* Decode packet - fixing clock drift in the process */
			decode(&pkt, pn);

			if (pn->hop_reversal_inited) {
				//pn->winnowed = 0;
				pn->pattern_indices[pn->packets_observed] = pkt.clkn - pn->first_pkt_time;
				pn->pattern_channels[pn->packets_observed] = pkt.channel;
				pn->packets_observed++;
				pn->total_packets_observed++;
				winnow(pn);
				if (pn->have_clk27) {
					printf("got CLK1-27\n");
					printf("clock offset = %d.\n", pn->clk_offset);
					clk_offset = pn->clk_offset;
					return;
				}
			} else {
				if (pn->have_clk6) {
					UAP_from_header(&pkt, pn);
					if (pn->have_clk27) {
						printf("got CLK1-27\n");
						printf("clock offset = %d.\n", pn->clk_offset);
						clk_offset = pn->clk_offset;
						return;
					}
				} else {
					if (UAP_from_header(&pkt, pn)) {
						if (uap == pn->UAP) {
							printf("got CLK1-6\n");
							init_hop_reversal(0, pn);
							winnow(pn);
						} else {
							printf("failed to confirm UAP\n");
							exit(1);
						}
					}
				}
			}
			if(!pn->have_UAP) {
				pn->have_UAP = 1;
				pn->UAP = uap;
			}
		}
	}
}

/* Follow a given piconet. */
static void cb_follow(void* args, usb_pkt_rx *rx, int bank)
{
	piconet* pn = (piconet *)args;
	char syms[BANK_LEN * NUM_BANKS];
	int i;
	access_code ac;
	packet pkt;
	char *channel_rssi_history;
	int8_t signal_level;
	int8_t noise_level;
	int8_t snr;
	uint32_t clkn, clk1;

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
	signal_level = MAX(channel_rssi_history[0], channel_rssi_history[1]) + RSSI_BASE;

	/* Noise is an IIR of averages */
	noise_level = rx->rssi_avg + RSSI_BASE;
	snr = signal_level - noise_level;

	/* Copy all banks for analysis */
	for (i = 0; i < NUM_BANKS; i++)
		memcpy(syms + i * BANK_LEN,
		       symbols[(i + 1 + bank) % NUM_BANKS],
		       BANK_LEN);

	if ((find_ac(syms, BANK_LEN, pn->LAP, &ac))
		&& (ac.error_count <= max_ac_errors)) {
		/* When reading from file, caller will read
		 * systime before calling this routine, so do
		 * not overwrite. Otherwise, get current time. */
		if ( infile == NULL )
			systime = time(NULL);

		/* If dumpfile is specified, write out all banks to
		 * the file. There could be duplicate data in the dump
		 * if more than one LAP is found within the span of
		 * NUM_BANKS. If experiment mode is selected, extra
		 * info is written out. For now, it just prepends
		 * capture time. */
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

		/* Native (Ubertooth) clock with period 312.5 uS. */
		clkn = le32toh(rx->clk100ns);

		/* Bottom clkn bit not needed, clk1 period is 625 uS. */
		clk1 = clkn >> 1;

		printf("systime=%u ch=%2d LAP=%06x err=%u clk1=0x%07x s=%d n=%d snr=%d\n",
		       (int)systime, rx->channel, ac.LAP, ac.error_count, clk1,
			   signal_level, noise_level, snr);

		init_packet(&pkt, &syms[ac.offset],
			    BANK_LEN * NUM_BANKS - ac.offset);
		pkt.LAP = ac.LAP;
		pkt.UAP = pn->UAP;
		pkt.whitened = 1;
		pkt.clkn = clkn;
		pkt.clock = clk1;
		pkt.have_clk6 = 1;
		pkt.have_clk27 = 1;
		pkt.channel = rx->channel;

		if(decode(&pkt, pn))
			print(&pkt);
		else
			printf("Failed to decode packet\n");
	}
}

/* sniff one target address until CLK is determined */
void rx_hop(struct libusb_device_handle* devh, piconet* pn, int follow)
{
	int ret;
	u64 address = 0;
	address = (pn->LAP & 0xffffff) | (pn->UAP & 0xff) << 24;
	cmd_set_bdaddr(devh, address);

	ret = stream_rx_usb(devh, XFER_LEN, 0, cb_hop, pn);
	if (follow == 1 && ret == 1) {
		/* Allow previous transfers to be cleared out before starting again */
		sleep(1);
		rx_follow_offset(devh, pn);
	}
}

/* sniff one target LAP until the UAP is determined */
void rx_hop_file(FILE* fp, piconet* pn)
{
	stream_rx_file(fp, 0, cb_hop, pn);
}

/* sniff one target address until CLK is determined */
void rx_follow(struct libusb_device_handle* devh, piconet* pn, uint32_t clock, uint32_t delay)
{
	u64 address = 0;
	address = (pn->LAP & 0xffffff) | (pn->UAP & 0xff) << 24;
	cmd_set_bdaddr(devh, address);

	printf("Setting CLKN = 0x%x\n", clock);
	cmd_set_clock(devh, clock);
	init_hop_reversal(0, pn);
	pn->have_clk27 = 1;
	hopping = 1;

	/* delay value shlould be varied based on the delay in reading the clock */
	cmd_start_hopping(devh, delay);
	stream_rx_usb(devh, XFER_LEN, 0, cb_follow, pn);
}

/* sniff one target address until CLK is determined */
void rx_follow_offset(struct libusb_device_handle* devh, piconet* pn)
{
	//u64 address = 0;
	//address = (pn->LAP & 0xffffff) | (pn->UAP & 0xff) << 24;
	//cmd_set_bdaddr(devh, address);

	cmd_start_hopping(devh, pn->clk_offset);
	hopping = 1;
	stream_rx_usb(devh, XFER_LEN, 0, cb_follow, pn);
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

	/* unused parameter */ args = args;
	/* unused parameter */ bank = bank;

	/* Sanity check */
	if (rx->channel > (NUM_CHANNELS-1))
		return;

	if (infile == NULL)
		systime = time(NULL);

		/* Dump to sumpfile if specified */
		if (dumpfile) {
			for(i = 0; i < NUM_BANKS; i++) {
				uint32_t systime_be = htobe32(systime);
				if (fwrite(&systime_be, 
					   sizeof(systime_be), 1,
					   dumpfile)
				    != 1) {;}
				if (fwrite(rx,
					   sizeof(usb_pkt_rx), 1, dumpfile)
				    != 1) {;}
			}
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
