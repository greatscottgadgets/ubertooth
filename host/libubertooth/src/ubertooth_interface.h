/*
 * Copyright 2012 Dominic Spill
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

#ifndef __UBERTOOTH_INTERFACE_H
#define __UBERTOOTH_INTERFACE_H

#include <stdint.h>

// increment on every API change
#define UBERTOOTH_API_VERSION 0x0107

#define DMA_SIZE 50

#define NUM_BREDR_CHANNELS 79

/*
 * CLK_TUNE_TIME is the duration in units of 100 ns that we reserve for tuning
 * the radio while frequency hopping.  We start the tuning process
 * CLK_TUNE_TIME * 100 ns prior to the start of an upcoming time slot.
*/
#define CLK_TUNE_TIME   2250
#define CLK_TUNE_OFFSET  200

enum ubertooth_usb_commands {
	UBERTOOTH_PING               = 0,
	UBERTOOTH_RX_SYMBOLS         = 1,
	UBERTOOTH_TX_SYMBOLS         = 2,
	UBERTOOTH_GET_USRLED         = 3,
	UBERTOOTH_SET_USRLED         = 4,
	UBERTOOTH_GET_RXLED          = 5,
	UBERTOOTH_SET_RXLED          = 6,
	UBERTOOTH_GET_TXLED          = 7,
	UBERTOOTH_SET_TXLED          = 8,
	UBERTOOTH_GET_1V8            = 9,
	UBERTOOTH_SET_1V8            = 10,
	UBERTOOTH_GET_CHANNEL        = 11,
	UBERTOOTH_SET_CHANNEL        = 12,
	UBERTOOTH_RESET              = 13,
	UBERTOOTH_GET_SERIAL         = 14,
	UBERTOOTH_GET_PARTNUM        = 15,
	UBERTOOTH_GET_PAEN           = 16,
	UBERTOOTH_SET_PAEN           = 17,
	UBERTOOTH_GET_HGM            = 18,
	UBERTOOTH_SET_HGM            = 19,
	UBERTOOTH_TX_TEST            = 20,
	UBERTOOTH_STOP               = 21,
	UBERTOOTH_GET_MOD            = 22,
	UBERTOOTH_SET_MOD            = 23,
	UBERTOOTH_SET_ISP            = 24,
	UBERTOOTH_FLASH              = 25,
	BOOTLOADER_FLASH             = 26,
	UBERTOOTH_SPECAN             = 27,
	UBERTOOTH_GET_PALEVEL        = 28,
	UBERTOOTH_SET_PALEVEL        = 29,
	UBERTOOTH_REPEATER           = 30,
	UBERTOOTH_RANGE_TEST         = 31,
	UBERTOOTH_RANGE_CHECK        = 32,
	UBERTOOTH_GET_REV_NUM        = 33,
	UBERTOOTH_LED_SPECAN         = 34,
	UBERTOOTH_GET_BOARD_ID       = 35,
	UBERTOOTH_SET_SQUELCH        = 36,
	UBERTOOTH_GET_SQUELCH        = 37,
	UBERTOOTH_SET_BDADDR         = 38,
	UBERTOOTH_START_HOPPING      = 39,
	UBERTOOTH_SET_CLOCK          = 40,
	UBERTOOTH_GET_CLOCK          = 41,
	UBERTOOTH_BTLE_SNIFFING      = 42,
	UBERTOOTH_GET_ACCESS_ADDRESS = 43,
	UBERTOOTH_SET_ACCESS_ADDRESS = 44,
	UBERTOOTH_DO_SOMETHING       = 45,
	UBERTOOTH_DO_SOMETHING_REPLY = 46,
	UBERTOOTH_GET_CRC_VERIFY     = 47,
	UBERTOOTH_SET_CRC_VERIFY     = 48,
	UBERTOOTH_POLL               = 49,
	UBERTOOTH_BTLE_PROMISC       = 50,
	UBERTOOTH_SET_AFHMAP         = 51,
	UBERTOOTH_CLEAR_AFHMAP       = 52,
	UBERTOOTH_READ_REGISTER      = 53,
	UBERTOOTH_BTLE_SLAVE         = 54,
	UBERTOOTH_GET_COMPILE_INFO   = 55,
	UBERTOOTH_BTLE_SET_TARGET    = 56,
	UBERTOOTH_BTLE_PHY           = 57,
	UBERTOOTH_WRITE_REGISTER     = 58,
	UBERTOOTH_JAM_MODE           = 59,
	UBERTOOTH_EGO                = 60,
	UBERTOOTH_AFH                = 61,
	UBERTOOTH_HOP                = 62,
	UBERTOOTH_TRIM_CLOCK         = 63,
	// UBERTOOTH_GET_API_VERSION    = 64,
	UBERTOOTH_WRITE_REGISTERS    = 65,
	UBERTOOTH_READ_ALL_REGISTERS = 66,
	UBERTOOTH_RX_GENERIC         = 67,
	UBERTOOTH_TX_GENERIC_PACKET  = 68,
	UBERTOOTH_FIX_CLOCK_DRIFT    = 69,
	UBERTOOTH_CANCEL_FOLLOW      = 70,
	UBERTOOTH_LE_SET_ADV_DATA    = 71,
	UBERTOOTH_RFCAT_SUBCMD       = 72,
	UBERTOOTH_XMAS               = 73,
};

