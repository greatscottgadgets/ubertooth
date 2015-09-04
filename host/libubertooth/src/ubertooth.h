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
#include <btbb.h>

/* Mark unused variables to avoid gcc/clang warnings */
#define UNUSED(x) (void)(x)

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

typedef void (*rx_callback)(void* args, usb_pkt_rx *rx, int bank);

typedef struct {
	unsigned allowed_access_address_errors;
} btle_options;

void print_version();
void register_cleanup_handler(struct libusb_device_handle *devh);
struct libusb_device_handle* ubertooth_start(int ubertooth_device);
void ubertooth_stop(struct libusb_device_handle *devh);
int specan(struct libusb_device_handle* devh, int xfer_size, u16 low_freq,
		   u16 high_freq, u8 output_mode);
int cmd_ping(struct libusb_device_handle* devh);
int stream_rx_usb(struct libusb_device_handle* devh, int xfer_size,
				  rx_callback cb, void* cb_args);
int stream_rx_file(FILE* fp, rx_callback cb, void* cb_args);
void rx_live(struct libusb_device_handle* devh, btbb_piconet* pn, int timeout);
void rx_file(FILE* fp, btbb_piconet* pn);
void rx_dump(struct libusb_device_handle* devh, int full);
void rx_btle(struct libusb_device_handle* devh);
void rx_btle_file(FILE* fp);
void cb_btle(void* args, usb_pkt_rx *rx, int bank);
void cb_ego(void* args, usb_pkt_rx *rx, int bank);

#ifdef ENABLE_PCAP
extern btbb_pcap_handle * h_pcap_bredr;
extern lell_pcap_handle * h_pcap_le;
#endif
extern btbb_pcapng_handle * h_pcapng_bredr;
extern lell_pcapng_handle * h_pcapng_le;
#endif /* __UBERTOOTH_H__ */
