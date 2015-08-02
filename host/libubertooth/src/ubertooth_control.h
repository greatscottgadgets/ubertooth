/*
 * Copyright 2010-2012 Michael Ossmann, Dominic Spill
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

#ifndef __UBERTOOTH_CONTROL_H__
#define __UBERTOOTH_CONTROL_H__

#include <libusb.h>

#if defined __MACH__
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#define htobe32 EndianU32_NtoB
#define be32toh EndianU32_BtoN
#define le32toh EndianU32_LtoN
#define htobe64 EndianU64_NtoB
#define be64toh EndianU64_BtoN
#define htole16 EndianU16_NtoL
#define htole32 EndianU32_NtoL
#else
#include <endian.h>
#endif

#include <stdio.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#include "ubertooth_interface.h"

#define U0_VENDORID    0x1d50
#define U0_PRODUCTID   0x6000
#define U1_VENDORID    0x1d50
#define U1_PRODUCTID   0x6002
#define TC13_VENDORID  0xffff
#define TC13_PRODUCTID 0x0004

#define DATA_IN     (0x82 | LIBUSB_ENDPOINT_IN)
#define DATA_OUT    (0x05 | LIBUSB_ENDPOINT_OUT)
#define CTRL_IN     (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT    (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)
#define TIMEOUT     20000
#define BUFFER_SIZE 102400

/* RX USB packet parameters */
#define PKT_LEN       64
#define SYM_LEN       50
#define SYM_OFFSET    14
#define PKTS_PER_XFER 8
#define NUM_BANKS     10
#define XFER_LEN      (PKT_LEN * PKTS_PER_XFER)
#define BANK_LEN      (SYM_LEN * PKTS_PER_XFER)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

void show_libusb_error(int error_code);
int cmd_rx_syms(struct libusb_device_handle* devh);
int cmd_specan(struct libusb_device_handle* devh, u16 low_freq, u16 high_freq);
int cmd_led_specan(struct libusb_device_handle* devh, u16 rssi_threshold);
int cmd_set_usrled(struct libusb_device_handle* devh, u16 state);
int cmd_get_usrled(struct libusb_device_handle* devh);
int cmd_set_rxled(struct libusb_device_handle* devh, u16 state);
int cmd_get_rxled(struct libusb_device_handle* devh);
int cmd_set_txled(struct libusb_device_handle* devh, u16 state);
int cmd_get_txled(struct libusb_device_handle* devh);
int cmd_get_partnum(struct libusb_device_handle* devh);
void print_serial(u8 *serial, FILE *fileptr);
int cmd_get_serial(struct libusb_device_handle* devh, u8 *serial);
int cmd_set_modulation(struct libusb_device_handle* devh, u16 mod);
int cmd_get_modulation(struct libusb_device_handle* devh);
int cmd_set_isp(struct libusb_device_handle* devh);
int cmd_reset(struct libusb_device_handle* devh);
int cmd_stop(struct libusb_device_handle* devh);
int cmd_set_paen(struct libusb_device_handle* devh, u16 state);
int cmd_set_hgm(struct libusb_device_handle* devh, u16 state);
int cmd_tx_test(struct libusb_device_handle* devh);
int cmd_flash(struct libusb_device_handle* devh);
int cmd_get_palevel(struct libusb_device_handle* devh);
int cmd_set_palevel(struct libusb_device_handle* devh, u16 level);
int cmd_get_channel(struct libusb_device_handle* devh);
int cmd_set_channel(struct libusb_device_handle* devh, u16 channel);
int cmd_get_rangeresult(struct libusb_device_handle* devh, rangetest_result *rr);
int cmd_range_test(struct libusb_device_handle* devh);
int cmd_repeater(struct libusb_device_handle* devh);
void cmd_get_rev_num(struct libusb_device_handle* devh, char *version, u8 len);
void cmd_get_compile_info(struct libusb_device_handle* devh, char *compile_info, u8 len);
int cmd_get_board_id(struct libusb_device_handle* devh);
int cmd_set_squelch(struct libusb_device_handle* devh, u16 level);
int cmd_get_squelch(struct libusb_device_handle* devh);
int cmd_set_bdaddr(struct libusb_device_handle* devh, u64 bdaddr);
int cmd_set_syncword(struct libusb_device_handle* devh, u64 syncword);
int cmd_next_hop(struct libusb_device_handle* devh, u16 clk);
int cmd_start_hopping(struct libusb_device_handle* devh, int clock_offset);
int cmd_set_clock(struct libusb_device_handle* devh, u32 clkn);
uint32_t cmd_get_clock(struct libusb_device_handle* devh);
int cmd_set_afh_map(struct libusb_device_handle* devh, u8* afh_map);
int cmd_clear_afh_map(struct libusb_device_handle* devh);
int cmd_btle_sniffing(struct libusb_device_handle* devh, u16 num);
u32 cmd_get_access_address(struct libusb_device_handle* devh);
int cmd_set_access_address(struct libusb_device_handle* devh, u32 access_address);
int cmd_do_something(struct libusb_device_handle *devh, unsigned char *data, int len);
int cmd_do_something_reply(struct libusb_device_handle* devh, unsigned char *data, int len);
int cmd_get_crc_verify(struct libusb_device_handle* devh);
int cmd_set_crc_verify(struct libusb_device_handle* devh, int verify);
int cmd_poll(struct libusb_device_handle* devh, usb_pkt_rx *p);
int cmd_btle_promisc(struct libusb_device_handle* devh);
int cmd_read_register(struct libusb_device_handle* devh, u8 reg);
int cmd_btle_slave(struct libusb_device_handle* devh, u8 *mac_address);
int cmd_btle_set_target(struct libusb_device_handle* devh, u8 *mac_address);
int cmd_set_jam_mode(struct libusb_device_handle* devh, int mode);
int cmd_ego(struct libusb_device_handle* devh, int mode);

#endif /* __UBERTOOTH_CONTROL_H__ */