enum rfcat24_subcommands {
	RFCAT_RESERVED               = 0,
	RFCAT_SET_AA                 = 1,
	RFCAT_CAP_LEN                = 2,
};

// maximum adv data by the 5.0 specs 
#define LE_ADV_MAX_LEN 255
enum jam_modes {
	JAM_NONE       = 0,
	JAM_ONCE       = 1,
	JAM_CONTINUOUS = 2,
};

enum modulations {
	MOD_BT_BASIC_RATE = 0,
	MOD_BT_LOW_ENERGY = 1,
	MOD_80211_FHSS    = 2,
	MOD_NONE          = 3
};

enum usb_pkt_types {
	BR_PACKET  = 0,
	LE_PACKET  = 1,
	MESSAGE    = 2,
	KEEP_ALIVE = 3,
	SPECAN     = 4,
	LE_PROMISC = 5,
	EGO_PACKET = 6,
};

enum hop_mode {
	HOP_NONE      = 0,
	HOP_SWEEP     = 1,
	HOP_BLUETOOTH = 2,
	HOP_BTLE      = 3,
	HOP_DIRECT    = 4,
	HOP_AFH       = 5,
};

enum usb_pkt_status {
	DMA_OVERFLOW  = 0x01,
	DMA_ERROR     = 0x02,
	FIFO_OVERFLOW = 0x04,
	CS_TRIGGER    = 0x08,
	RSSI_TRIGGER  = 0x10,
	DISCARD       = 0x20,
};

/*
 * USB packet for Bluetooth RX (64 total bytes)
 */
typedef struct {
	uint8_t  pkt_type;
	uint8_t  status;
	uint8_t  channel;
	uint8_t  clkn_high;
	uint32_t clk100ns;
	int8_t   rssi_max;   // Max RSSI seen while collecting symbols in this packet
	int8_t   rssi_min;   // Min ...
	int8_t   rssi_avg;   // Average ...
	uint8_t  rssi_count; // Number of ... (0 means RSSI stats are invalid)
	uint8_t  reserved[2];
	uint8_t  data[DMA_SIZE];
} usb_pkt_rx;

typedef struct {
	uint64_t address;
	uint64_t syncword;
} bdaddr;

typedef struct {
	uint8_t valid;
	uint8_t request_pa;
	uint8_t request_num;
	uint8_t reply_pa;
	uint8_t reply_num;
} rangetest_result;

typedef struct {
	uint16_t synch;
	uint16_t syncl;
	uint16_t channel;
	uint8_t  length;
	uint8_t  pa_level;
	uint8_t  data[DMA_SIZE];
} generic_tx_packet;

/* BTBR USB interface */
enum {
	BTUSB_MSG_START = 'S',
	BTUSB_MSG_CONT = 'C',
	BTUSB_EARLY_PRINT = 'P'
};

/* BTCTL interface */
typedef enum btctl_state_e {
	BTCTL_STATE_STANDBY	= 0,
	BTCTL_STATE_INQUIRY	= 1,
	BTCTL_STATE_PAGE	= 2,
	BTCTL_STATE_CONNECTED	= 3,
	BTCTL_STATE_INQUIRY_SCAN = 4,
	BTCTL_STATE_PAGE_SCAN	= 5,
	BTCTL_STATE_COUNT
} btctl_state_t;

