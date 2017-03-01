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
#include "ubertooth_fifo.h"
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
	fifo_t* fifo;

	struct libusb_device_handle* devh;
	struct libusb_transfer* rx_xfer;

	uint8_t stop_ubertooth;
	uint64_t abs_start_ns;
	uint32_t start_clk100ns;
	uint64_t last_clk100ns;
	uint64_t clk100ns_upper;

	btbb_pcap_handle* h_pcap_bredr;
	lell_pcap_handle* h_pcap_le;
	btbb_pcapng_handle* h_pcapng_bredr;
	lell_pcapng_handle* h_pcapng_le;
} ubertooth_t;

typedef void (*rx_callback)(ubertooth_t* ut, void* args);

typedef struct {
	unsigned allowed_access_address_errors;
} btle_options;

extern uint32_t systime;
extern FILE* infile;
extern FILE* dumpfile;
extern int max_ac_errors;

void print_version();
void register_cleanup_handler(ubertooth_t* ut, int do_exit);
ubertooth_t* ubertooth_init();
int ubertooth_connect(ubertooth_t* ut, int ubertooth_device);
ubertooth_t* ubertooth_start(int ubertooth_device);
void ubertooth_stop(ubertooth_t* ut);
int ubertooth_get_api(ubertooth_t *ut, uint16_t *version);
int ubertooth_check_api(ubertooth_t *ut);
void ubertooth_set_timeout(ubertooth_t* ut, int seconds);

int ubertooth_bulk_init(ubertooth_t* ut);
void ubertooth_bulk_wait(ubertooth_t* ut);
int ubertooth_bulk_receive(ubertooth_t* ut, rx_callback cb, void* cb_args);
int ubertooth_bulk_thread_start();
void ubertooth_bulk_thread_stop();

int stream_rx_file(ubertooth_t* ut,FILE* fp, rx_callback cb, void* cb_args);

void rx_dump(ubertooth_t* ut, int full);
void rx_btle(ubertooth_t* ut);
void rx_btle_file(FILE* fp);
void rx_afh(ubertooth_t* ut, btbb_piconet* pn, int timeout);
void rx_afh_r(ubertooth_t* ut, btbb_piconet* pn, int timeout);

void ubertooth_unpack_symbols(const uint8_t* buf, char* unpacked);

#endif /* __UBERTOOTH_H__ */
