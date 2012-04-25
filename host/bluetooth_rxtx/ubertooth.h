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

#ifndef __UBERTOOTH_H__
#define __UBERTOOTH_H__

#ifdef FREEBSD
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <stdio.h>
#include <bluetooth_piconet.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define VENDORID    0xffff
#define PRODUCTID   0x0004
#define DATA_IN     (0x82 | LIBUSB_ENDPOINT_IN)
#define DATA_OUT    (0x05 | LIBUSB_ENDPOINT_OUT)
#define CTRL_IN     (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT    (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)
#define TIMEOUT     5000
#define BUFFER_SIZE 102400
#define NUM_BANKS   10

/* RX USB packet parameters */
#define PKT_LEN       64
#define SYM_LEN       50
#define SYM_OFFSET    14
#define PKTS_PER_XFER 8
#define XFER_LEN      (PKT_LEN * PKTS_PER_XFER)
#define BANK_LEN      (SYM_LEN * PKTS_PER_XFER)

/* gnuplot output types
 * see https://github.com/dkogan/feedgnuplot for plotter */
#define GNUPLOT_NORMAL		1
#define GNUPLOT_3D		2

enum ubertooth_usb_commands {
    UBERTOOTH_PING         = 0,
    UBERTOOTH_RX_SYMBOLS   = 1,
    UBERTOOTH_TX_SYMBOLS   = 2,
    UBERTOOTH_GET_USRLED   = 3,
    UBERTOOTH_SET_USRLED   = 4,
    UBERTOOTH_GET_RXLED    = 5,
    UBERTOOTH_SET_RXLED    = 6,
    UBERTOOTH_GET_TXLED    = 7,
    UBERTOOTH_SET_TXLED    = 8,
    UBERTOOTH_GET_1V8      = 9,
    UBERTOOTH_SET_1V8      = 10,
    UBERTOOTH_GET_CHANNEL  = 11,
    UBERTOOTH_SET_CHANNEL  = 12,
    UBERTOOTH_RESET        = 13,
	UBERTOOTH_GET_SERIAL   = 14,
	UBERTOOTH_GET_PARTNUM  = 15,
	UBERTOOTH_GET_PAEN     = 16,
	UBERTOOTH_SET_PAEN     = 17,
	UBERTOOTH_GET_HGM      = 18,
	UBERTOOTH_SET_HGM      = 19,
	UBERTOOTH_TX_TEST      = 20,
	UBERTOOTH_STOP         = 21,
	UBERTOOTH_GET_MOD      = 22,
	UBERTOOTH_SET_MOD      = 23,
	UBERTOOTH_SET_ISP      = 24,
	UBERTOOTH_FLASH        = 25,
	BOOTLOADER_FLASH       = 26,
	UBERTOOTH_SPECAN       = 27,
	UBERTOOTH_GET_PALEVEL  = 28,
	UBERTOOTH_SET_PALEVEL  = 29,
	UBERTOOTH_REPEATER     = 30,
	UBERTOOTH_RANGE_TEST   = 31,
	UBERTOOTH_RANGE_CHECK  = 32,
	UBERTOOTH_GET_REV_NUM  = 33,
	UBERTOOTH_LED_SPECAN   = 34,
	UBERTOOTH_GET_BOARD_ID = 35,
	UBERTOOTH_SET_SQUELCH  = 36,
	UBERTOOTH_GET_SQUELCH  = 37
};

enum modulations {
	MOD_BT_BASIC_RATE = 0,
	MOD_BT_LOW_ENERGY = 1,
	MOD_80211_FHSS    = 2
};

enum board_ids {
	BOARD_ID_UBERTOOTH_ZERO = 0,
	BOARD_ID_UBERTOOTH_ONE  = 1,
	BOARD_ID_TC13BADGE      = 2
};

/*
 * USB packet for Bluetooth RX (64 total bytes)
 */
