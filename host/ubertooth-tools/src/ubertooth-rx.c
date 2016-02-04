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

#include "ubertooth.h"
#include <err.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

extern FILE* dumpfile;
extern FILE* infile;
extern int max_ac_errors;

uint8_t calibrated = 0;

static void usage()
{
	printf("ubertooth-rx - passive Bluetooth discovery/decode\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-V print version information\n");
	printf("\t-i filename\n");
	printf("\t-l <LAP> to decode (6 hex), otherwise sniff all LAPs\n");
	printf("\t-u <UAP> to decode (2 hex), otherwise try to calculate (requires LAP)\n");
	printf("\t-U <0-7> set ubertooth device to use\n");
	printf("\t-r<filename> capture packets to PCAPNG file\n");
#ifdef ENABLE_PCAP
	printf("\t-q<filename> capture packets to PCAP file\n");
#endif
	printf("\t-d<filename> dump packets to binary file\n");
	printf("\t-e max_ac_errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\t-s reset channel scanning\n");
	printf("\t-t <SECONDS> sniff timeout - 0 means no timeout [Default: 0]\n");
	printf("\nIf an input file is not specified, an Ubertooth device is used for live capture.\n");
}

static void cb_rx(ubertooth_t* ut, void* args)
{
	btbb_packet* pkt = NULL;
	btbb_piconet* pn = (btbb_piconet *)args;
	char syms[NUM_BANKS * BANK_LEN];
	int offset;
	uint16_t clk_offset;
	uint32_t clkn;
	int i;
	uint32_t lap = LAP_ANY;

	/* Do analysis based on oldest packet */
	usb_pkt_rx* rx = ringbuffer_bottom_usb(ut->packets);

	if (rx->status & DISCARD) {
		goto out;
	}

	// /* Sanity check */
	if (rx->channel > (NUM_BREDR_CHANNELS-1))
		goto out;

	int8_t signal_level = rx->rssi_max;
	int8_t noise_level = rx->rssi_min;
	int8_t snr = signal_level - noise_level;

	/* Copy out remaining banks of symbols for full analysis. */
	for (i = 0; i < NUM_BANKS; i++)
		memcpy(syms + i * BANK_LEN,
		       ringbuffer_get_bt(ut->packets, i),
		       BANK_LEN);

	/* Look for packets with specified LAP, if given. Otherwise
	 * search for any packet. */
	lap = btbb_piconet_get_flag(pn, BTBB_LAP_VALID) ? btbb_piconet_get_lap(pn) : LAP_ANY;

	/* Pass packet-pointer-pointer so that
	 * packet can be created in libbtbb. */
	offset = btbb_find_ac(syms, BANK_LEN, lap, max_ac_errors, &pkt);
	if (offset < 0)
		goto out;

	/* calculate the offset between the first bit of the AC and the rising edge of CLKN */
	clk_offset = (le32toh(rx->clk100ns) + offset*10 + 6250 - 4000) % 6250;

	btbb_packet_set_modulation(pkt, BTBB_MOD_GFSK);
	btbb_packet_set_transport(pkt, BTBB_TRANSPORT_ANY);

	/* Once offset is known for a valid packet, copy in symbols
	 * and other rx data. CLKN here is the 312.5us CLK27-0. The
	 * btbb library can shift it be CLK1 if needed. */
	clkn = (le32toh(rx->clkn_high) << 20) + (le32toh(rx->clk100ns) + offset*10 - 4000) / 3125;
	btbb_packet_set_data(pkt, syms + offset, NUM_BANKS * BANK_LEN - offset,
	                     rx->channel, clkn);

	printf("systime=%u ch=%2d LAP=%06x err=%u clkn=%u clk_offset=%u s=%d n=%d snr=%d\n",
	       (uint32_t)time(NULL),
	       btbb_packet_get_channel(pkt),
	       btbb_packet_get_lap(pkt),
	       btbb_packet_get_ac_errors(pkt),
	       clkn,
	       clk_offset,
	       signal_level,
	       noise_level,
	       snr
	);

	/* calibrate Ubertooth clock such that the first bit of the AC
	 * arrives CLK_TUNE_TIME after the rising edge of CLKN */
	if (!calibrated) {
		if (clk_offset < CLK_TUNE_TIME) {
			printf("offset < CLK_TUNE_TIME\n");
			printf("CLK100ns Trim: %d\n", 6250 + clk_offset - CLK_TUNE_TIME);
			cmd_trim_clock(ut->devh, 6250 + clk_offset - CLK_TUNE_TIME);
		} else if (clk_offset > CLK_TUNE_TIME) {
			printf("offset > CLK_TUNE_TIME\n");
			printf("CLK100ns Trim: %d\n", clk_offset - CLK_TUNE_TIME);
			cmd_trim_clock(ut->devh, clk_offset - CLK_TUNE_TIME);
		}
		calibrated = 1;
		goto out;
	}

	int r = btbb_process_packet(pkt, pn);
	if(r < 0)
		cmd_start_hopping(ut->devh, btbb_piconet_get_clk_offset(pn), 0);

out:
	if (pkt)
		btbb_packet_unref(pkt);
}