typedef enum btctl_reason_e {
	BTCTL_REASON_SUCCESS	= 0,
	BTCTL_REASON_TIMEOUT	= 1,
	/* REASON_PAGED indicates that device entered CONNECTED state,
	 * but the slave did not answer yet */
	BTCTL_REASON_PAGED	= 2
} btctl_reason_t;

typedef enum {
	BTCTL_DEBUG		= 0,
	/* Any -> Device */
	BTCTL_RESET_REQ		= 20,
	BTCTL_IDLE_REQ		= 21,
	/* Host -> Device */
	BTCTL_SET_FREQ_OFF_REQ	= 22,
	BTCTL_SET_BDADDR_REQ	= 23,
	BTCTL_INQUIRY_REQ	= 24,
	BTCTL_PAGING_REQ	= 25,
	BTCTL_SET_MAX_AC_ERRORS_REQ = 26,
	BTCTL_TX_ACL_REQ	= 27,
	BTCTL_INQUIRY_SCAN_REQ	= 28,
	BTCTL_PAGE_SCAN_REQ	= 29,
	BTCTL_SET_EIR_REQ	= 30,// in a btctl_tx_pkt_t
	BTCTL_SET_AFH_REQ	= 31,
	BTCTL_MONITOR_REQ	= 32,
	/* Device -> Host */
	BTCTL_RX_PKT		= 40,
	BTCTL_STATE_RESP	= 41
} btctl_cmd_type_t ;

typedef struct btctl_hdr_s {
	uint8_t type;
	uint8_t padding[3];
	uint8_t data[0];
} __attribute__((packed)) btctl_hdr_t;

typedef struct btctl_paging_req_s {
	uint64_t bdaddr;
} btctl_paging_req_t;

typedef struct btctl_set_reg_req_s {
	uint16_t reg;
} btctl_set_reg_req_t;

typedef struct btctl_set_bdaddr_req_s {
	uint64_t bdaddr;
} btctl_set_bdaddr_req_t;

typedef struct btctl_set_afh_req_s {
	uint32_t instant;
	uint8_t mode;
	uint8_t map[10];
} __attribute__((packed)) btctl_set_afh_req_t;

typedef struct btctl_state_resp_s {
	uint8_t state;
	uint8_t reason;
} btctl_state_resp_t;

typedef struct bthdr_s {
	uint8_t lt_addr;
	uint8_t type;
	uint8_t flags;
	uint8_t hec;
} __attribute__((packed)) bbhdr_t;

#define BTHDR_FLOW 0
#define BTHDR_ARQN 1
#define BTHDR_SEQN 2

#define BBPKT_F_HAS_PKT		0
#define BBPKT_F_HAS_HDR		1
#define BBPKT_F_HAS_CRC		2
#define BBPKT_F_GOOD_CRC	3
#define BBPKT_HAS_PKT(p)	(((p)->flags & (1<<BBPKT_F_HAS_PKT))!=0)
#define BBPKT_HAS_HDR(p)	(((p)->flags & (1<<BBPKT_F_HAS_HDR))!=0)
#define BBPKT_HAS_CRC(p)	(((p)->flags & (1<<BBPKT_F_HAS_CRC))!=0)
#define BBPKT_GOOD_CRC(p)	(((p)->flags & (1<<BBPKT_F_GOOD_CRC))!=0)

#define MAX_ACL_PACKET_SIZE 344

typedef struct {
	uint32_t clkn;		// Clkn sampled at start of rx
	uint8_t chan;		// Channel number
	uint8_t flags;		// Decode flags
	uint16_t data_size;	// Size of bt_data field
	bbhdr_t bb_hdr;		// decoded header
	uint8_t bt_data[0];	// maximum data size for an ACL packet
} __attribute__((packed)) btctl_rx_pkt_t;

typedef struct {
	bbhdr_t bb_hdr;		// header (only lt_addr/type/flags are relevant)
	uint8_t bt_data[0];	// maximum data size for an ACL packet
} __attribute__((packed)) btctl_tx_pkt_t;

#endif /* __UBERTOOTH_INTERFACE_H */
