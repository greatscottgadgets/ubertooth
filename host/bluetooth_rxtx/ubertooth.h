/*
 * Copyright 2010 Michael Ossmann
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

#include <libusb-1.0/libusb.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define VENDORID    0xffff
#define PRODUCTID   0x0004
#define DATA_IN     (0x82 | LIBUSB_ENDPOINT_IN)
#define DATA_OUT    (0x05 | LIBUSB_ENDPOINT_OUT)
#define CTRL_IN     (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT    (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)
#define TIMEOUT     2000
#define BUFFER_SIZE 102400

enum ubertooth_usb_commands {
    UBERTOOTH_PING        = 0,
    UBERTOOTH_RX_SYMBOLS  = 1,
    UBERTOOTH_TX_SYMBOLS  = 2,
    UBERTOOTH_GET_USRLED  = 3,
    UBERTOOTH_SET_USRLED  = 4,
    UBERTOOTH_GET_RXLED   = 5,
    UBERTOOTH_SET_RXLED   = 6,
    UBERTOOTH_GET_TXLED   = 7,
    UBERTOOTH_SET_TXLED   = 8,
    UBERTOOTH_GET_1V8     = 9,
    UBERTOOTH_SET_1V8     = 10,
    UBERTOOTH_GET_CHANNEL = 11,
    UBERTOOTH_SET_CHANNEL = 12,
    UBERTOOTH_RESET       = 13
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
	u8     reserved[6];
	u8     data[50];
} usb_pkt_rx;

struct libusb_device_handle* ubertooth_start();
void ubertooth_stop(struct libusb_device_handle *devh);
int stream_rx(struct libusb_device_handle* devh, int xfer_size, u16 num_blocks);
int send_cmd_rx_syms(struct libusb_device_handle* devh, u16 num);
