/*
 * Copyright 2010 - 2013 Michael Ossmann, Dominic Spill, Will Code, Mike Ryan
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

#ifndef __UBERTOOTH_H__
#define __UBERTOOTH_H__

#include "ubertooth_control.h"
#include "ubertooth_ringbuffer.h"
#include <btbb.h>

/* specan output types
 * see https://github.com/dkogan/feedgnuplot for plotter */
enum specan_modes {
	SPECAN_STDOUT         = 0,
	SPECAN_GNUPLOT_NORMAL = 1,
	SPECAN_GNUPLOT_3D     = 2,
	SPECAN_FILE           = 3
};

enum board_ids {
	BOARD_ID_UBERTOOTH_ZERO = 0,
	BOARD_ID_UBERTOOTH_ONE  = 1,
	BOARD_ID_TC13BADGE      = 2
};

typedef struct {
	/* Ringbuffers for USB and Bluetooth symbols */
	ringbuffer_t* packets;
	usb_pkt_rx usb_packets[NUM_BANKS];
	char br_symbols[NUM_BANKS][BANK_LEN];

	struct libusb_device_handle* devh;
	struct libusb_transfer* rx_xfer;
	uint8_t* empty_usb_buf;
	uint8_t* full_usb_buf;
	uint8_t usb_really_full;
	uint8_t usb_retry;

	uint8_t stop_ubertooth;
	uint64_t abs_start_ns;
	uint32_t start_clk100ns;
	uint64_t last_clk100ns;
	uint64_t clk100ns_upper;
	btbb_piconet* follow_pn;
} ubertooth_t;

typedef void (*rx_callback)(ubertooth_t* ut, void* args);

typedef struct {
	unsigned allowed_access_address_errors;
} btle_options;

void print_version();
void register_cleanup_handler(ubertooth_t* ut);
ubertooth_t* ubertooth_init();
ubertooth_t* ubertooth_start(int ubertooth_device);
void ubertooth_stop(ubertooth_t* ut);
int specan(ubertooth_t* ut, int xfer_size, u16 low_freq,
           u16 high_freq, u8 output_mode);
int cmd_ping(struct libusb_device_handle* devh);
int stream_rx_usb(ubertooth_t* ut, int xfer_size,
                  rx_callback cb, void* cb_args);
int stream_rx_file(FILE* fp, rx_callback cb, void* cb_args);
void rx_live(ubertooth_t* ut, btbb_piconet* pn, int timeout);
void rx_file(FILE* fp, btbb_piconet* pn);
void rx_dump(ubertooth_t* ut, int full);
void rx_btle(ubertooth_t* ut);
void rx_btle_file(FILE* fp);
void cb_btle(ubertooth_t* ut, void* args);
void cb_ego(ubertooth_t* ut, void* args);

#ifdef ENABLE_PCAP
extern btbb_pcap_handle * h_pcap_bredr;
extern lell_pcap_handle * h_pcap_le;
#endif
extern btbb_pcapng_handle * h_pcapng_bredr;
extern lell_pcapng_handle * h_pcapng_le;
#endif /* __UBERTOOTH_H__ */