int main(int argc, char* argv[])
{
	int opt, have_lap = 0, have_uap = 0;
	int r;
	int timeout = 0;
	int reset_scan = 0;
	char* end;
	char ubertooth_device = -1;
	btbb_piconet* pn = NULL;
	uint32_t lap = 0;
	uint8_t uap = 0;

	ubertooth_t* ut = ubertooth_init();

	while ((opt=getopt(argc,argv,"hVi:l:u:U:d:e:r:sq:t:")) != EOF) {
		switch(opt) {
		case 'i':
			infile = fopen(optarg, "r");
			if (infile == NULL) {
				printf("Could not open file %s\n", optarg);
				usage();
				return 1;
			}
			break;
		case 'l':
			lap = strtol(optarg, &end, 16);
			have_lap++;
			break;
		case 'u':
			uap = strtol(optarg, &end, 16);
			have_uap++;
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'r':
			if (!ut->h_pcapng_bredr) {
				if (btbb_pcapng_create_file( optarg, "Ubertooth", &ut->h_pcapng_bredr )) {
					err(1, "create_bredr_capture_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
#ifdef ENABLE_PCAP
		case 'q':
			if (!ut->h_pcap_bredr) {
				if (btbb_pcap_create_file(optarg, &ut->h_pcap_bredr)) {
					err(1, "btbb_pcap_create_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
#endif
		case 'd':
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'e':
			max_ac_errors = atoi(optarg);
			break;
		case 's':
			++reset_scan;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'V':
			print_version();
			return 0;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	r = ubertooth_connect(ut, ubertooth_device);
	if (r < 0) {
		usage();
		return 1;
	}

	pn = btbb_piconet_new();
	if (have_lap) {
		btbb_init_piconet(pn, lap);
		if (have_uap) {
			btbb_piconet_set_uap(pn, uap);
			cmd_set_bdaddr(ut->devh, btbb_piconet_get_bdaddr(pn));
		}
		if (ut->h_pcapng_bredr) {
			btbb_pcapng_record_bdaddr(ut->h_pcapng_bredr,
						  (((uint32_t)uap)<<24)|lap,
						  have_uap ? 0xff : 0x00, 0);
		}
	} else if (have_uap) {
		printf("Error: UAP but no LAP specified\n");
		usage();
		return 1;
	}

	if (infile == NULL) {

		/* Scan all frequencies. Same effect as
		 * ubertooth-utils -c9999. This is necessary after
		 * following a piconet. */
		if (reset_scan) {
			cmd_set_channel(ut->devh, 9999);
		}

		/* Clean up on exit. */
		register_cleanup_handler(ut);

		r = btbb_init(max_ac_errors);
		if (r < 0)
			return r;

		if (timeout)
			ubertooth_set_timeout(ut, timeout);

		// init USB transfer
		r = ubertooth_bulk_init(ut);
		if (r < 0)
			return r;

		// tell ubertooth to send packets
		r = cmd_rx_syms(ut->devh);
		if (r < 0)
			return r;

		// receive and process each packet
		while(!ut->stop_ubertooth) {
			ubertooth_bulk_wait(ut);
			ubertooth_bulk_receive(ut, cb_rx, pn);
		}

		ubertooth_stop(ut);
	} else {
		rx_file(infile, pn);
		fclose(infile);
	}

	return 0;
}