typedef struct {
	u8     pkt_type;
	u8     status;
	u8     channel;
	u8     clkn_high;
	u32    clk100ns;
	char   rssi_max;
	char   rssi_min;
	char   rssi_avg;
	u8     rssi_count;
	u8     reserved[2];
	u8     data[50];
} usb_pkt_rx;

typedef struct {
	u8 valid;
	u8 request_pa;
	u8 request_num;
	u8 reply_pa;
	u8 reply_num;
} rangetest_result;

typedef void (*rx_callback)(void* args, usb_pkt_rx *rx, int bank);

struct libusb_device_handle* ubertooth_start();
extern void ubertooth_stop(struct libusb_device_handle *devh);
extern int specan(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks,
		  u16 low_freq, u16 high_freq);
extern int do_specan(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks,
		     u16 low_freq, u16 high_freq, char gnuplot);
extern int cmd_ping(struct libusb_device_handle* devh);
extern int stream_rx_usb(struct libusb_device_handle* devh, int xfer_size,
			 uint16_t num_blocks, rx_callback cb, void* cb_args);
extern int stream_rx_file(FILE* fp, uint16_t num_blocks, rx_callback cb, void* cb_args);
extern void rx_lap(struct libusb_device_handle* devh);
extern void rx_lap_file(FILE* fp);
extern void rx_uap(struct libusb_device_handle* devh, piconet* pn);
extern void rx_uap_file(FILE* fp, piconet* pn);
extern void rx_hop(struct libusb_device_handle* devh, piconet* pn);
extern void rx_hop_file(FILE* fp, piconet* pn);
extern void rx_dump(struct libusb_device_handle* devh, int full);
extern void rx_btle(struct libusb_device_handle* devh);
extern void rx_btle_file(FILE* fp);
extern int cmd_rx_syms(struct libusb_device_handle* devh, u16 num);
extern int cmd_specan(struct libusb_device_handle* devh, u16 low_freq, u16 high_freq);
extern int cmd_led_specan(struct libusb_device_handle* devh, u16 rssi_threshold);
extern int cmd_set_usrled(struct libusb_device_handle* devh, u16 state);
extern int cmd_get_usrled(struct libusb_device_handle* devh);
extern int cmd_set_rxled(struct libusb_device_handle* devh, u16 state);
extern int cmd_get_rxled(struct libusb_device_handle* devh);
extern int cmd_set_txled(struct libusb_device_handle* devh, u16 state);
extern int cmd_get_txled(struct libusb_device_handle* devh);
extern int cmd_get_partnum(struct libusb_device_handle* devh);
extern int cmd_get_serial(struct libusb_device_handle* devh);
extern int cmd_set_modulation(struct libusb_device_handle* devh, u16 mod);
extern int cmd_get_modulation(struct libusb_device_handle* devh);
extern int cmd_set_isp(struct libusb_device_handle* devh);
extern int cmd_reset(struct libusb_device_handle* devh);
extern int cmd_stop(struct libusb_device_handle* devh);
extern int cmd_set_paen(struct libusb_device_handle* devh, u16 state);
extern int cmd_set_hgm(struct libusb_device_handle* devh, u16 state);
extern int cmd_tx_test(struct libusb_device_handle* devh);
extern int cmd_flash(struct libusb_device_handle* devh);
extern int cmd_get_palevel(struct libusb_device_handle* devh);
extern int cmd_set_palevel(struct libusb_device_handle* devh, u16 level);
extern int cmd_get_channel(struct libusb_device_handle* devh);
extern int cmd_set_channel(struct libusb_device_handle* devh, u16 channel);
extern int cmd_get_rangeresult(struct libusb_device_handle* devh, rangetest_result *rr);
extern int cmd_range_test(struct libusb_device_handle* devh);
extern int cmd_repeater(struct libusb_device_handle* devh);
extern int cmd_get_rev_num(struct libusb_device_handle* devh);
extern int cmd_get_board_id(struct libusb_device_handle* devh);
extern int cmd_set_squelch(struct libusb_device_handle* devh, u16 level);
extern int cmd_get_squelch(struct libusb_device_handle* devh);
#endif
