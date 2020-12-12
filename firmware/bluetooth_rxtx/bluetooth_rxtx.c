/*
 * Copyright 2010-2013 Michael Ossmann
 * Copyright 2011-2013 Dominic Spill
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

#include <string.h>

#include "ubertooth.h"
#include "ubertooth_usb.h"
#include "ubertooth_interface.h"
#include "ubertooth_rssi.h"
#include "ubertooth_cs.h"
#include "ubertooth_dma.h"
#include "ubertooth_clock.h"
#include "bluetooth.h"
#include "bluetooth_le.h"
#include "cc2400_rangetest.h"
#include "ego.h"
#include "debug_uart.h"
#include "xmas.h"

#define MIN(x,y)	((x)<(y)?(x):(y))
#define MAX(x,y)	((x)>(y)?(x):(y))

/* build info */
const char compile_info[] =
	"ubertooth " GIT_REVISION " (" COMPILE_BY "@" COMPILE_HOST ") " TIMESTAMP;

/* hopping stuff */
volatile uint8_t  hop_mode = HOP_NONE;
volatile uint8_t  do_hop = 0;                  // set by timer interrupt
volatile uint16_t channel = 2441;
volatile uint16_t hop_direct_channel = 0;      // for hopping directly to a channel
volatile uint16_t hop_timeout = 158;
volatile uint16_t requested_channel = 0;
volatile uint16_t le_adv_channel = 2402;
volatile int      cancel_follow = 0;

/* bulk USB stuff */
volatile uint8_t  idle_buf_clkn_high = 0;
volatile uint32_t idle_buf_clk100ns = 0;
volatile uint16_t idle_buf_channel = 0;
volatile uint8_t  dma_discard = 0;
volatile uint8_t  status = 0;

/* operation mode */
volatile uint8_t mode = MODE_IDLE;
volatile uint8_t requested_mode = MODE_IDLE;
volatile uint8_t jam_mode = JAM_NONE;
volatile uint8_t ego_mode = EGO_FOLLOW;

volatile uint8_t modulation = MOD_BT_BASIC_RATE;

/* specan stuff */
volatile uint16_t low_freq = 2400;
volatile uint16_t high_freq = 2483;
volatile int8_t rssi_threshold = -30;  // -54dBm - 30 = -84dBm

/* Generic TX stuff */
generic_tx_packet tx_pkt;

/* le slave stuff */
uint8_t slave_mac_address[6] = { 0, };
uint8_t le_adv_data[LE_ADV_MAX_LEN] = { 0x02, 0x01, 0x05 };
unsigned le_adv_len = 3;

le_state_t le = {
	.access_address = 0x8e89bed6,           // advertising channel access address
	.synch = 0x6b7d,                        // bit-reversed adv channel AA
	.syncl = 0x9171,
	.crc_init  = 0x555555,                  // advertising channel CRCInit
	.crc_init_reversed = 0xAAAAAA,
	.crc_verify = 0,

	.link_state = LINK_INACTIVE,
	.conn_epoch = 0,
	.do_follow = 1,
	.target_set = 0,
	.last_packet = 0,
};

typedef struct _le_promisc_active_aa_t {
	u32 aa;
	int count;
} le_promisc_active_aa_t;

typedef struct _le_promisc_state_t {
	// LFU cache of recently seen AA's
	le_promisc_active_aa_t active_aa[32];

	// recovering hop interval
	u32 smallest_hop_interval;
	int consec_intervals;
} le_promisc_state_t;
le_promisc_state_t le_promisc;
#define AA_LIST_SIZE (int)(sizeof(le_promisc.active_aa) / sizeof(le_promisc_active_aa_t))

/* LE jamming */
#define JAM_COUNT_DEFAULT 40
int le_jam_count = 0;

/* set LE access address */
static void le_set_access_address(u32 aa);

typedef int (*data_cb_t)(char *);
data_cb_t data_cb = NULL;

typedef void (*packet_cb_t)(u8 *);
packet_cb_t packet_cb = NULL;

/* Unpacked symbol buffers (two rxbufs) */
char unpacked[DMA_SIZE*8*2];

static int enqueue(uint8_t type, uint8_t* buf)
{
	usb_pkt_rx* f = usb_enqueue();

	/* fail if queue is full */
	if (f == NULL) {
		status |= FIFO_OVERFLOW;
		return 0;
	}

	f->pkt_type = type;
	if(type == SPECAN) {
		f->clkn_high = (clkn >> 20) & 0xff;
		f->clk100ns = CLK100NS;
	} else {
		f->clkn_high = idle_buf_clkn_high;
		f->clk100ns = idle_buf_clk100ns;
		f->channel = (uint8_t)((idle_buf_channel - 2402) & 0xff);
		f->rssi_min = rssi_min;
		f->rssi_max = rssi_max;
		f->rssi_avg = rssi_get_avg(idle_buf_channel);
		f->rssi_count = rssi_count;
	}

	memcpy(f->data, buf, DMA_SIZE);

	f->status = status;
	status = 0;

	return 1;
}

int enqueue_with_ts(uint8_t type, uint8_t* buf, uint32_t ts)
{
	usb_pkt_rx* f = usb_enqueue();

	/* fail if queue is full */
	if (f == NULL) {
		status |= FIFO_OVERFLOW;
		return 0;
	}

	f->pkt_type = type;

	f->clkn_high = 0;
	f->clk100ns = ts;

	f->channel = (uint8_t)((channel - 2402) & 0xff);
	f->rssi_avg = 0;
	f->rssi_count = 0;

	memcpy(f->data, buf, DMA_SIZE);

	f->status = status;
	status = 0;

	return 1;
}

static int vendor_request_handler(uint8_t request, uint16_t* request_params, uint8_t* data, int* data_len)
{
	uint32_t clock;
	size_t length; // string length
	usb_pkt_rx* p = NULL;
	uint16_t reg_val;
	uint8_t i;
	unsigned data_in_len = request_params[2];

	switch (request) {

	case UBERTOOTH_PING:
		*data_len = 0;
		break;

	case UBERTOOTH_RX_SYMBOLS:
		requested_mode = MODE_RX_SYMBOLS;
		*data_len = 0;
		break;

#ifdef TX_ENABLE
	case UBERTOOTH_TX_SYMBOLS:
		hop_mode = HOP_BLUETOOTH;
		requested_mode = MODE_TX_SYMBOLS;
		*data_len = 0;
		break;
#endif

	case UBERTOOTH_GET_USRLED:
		data[0] = (USRLED) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_USRLED:
		if (request_params[0])
			USRLED_SET;
		else
			USRLED_CLR;
		break;

	case UBERTOOTH_GET_RXLED:
		data[0] = (RXLED) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_RXLED:
		if (request_params[0])
			RXLED_SET;
		else
			RXLED_CLR;
		break;

	case UBERTOOTH_GET_TXLED:
		data[0] = (TXLED) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_TXLED:
		if (request_params[0])
			TXLED_SET;
		else
			TXLED_CLR;
		break;

	case UBERTOOTH_GET_1V8:
		data[0] = (CC1V8) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_1V8:
		if (request_params[0])
			CC1V8_SET;
		else
			CC1V8_CLR;
		break;

	case UBERTOOTH_GET_PARTNUM:
		get_part_num(data, data_len);
		break;

	case UBERTOOTH_RESET:
		requested_mode = MODE_RESET;
		break;

	case UBERTOOTH_GET_SERIAL:
		get_device_serial(data, data_len);
		break;

#ifdef UBERTOOTH_ONE
	case UBERTOOTH_GET_PAEN:
		data[0] = (PAEN) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_PAEN:
		if (request_params[0])
			PAEN_SET;
		else
			PAEN_CLR;
		break;

	case UBERTOOTH_GET_HGM:
		data[0] = (HGM) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_HGM:
		if (request_params[0])
			HGM_SET;
		else
			HGM_CLR;
		break;
#endif

#ifdef TX_ENABLE
	case UBERTOOTH_TX_TEST:
		requested_mode = MODE_TX_TEST;
		break;

	case UBERTOOTH_GET_PALEVEL:
		data[0] = cc2400_get(FREND) & 0x7;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_PALEVEL:
		if( request_params[0] < 8 ) {
			cc2400_set(FREND, 8 | request_params[0]);
		} else {
			return 0;
		}
		break;

	case UBERTOOTH_RANGE_TEST:
		requested_mode = MODE_RANGE_TEST;
		break;

	case UBERTOOTH_REPEATER:
		requested_mode = MODE_REPEATER;
		break;
#endif

	case UBERTOOTH_RANGE_CHECK:
		data[0] = rr.valid;
		data[1] = rr.request_pa;
		data[2] = rr.request_num;
		data[3] = rr.reply_pa;
		data[4] = rr.reply_num;
		*data_len = 5;
		break;

	case UBERTOOTH_STOP:
		requested_mode = MODE_IDLE;
		break;

	case UBERTOOTH_GET_MOD:
		data[0] = modulation;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_MOD:
		modulation = request_params[0];
		break;

	case UBERTOOTH_GET_CHANNEL:
		data[0] = channel & 0xFF;
		data[1] = (channel >> 8) & 0xFF;
		*data_len = 2;
		break;

	case UBERTOOTH_SET_CHANNEL:
		requested_channel = request_params[0];
		/* bluetooth band sweep mode, start at channel 2402 */
		if (requested_channel > MAX_FREQ) {
			hop_mode = HOP_SWEEP;
			requested_channel = 2402;
		}
		/* fixed channel mode, can be outside bluetooth band */
		else {
			hop_mode = HOP_NONE;
			requested_channel = MAX(requested_channel, MIN_FREQ);
			requested_channel = MIN(requested_channel, MAX_FREQ);
		}

		le_adv_channel = requested_channel;
		if (mode != MODE_BT_FOLLOW_LE) {
			channel = requested_channel;
			requested_channel = 0;

			/* CS threshold is mode-dependent. Update it after
			 * possible mode change. TODO - kludgy. */
			cs_threshold_calc_and_set(channel);
		}
		break;

	case UBERTOOTH_SET_ISP:
		set_isp();
		*data_len = 0; /* should never return */
		break;

	case UBERTOOTH_FLASH:
		bootloader_ctrl = DFU_MODE;
		requested_mode = MODE_RESET;
		break;

	case UBERTOOTH_SPECAN:
		if (request_params[0] < 2049 || request_params[0] > 3072 ||
				request_params[1] < 2049 || request_params[1] > 3072 ||
				request_params[1] < request_params[0])
			return 0;
		low_freq = request_params[0];
		high_freq = request_params[1];
		requested_mode = MODE_SPECAN;
		*data_len = 0;
		break;

	case UBERTOOTH_RX_GENERIC:
		requested_mode = MODE_RX_GENERIC;
		*data_len = 0;
		break;

	case UBERTOOTH_LED_SPECAN:
		if (request_params[0] > 256)
			return 0;
		rssi_threshold = 54 - request_params[0];
		requested_mode = MODE_LED_SPECAN;
		*data_len = 0;
		break;

	case UBERTOOTH_GET_REV_NUM:
		data[0] = 0x00;
		data[1] = 0x00;

		length = (u8)strlen(GIT_REVISION);
		data[2] = length;

		memcpy(&data[3], GIT_REVISION, length);

		*data_len = 2 + 1 + length;
		break;

	case UBERTOOTH_GET_COMPILE_INFO:
		length = (u8)strlen(compile_info);
		data[0] = length;
		memcpy(&data[1], compile_info, length);
		*data_len = 1 + length;
		break;

	case UBERTOOTH_GET_BOARD_ID:
		data[0] = BOARD_ID;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_SQUELCH:
		cs_threshold_req = (int8_t)request_params[0];
		cs_threshold_calc_and_set(channel);
		break;

	case UBERTOOTH_GET_SQUELCH:
		data[0] = cs_threshold_req;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_BDADDR:
		target.address = 0;
		target.syncword = 0;
		for(int i=0; i < 8; i++) {
			target.address |= (uint64_t)data[i] << 8*i;
		}
		for(int i=0; i < 8; i++) {
			target.syncword |= (uint64_t)data[i+8] << 8*i;
		}
		precalc();
		break;

	case UBERTOOTH_START_HOPPING:
		clkn_offset = 0;
		for(int i=0; i < 4; i++) {
			clkn_offset <<= 8;
			clkn_offset |= data[i];
		}
		hop_mode = HOP_BLUETOOTH;
		dma_discard = 1;
		DIO_SSEL_SET;
		clk100ns_offset = (data[4] << 8) | (data[5] << 0);
		requested_mode = MODE_BT_FOLLOW;
		break;

	case UBERTOOTH_AFH:
		hop_mode = HOP_AFH;
		requested_mode = MODE_AFH;

		for(int i=0; i < 10; i++) {
			afh_map[i] = 0;
		}
		used_channels = 0;
		afh_enabled = 1;
		break;

	case UBERTOOTH_HOP:
		do_hop = 1;
		break;

	case UBERTOOTH_SET_CLOCK:
		clock = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
		clkn = clock;
		cs_threshold_calc_and_set(channel);
		break;

	case UBERTOOTH_SET_AFHMAP:
		for(int i=0; i < 10; i++) {
			afh_map[i] = data[i];
		}
		afh_enabled = 1;
		*data_len = 10;
		break;

	case UBERTOOTH_CLEAR_AFHMAP:
		for(int i=0; i < 10; i++) {
			afh_map[i] = 0;
		}
		afh_enabled = 0;
		*data_len = 10;
		break;

	case UBERTOOTH_GET_CLOCK:
		clock = clkn;
		for(int i=0; i < 4; i++) {
			data[i] = (clock >> (8*i)) & 0xff;
		}
		*data_len = 4;
		break;

	case UBERTOOTH_TRIM_CLOCK:
		clk100ns_offset = (data[0] << 8) | (data[1] << 0);
		break;

	case UBERTOOTH_FIX_CLOCK_DRIFT:
		clk_drift_ppm += (int16_t)(data[0] << 8) | (data[1] << 0);

		// Too slow
		if (clk_drift_ppm < 0) {
			clk_drift_correction = 320 / (uint16_t)(-clk_drift_ppm);
			clkn_next_drift_fix = clkn_last_drift_fix + clk_drift_correction;
		}
		// Too fast
		else if (clk_drift_ppm > 0) {
			clk_drift_correction = 320 / clk_drift_ppm;
			clkn_next_drift_fix = clkn_last_drift_fix + clk_drift_correction;
		}
		// Don't trim
		else {
			clk_drift_correction = 0;
			clkn_next_drift_fix = 0;
		}

		break;

	case UBERTOOTH_BTLE_SNIFFING:
		le.do_follow = request_params[0];
		*data_len = 0;

		do_hop = 0;
		hop_mode = HOP_BTLE;
		requested_mode = MODE_BT_FOLLOW_LE;

		usb_queue_init();
		cs_threshold_calc_and_set(channel);
		break;

	case UBERTOOTH_GET_ACCESS_ADDRESS:
		for(int i=0; i < 4; i++) {
			data[i] = (le.access_address >> (8*i)) & 0xff;
		}
		*data_len = 4;
		break;

	case UBERTOOTH_SET_ACCESS_ADDRESS:
		le_set_access_address(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
		le.target_set = 1;
		break;

	case UBERTOOTH_DO_SOMETHING:
		// do something! just don't commit anything here
		break;

	case UBERTOOTH_DO_SOMETHING_REPLY:
		// after you do something, tell me what you did!
		// don't commit here please
		data[0] = 0x13;
		data[1] = 0x37;
		*data_len = 2;
		break;

	case UBERTOOTH_GET_CRC_VERIFY:
		data[0] = le.crc_verify ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_CRC_VERIFY:
		le.crc_verify = request_params[0] ? 1 : 0;
		break;

	case UBERTOOTH_POLL:
		p = dequeue();
		if (p != NULL) {
			memcpy(data, (void *)p, sizeof(usb_pkt_rx));
			*data_len = sizeof(usb_pkt_rx);
		} else {
			data[0] = 0;
			*data_len = 1;
		}
		break;

	case UBERTOOTH_BTLE_PROMISC:
		*data_len = 0;

		hop_mode = HOP_NONE;
		requested_mode = MODE_BT_PROMISC_LE;

		usb_queue_init();
		cs_threshold_calc_and_set(channel);
		break;

	case UBERTOOTH_READ_REGISTER:
		reg_val = cc2400_get(request_params[0]);
		data[0] = (reg_val >> 8) & 0xff;
		data[1] = reg_val & 0xff;
		*data_len = 2;
		break;

	case UBERTOOTH_WRITE_REGISTER:
		cc2400_set(request_params[0] & 0xff, request_params[1]);
		break;

	case UBERTOOTH_WRITE_REGISTERS:
		for(i=0; i<request_params[0]; i++) {
			reg_val = (data[(i*3)+1] << 8) | data[(i*3)+2];
			cc2400_set(data[i*3], reg_val);
		}
		break;

	case UBERTOOTH_READ_ALL_REGISTERS:
		#define MAX_READ_REG 0x2d
		for(i=0; i<=MAX_READ_REG; i++) {
			reg_val = cc2400_get(i);
			data[i*3] = i;
			data[(i*3)+1] = (reg_val >> 8) & 0xff;
			data[(i*3)+2] = reg_val & 0xff;
		}
		*data_len = MAX_READ_REG*3;
		break;

#ifdef TX_ENABLE
	case UBERTOOTH_TX_GENERIC_PACKET:
		i = 7 + data[6];
		memcpy(&tx_pkt, data, i);
		//tx_pkt.channel = data[4] << 8 | data[5];
		requested_mode = MODE_TX_GENERIC;
		*data_len = 0;
		break;

	case UBERTOOTH_BTLE_SLAVE:
		memcpy(slave_mac_address, data, 6);
		requested_mode = MODE_BT_SLAVE_LE;
		break;
#endif

	case UBERTOOTH_LE_SET_ADV_DATA:
		// make sure the data fits in our buffer
		if (data_in_len > LE_ADV_MAX_LEN)
			return 0;
		le_adv_len = data_in_len;
		memcpy(le_adv_data, data, le_adv_len);
		break;

	case UBERTOOTH_BTLE_SET_TARGET:
		// Addresses appear in packets in reverse-octet order.
		// Store the target address in reverse order so that we can do a simple memcmp later
		if (data[6] > 48) {
			return 0; // invalid mask
		}
		else if (data[6] == 0) {
			le.target_set = 0;
			memset(le.target, 0, 6);
			memset(le.target_mask, 0, 6);
		} else {
			unsigned last;

			for (i = 0; i < 6; ++i)
				le.target[i] = data[5-i];

			// compute mask
			memset(le.target_mask, 0, 6);
			for (i = 5; data[6] > 8; --i, data[6] -= 8) {
				le.target_mask[i] = 0xff;
			}
			last = i;

			if (data[6] > 0) {
				uint8_t final_byte = 0;
				for (i = 0; i < data[6]; ++i) {
					final_byte >>= 1;
					final_byte |= 0b10000000;
				}
				le.target_mask[last] = final_byte;
			}

			// in case the user specifies a bad mask
			for (i = 0; i < 5; ++i)
				le.target[i] &= le.target_mask[i];

			le.target_set = 1;
		}
		break;

	case UBERTOOTH_CANCEL_FOLLOW:
		// cancel following an active connection
		cancel_follow = 1;
		break;

#ifdef TX_ENABLE
	case UBERTOOTH_JAM_MODE:
		jam_mode = request_params[0];
		break;
#endif

	case UBERTOOTH_EGO:
		ego_mode = request_params[0];
#ifndef TX_ENABLE
		if (ego_mode == EGO_JAM)
			return 0;
#endif
		requested_mode = MODE_EGO;
		break;

	case UBERTOOTH_RFCAT_SUBCMD:
		return rfcat_subcommand(request_params[0], data, *data_len);
		break;

	case UBERTOOTH_XMAS:
		requested_mode = MODE_XMAS;
		break;

	default:
		return 0;
	}
	return 1;
}

/* Update CLKN. */
void TIMER0_IRQHandler()
{
	if (T0IR & TIR_MR0_Interrupt) {

		clkn += clkn_offset + 1;
		clkn_offset = 0;

		uint32_t le_clk = (clkn - le.conn_epoch) & 0x03;

		/* Trigger hop based on mode */

		/* NONE or SWEEP -> 25 Hz */
		if (hop_mode == HOP_NONE || hop_mode == HOP_SWEEP) {
			if ((clkn & 0x7f) == 0)
				do_hop = 1;
		}
		/* BLUETOOTH -> 1600 Hz */
		else if (hop_mode == HOP_BLUETOOTH) {
			if ((clkn & 0x1) == 0)
				do_hop = 1;
		}
		/* BLUETOOTH Low Energy -> 7.5ms - 4.0s in multiples of 1.25 ms */
		else if (hop_mode == HOP_BTLE) {
			// Only hop if connected
			if (le.link_state == LINK_CONNECTED && le_clk == 0) {
				--le.interval_timer;
				if (le.interval_timer == 0) {
					do_hop = 1;
					++le.conn_count;
					le.interval_timer = le.conn_interval;
				} else {
					TXLED_CLR; // hack!
				}
			}
		}
		else if (hop_mode == HOP_AFH) {
			if( (last_hop + hop_timeout) == clkn ) {
				do_hop = 1;
			}
		}

		// Fix linear clock drift deviation
		if(clkn_next_drift_fix != 0 && clk100ns_offset == 0) {
			if(clkn >= clkn_next_drift_fix) {

				// Too fast
				if(clk_drift_ppm >= 0) {
					clk100ns_offset = 1;
				}

				// Too slow
				else {
					clk100ns_offset = 6249;
				}
				clkn_last_drift_fix = clkn;
				clkn_next_drift_fix = clkn_last_drift_fix + clk_drift_correction;
			}
		}

		// Negative clock correction
		if(clk100ns_offset > 3124)
			clkn += 2;

		T0MR0 = 3124 + clk100ns_offset;
		clk100ns_offset = 0;

		/* Ack interrupt */
		T0IR = TIR_MR0_Interrupt;
	}
}

/* EINT3 handler is also defined in ubertooth.c for TC13BADGE. */
#ifndef TC13BADGE
void EINT3_IRQHandler()
{
	/* TODO - check specific source of shared interrupt */
	IO2IntClr   = PIN_GIO6; // clear interrupt
	DIO_SSEL_CLR;           // enable SPI
	cs_trigger  = 1;        // signal trigger
	if (hop_mode == HOP_BLUETOOTH)
		dma_discard = 0;

}
#endif // TC13BADGE

/*
 * Sleep (busy wait) for 'millis' milliseconds
 * Needs clkn. Be sure to call clkn_init() before using it.
 */
static void msleep(uint32_t millis)
{
	uint32_t now = (clkn & 0xffffff);
	uint32_t stop_at = now + millis * 10000 / 3125; // millis -> clkn ticks

	// handle clkn overflow
	if (stop_at >= ((uint32_t)1<<28)) {
		stop_at -= ((uint32_t)1<<28);
		while ((clkn & 0xffffff) >= now || (clkn & 0xffffff) < stop_at);
	} else {
		while ((clkn & 0xffffff) < stop_at);
	}
}

void legacy_DMA_IRQHandler();
void le_DMA_IRQHandler();
void DMA_IRQHandler(void) {
	if (mode == MODE_BT_FOLLOW_LE)
		le_DMA_IRQHandler();
	else
		legacy_DMA_IRQHandler();

	// DMA channel 7: debug UART
	if (DMACIntStat & (1 << 7)) {
		// TC -- DMA completed, unset flag so another printf can occur
		if (DMACIntTCStat & (1 << 7)) {
			DMACIntTCClear = (1 << 7);
			debug_dma_active = 0;
		}
		// error -- blow up
		if (DMACIntErrStat & (1 << 7)) {
			DMACIntErrClr = (1 << 7);
			// FIXME do something better here
			USRLED_SET;
			while (1) { }
		}
	}
}

void legacy_DMA_IRQHandler()
{
	if ( mode == MODE_RX_SYMBOLS
	   || mode == MODE_BT_FOLLOW
	   || mode == MODE_SPECAN
	   || mode == MODE_BT_FOLLOW_LE
	   || mode == MODE_BT_PROMISC_LE
	   || mode == MODE_BT_SLAVE_LE
	   || mode == MODE_RX_GENERIC)
	{
		/* interrupt on channel 0 */
		if (DMACIntStat & (1 << 0)) {
			if (DMACIntTCStat & (1 << 0)) {
				DMACIntTCClear = (1 << 0);

				if (hop_mode == HOP_BLUETOOTH)
					DIO_SSEL_SET;

				idle_buf_clk100ns  = CLK100NS;
				idle_buf_clkn_high = (clkn >> 20) & 0xff;
				idle_buf_channel   = channel;

				/* Keep buffer swapping in sync with DMA. */
				volatile uint8_t* tmp = active_rxbuf;
				active_rxbuf = idle_rxbuf;
				idle_rxbuf = tmp;

				++rx_tc;
			}
			if (DMACIntErrStat & (1 << 0)) {
				DMACIntErrClr = (1 << 0);
				++rx_err;
			}
		}
	}
}

static void cc2400_idle()
{
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK)); // need to wait for unlock?

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
	HGM_CLR;
#endif

	RXLED_CLR;
	TXLED_CLR;
	USRLED_CLR;

	clkn_stop();
	dio_ssp_stop();
	cs_reset();
	rssi_reset();

	/* hopping stuff */
	hop_mode = HOP_NONE;
	do_hop = 0;
	channel = 2441;
	hop_direct_channel = 0;
	hop_timeout = 158;
	requested_channel = 0;
	le_adv_channel = 2402;


	/* bulk USB stuff */
	idle_buf_clkn_high = 0;
	idle_buf_clk100ns = 0;
	idle_buf_channel = 0;
	dma_discard = 0;
	status = 0;

	/* operation mode */
	mode = MODE_IDLE;
	requested_mode = MODE_IDLE;
	jam_mode = JAM_NONE;
	ego_mode = EGO_FOLLOW;

	modulation = MOD_BT_BASIC_RATE;

	/* specan stuff */
	low_freq = 2400;
	high_freq = 2483;
	rssi_threshold = -30;

	target.address = 0;
	target.syncword = 0;
}

/* start un-buffered rx */
static void cc2400_rx()
{
	u16 mdmctrl = 0;

	if((modulation == MOD_BT_BASIC_RATE) || (modulation == MOD_BT_LOW_ENERGY)) {
		if (modulation == MOD_BT_BASIC_RATE) {
			mdmctrl = 0x0029; // 160 kHz frequency deviation
		} else if (modulation == MOD_BT_LOW_ENERGY) {
			mdmctrl = 0x0040; // 250 kHz frequency deviation
		}
		cc2400_set(MANAND,  0x7fff);
		cc2400_set(LMTST,   0x2b22);
		cc2400_set(MDMTST0, 0x134b); // without PRNG
		cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
		// 0 00 00 0 010 00 0 00 0 1
		//      |  | |   |  +--------> CRC off
		//      |  | |   +-----------> sync word: 8 MSB bits of SYNC_WORD
		//      |  | +---------------> 2 preamble bytes of 01010101
		//      |  +-----------------> not packet mode
			//      +--------------------> un-buffered mode
		cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
		cc2400_set(MDMCTRL, mdmctrl);
	}

	// Set up CS register
	cs_threshold_calc_and_set(channel);

	clkn_start();

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
#ifdef UBERTOOTH_ONE
	PAEN_SET;
	HGM_SET;
#endif
}

/* start un-buffered rx */
static void cc2400_rx_sync(u32 sync)
{
	u16 grmdm, mdmctrl;

	if (modulation == MOD_BT_BASIC_RATE) {
		mdmctrl = 0x0029; // 160 kHz frequency deviation
		grmdm = 0x0461; // un-buffered mode, packet w/ sync word detection
		// 0 00 00 1 000 11 0 00 0 1
		//   |  |  | |   |  +--------> CRC off
		//   |  |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
		//   |  |  | +---------------> 0 preamble bytes of 01010101
		//   |  |  +-----------------> packet mode
		//   |  +--------------------> un-buffered mode
		//   +-----------------------> sync error bits: 0

	} else if (modulation == MOD_BT_LOW_ENERGY) {
		mdmctrl = 0x0040; // 250 kHz frequency deviation
		grmdm = 0x0561; // un-buffered mode, packet w/ sync word detection
		// 0 00 00 1 010 11 0 00 0 1
		//   |  |  | |   |  +--------> CRC off
		//   |  |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
		//   |  |  | +---------------> 2 preamble bytes of 01010101
		//   |  |  +-----------------> packet mode
		//   |  +--------------------> un-buffered mode
		//   +-----------------------> sync error bits: 0

	} else {
		/* oops */
		return;
	}

	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);

	cc2400_set(MDMTST0, 0x124b);
	// 1      2      4b
	// 00 0 1 0 0 10 01001011
	//    | | | | |  +---------> AFC_DELTA = ??
	//    | | | | +------------> AFC settling = 4 pairs (8 bit preamble)
	//    | | | +--------------> no AFC adjust on packet
	//    | | +----------------> do not invert data
	//    | +------------------> TX IF freq 1 0Hz
	//    +--------------------> PRNG off
	//
	// ref: CC2400 datasheet page 67
	// AFC settling explained page 41/42

	cc2400_set(GRMDM,   grmdm);

	cc2400_set(SYNCL,   sync & 0xffff);
	cc2400_set(SYNCH,   (sync >> 16) & 0xffff);

	cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
	cc2400_set(MDMCTRL, mdmctrl);

	// Set up CS register
	cs_threshold_calc_and_set(channel);

	clkn_start();

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
#ifdef UBERTOOTH_ONE
	PAEN_SET;
	HGM_SET;
#endif
}

/* start buffered tx */
#ifdef TX_ENABLE
static void cc2400_tx_sync(uint32_t sync)
{
	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);    // LNA and receive mixers test register
	cc2400_set(MDMTST0, 0x134b);    // no PRNG

	cc2400_set(GRMDM,   0x0c01);
	// 0 00 01 1 000 00 0 00 0 1
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 8 MSB bits of SYNC_WORD
	//      |  | +---------------> 0 preamble bytes of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> buffered mode

	cc2400_set(SYNCL,   sync & 0xffff);
	cc2400_set(SYNCH,   (sync >> 16) & 0xffff);

	cc2400_set(FSDIV,   channel);
	cc2400_set(FREND,   0b1011);    // amplifier level (-7 dBm, picked from hat)

	if (modulation == MOD_BT_BASIC_RATE) {
		cc2400_set(MDMCTRL, 0x0029);    // 160 kHz frequency deviation
	} else if (modulation == MOD_BT_LOW_ENERGY) {
		cc2400_set(MDMCTRL, 0x0040);    // 250 kHz frequency deviation
	} else {
		/* oops */
		return;
	}

	clkn_start();

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));

#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif

	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	cc2400_strobe(STX);

}
#endif

/*
 * Transmit a BTLE packet with the specified access address.
 *
 * All modulation parameters are set within this function. The data
 * should not be pre-whitened, but the CRC should be calculated and
 * included in the data length.
 *
 */
#ifdef TX_ENABLE
void le_transmit(u32 aa, u8 len, u8 *data)
{
	unsigned int fifo_space, i, j;
	int bit;
	u8 txbuf[LE_ADV_MAX_LEN];
	u8 tx_len;
	u8 byte;
	uint32_t sync = rbit(aa);
	u16 gio_save;

	// whiten the data and copy it into the txbuf
	int idx = whitening_index[btle_channel_index(channel)];
	for (i = 0; i < len; ++i) {
		byte = data[i];
		txbuf[i] = 0;
		for (j = 0; j < 8; ++j) {
			bit = (byte & 1) ^ whitening[idx];
			idx = (idx + 1) % sizeof(whitening);
			byte >>= 1;
			txbuf[i] |= bit << (7 - j);
		}
	}

	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);    // LNA and receive mixers test register
	cc2400_set(MDMTST0, 0x134b);    // no PRNG

	cc2400_set(GRMDM,   0x0ce1);
	// 0 00 01 1 001 11 0 00 0 1
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: all 32 bits of SYNC_WORD
	//      |  | +---------------> 1 preamble byte of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> buffered mode

	cc2400_set(FSDIV,   channel);
	cc2400_set(FREND,   0b1011);    // amplifier level (-7 dBm, picked from hat)
	cc2400_set(MDMCTRL, 0x0040);    // 250 kHz frequency deviation
	cc2400_set(INT,     0x0010);    // FIFO_THRESHOLD: 16 bytes

	// set sync word to bit-reversed AA
	cc2400_set(SYNCL,   sync & 0xffff);
	cc2400_set(SYNCH,   (sync >> 16) & 0xffff);

	// set GIO to FIFO_FULL
	gio_save = cc2400_get(IOCFG);
	cc2400_set(IOCFG, (GIO_FIFO_EMPTY << 9) | (gio_save & 0x1ff));

	// turn on the radio
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	/*
		This is the correct way of talking to the CC2400 FIFO, the algorithm is as follows:
	 	While i need to send more data:
	 		1. Wait for FIFO to drain according to FIFO_THRESHOLD, (or if first iteration immediately)
	 		2. Fill the FIFO with the correct amount of data to send (fifo_space is an indicator for how much data has "drained" from the fifo and is now available for us to fill up again)
	 		3. Signal FIFO to send data
	*/
	fifo_space = 32; // We start at 32, because the C2400 FIFO overall size is 32 bytes.
	i = 0;
	while (len != 0)
	{
		// Wait for the FIFO to drain (when FIFO_FULL is false)	
		while (!GIO6);
		// Decide how much we should send in this iteration
		if (len >= fifo_space) {
			len -= fifo_space;
			tx_len = fifo_space;
		} else {
			tx_len = len;
			len = 0;
		}
		// Signal FIFO to send data
		cc2400_fifo_write(tx_len, txbuf + i);
		cc2400_strobe(STX);
		i += tx_len;
		// From now on, we only get interrupt on GIO6, which means that we work according to FIFO_THRESHOLD now, the fifo is only half full.
		fifo_space = 16;
	}
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	TXLED_CLR;

	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
#endif
	// Restore GIO
 	cc2400_set(IOCFG, gio_save);
}
#endif

#ifdef TX_ENABLE
void le_jam(void) {
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);    // LNA and receive mixers test register
	cc2400_set(MDMTST0, 0x234b);    // PRNG, 1 MHz offset

	cc2400_set(GRMDM,   0x0c01);
	// 0 00 01 1 000 00 0 00 0 1
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 8 MSB bits of SYNC_WORD
	//      |  | +---------------> 0 preamble bytes of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> buffered mode

	// cc2400_set(FSDIV,   channel);
	cc2400_set(FREND,   0b1011);    // amplifier level (-7 dBm, picked from hat)
	cc2400_set(MDMCTRL, 0x0040);    // 250 kHz frequency deviation

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	cc2400_strobe(STX);
}
#endif

/* TODO - return whether hop happened, or should caller have to keep
 * track of this? */
void hop(void)
{
	do_hop = 0;
	last_hop = clkn;

	// No hopping, if channel is set correctly, do nothing
	if (hop_mode == HOP_NONE) {
		if (cc2400_get(FSDIV) == (channel - 1))
			return;
	}
	/* Slow sweep (100 hops/sec)
	 * only hop to currently used channels if AFH is enabled
	 */
	else if (hop_mode == HOP_SWEEP) {
		do {
			channel += 32;
			if (channel > 2480)
				channel -= 79;
		} while ( used_channels != 0 && afh_enabled && !( afh_map[(channel-2402)/8] & 0x1<<((channel-2402)%8) ) );
	}

	/* AFH detection
	 * only hop to currently unused channesl
	 */
	else if (hop_mode == HOP_AFH) {
		do {
			channel += 32;
			if (channel > 2480)
				channel -= 79;
		} while( used_channels != 79 && (afh_map[(channel-2402)/8] & 0x1<<((channel-2402)%8)) );
	}

	else if (hop_mode == HOP_BLUETOOTH) {
		channel = next_hop(clkn);
	}

	else if (hop_mode == HOP_BTLE) {
		channel = btle_next_hop(&le);
	}

	else if (hop_mode == HOP_DIRECT) {
		channel = hop_direct_channel;
	}
	/* IDLE mode, but leave amp on, so don't call cc2400_idle(). */
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK)); // need to wait for unlock?

	/* Retune */
	if(mode == MODE_TX_SYMBOLS)
		cc2400_set(FSDIV, channel);
	else
		cc2400_set(FSDIV, channel - 1);

	/* Update CS register if hopping.  */
	if (hop_mode > 0) {
		cs_threshold_calc_and_set(channel);
	}

	/* Wait for lock */
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));

	dma_discard = 1;

	if(mode == MODE_TX_SYMBOLS) {
#ifdef TX_ENABLE
		cc2400_strobe(STX);
#endif
	} else {
		cc2400_strobe(SRX);
	}
}

/* Bluetooth packet monitoring */
void bt_stream_rx()
{
	int8_t rssi;
	int8_t rssi_at_trigger;

	RXLED_CLR;

	usb_queue_init();
	dio_ssp_init();
	dma_init_rx_symbols();
	dio_ssp_start();

	cc2400_rx();

	cs_trigger_enable();

	while ( requested_mode == MODE_RX_SYMBOLS || requested_mode == MODE_BT_FOLLOW )
	{

		RXLED_CLR;

		/* Wait for DMA transfer. TODO - need more work on
		 * RSSI. Should send RSSI indications to host even
		 * when not transferring data. That would also keep
		 * the USB stream going. This loop runs 50-80 times
		 * while waiting for DMA, but RSSI sampling does not
		 * cover all the symbols in a DMA transfer. Can not do
		 * RSSI sampling in CS interrupt, but could log time
		 * at multiple trigger points there. The MAX() below
		 * helps with statistics in the case that cs_trigger
		 * happened before the loop started. */
		rssi_reset();
		rssi_at_trigger = INT8_MIN;
		while (!rx_tc) {
			rssi = (int8_t)(cc2400_get(RSSI) >> 8);
			if (cs_trigger && (rssi_at_trigger == INT8_MIN)) {
				rssi = MAX(rssi,(cs_threshold_cur+54));
				rssi_at_trigger = rssi;
			}
			rssi_add(rssi);

			handle_usb(clkn);

			/* If timer says time to hop, do it. */
			if (do_hop) {
				hop();
			} else {
				TXLED_CLR;
			}
			/* TODO - set per-channel carrier sense threshold.
			 * Set by firmware or host. */
		}

		RXLED_SET;

		if (rx_err) {
			status |= DMA_ERROR;
		}

		/* Missed a DMA trasfer? */
		if (rx_tc > 1)
			status |= DMA_OVERFLOW;

		if (dma_discard) {
			status |= DISCARD;
			dma_discard = 0;
		}

		rssi_iir_update(channel);

		/* Set squelch hold if there was either a CS trigger, squelch
		 * is disabled, or if the current rssi_max is above the same
		 * threshold. Currently, this is redundant, but allows for
		 * per-channel or other rssi triggers in the future. */
		if (cs_trigger || cs_no_squelch) {
			status |= CS_TRIGGER;
			cs_trigger = 0;
		}

		if (rssi_max >= (cs_threshold_cur + 54)) {
			status |= RSSI_TRIGGER;
		}

		enqueue(BR_PACKET, (uint8_t*)idle_rxbuf);

		handle_usb(clkn);
		rx_tc = 0;
		rx_err = 0;
	}

	dio_ssp_stop();
	cs_trigger_disable();
}

static uint8_t reverse8(uint8_t data)
{
	uint8_t reversed = 0;

	for(size_t i=0; i<8; i++)
	{
		reversed |= ((data >> i) & 0x01) << (7-i);
	}

	return reversed;
}

static uint16_t reverse16(uint16_t data)
{
	uint16_t reversed = 0;

	for(size_t i=0; i<16; i++)
	{
		reversed |= ((data >> i) & 0x01) << (15-i);
	}

	return reversed;
}

/*
 * Transmit a BTBR packet with the specified access code.
 *
 * All modulation parameters are set within this function.
 */
#ifdef TX_ENABLE
void br_transmit()
{
	uint16_t gio_save;

	uint32_t clkn_saved = 0;

	uint16_t preamble = (target.syncword & 1) == 1 ? 0x5555 : 0xaaaa;
	uint8_t trailer = ((target.syncword >> 63) & 1) == 1 ? 0xaa : 0x55;

	uint8_t data[16] = {
		reverse8((target.syncword >> 0) & 0xFF),
		reverse8((target.syncword >> 8) & 0xFF),
		reverse8((target.syncword >> 16) & 0xFF),
		reverse8((target.syncword >> 24) & 0xFF),
		reverse8((target.syncword >> 32) & 0xFF),
		reverse8((target.syncword >> 40) & 0xFF),
		reverse8((target.syncword >> 48) & 0xFF),
		reverse8((target.syncword >> 56) & 0xFF),
		reverse8(trailer),
		reverse8(0x77),
		reverse8(0x66),
		reverse8(0x55),
		reverse8(0x44),
		reverse8(0x33),
		reverse8(0x22),
		reverse8(0x11)
	};

	cc2400_tx_sync(reverse16(preamble));

	cc2400_set(INT,     0x0014);    // FIFO_THRESHOLD: 20 bytes

	// set GIO to FIFO_FULL
	gio_save = cc2400_get(IOCFG);
	cc2400_set(IOCFG, (GIO_FIFO_FULL << 9) | (gio_save & 0x1ff));

	while ( requested_mode == MODE_TX_SYMBOLS )
	{

		while ((clkn >> 1) == (clkn_saved >> 1) || T0TC < 2250) {

			// If timer says time to hop, do it.
			if (do_hop) {
				hop();
			}
		}

		clkn_saved = clkn;

		TXLED_SET;

		cc2400_fifo_write(16, data);

		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		TXLED_CLR;

		cc2400_strobe(SRFOFF);
		while ((cc2400_status() & FS_LOCK));

		while (!(cc2400_status() & XOSC16M_STABLE));
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));

		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		cc2400_strobe(STX);

		handle_usb(clkn);
	}

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
#endif

	// reset GIO
	cc2400_set(IOCFG, gio_save);
}
#endif

/* set LE access address */
static void le_set_access_address(u32 aa) {
	u32 aa_rev;

	le.access_address = aa;
	aa_rev = rbit(aa);
	le.syncl = aa_rev & 0xffff;
	le.synch = aa_rev >> 16;
}

/* reset le state, called by bt_generic_le and bt_follow_le() */
void reset_le() {
	le_set_access_address(0x8e89bed6);     // advertising channel access address
	le.crc_init  = 0x555555;               // advertising channel CRCInit
	le.crc_init_reversed = 0xAAAAAA;
	le.crc_verify = 0;
	le.last_packet = 0;

	le.link_state = LINK_INACTIVE;

	le.channel_idx = 0;
	le.channel_increment = 0;

	le.conn_epoch = 0;
	le.interval_timer = 0;
	le.conn_interval = 0;
	le.conn_interval = 0;
	le.conn_count = 0;

	le.win_size = 0;
	le.win_offset = 0;

	le.update_pending = 0;
	le.update_instant = 0;
	le.interval_update = 0;
	le.win_size_update = 0;
	le.win_offset_update = 0;

	do_hop = 0;
}

// reset LE Promisc state
void reset_le_promisc(void) {
	memset(&le_promisc, 0, sizeof(le_promisc));
	le_promisc.smallest_hop_interval = 0xffffffff;
}

/* generic le mode */
void bt_generic_le(u8 active_mode)
{
	u8 hold;
	int i, j;
	int8_t rssi, rssi_at_trigger;

	modulation = MOD_BT_LOW_ENERGY;
	mode = active_mode;

	reset_le();

	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	RXLED_CLR;

	usb_queue_init();
	dio_ssp_init();
	dma_init_rx_symbols();
	dio_ssp_start();
	cc2400_rx();

	cs_trigger_enable();

	hold = 0;

	while (requested_mode == active_mode) {
		if (requested_channel != 0) {
			cc2400_strobe(SRFOFF);
			while ((cc2400_status() & FS_LOCK)); // need to wait for unlock?

			/* Retune */
			cc2400_set(FSDIV, channel - 1);

			/* Wait for lock */
			cc2400_strobe(SFSON);
			while (!(cc2400_status() & FS_LOCK));

			/* RX mode */
			cc2400_strobe(SRX);

			requested_channel = 0;
		}

		if (do_hop) {
			hop();
		} else {
			TXLED_CLR;
		}

		RXLED_CLR;

		/* Wait for DMA. Meanwhile keep track of RSSI. */
		rssi_reset();
		rssi_at_trigger = INT8_MIN;
		while ((rx_tc == 0) && (rx_err == 0))
		{
			rssi = (int8_t)(cc2400_get(RSSI) >> 8);
			if (cs_trigger && (rssi_at_trigger == INT8_MIN)) {
				rssi = MAX(rssi,(cs_threshold_cur+54));
				rssi_at_trigger = rssi;
			}
			rssi_add(rssi);
		}

		if (rx_err) {
			status |= DMA_ERROR;
		}

		/* No DMA transfer? */
		if (!rx_tc)
			goto rx_continue;

		/* Missed a DMA trasfer? */
		if (rx_tc > 1)
			status |= DMA_OVERFLOW;

		rssi_iir_update(channel);

		/* Set squelch hold if there was either a CS trigger, squelch
		 * is disabled, or if the current rssi_max is above the same
		 * threshold. Currently, this is redundant, but allows for
		 * per-channel or other rssi triggers in the future. */
		if (cs_trigger || cs_no_squelch) {
			status |= CS_TRIGGER;
			hold = CS_HOLD_TIME;
			cs_trigger = 0;
		}

		if (rssi_max >= (cs_threshold_cur + 54)) {
			status |= RSSI_TRIGGER;
			hold = CS_HOLD_TIME;
		}

		/* Hold expired? Ignore data. */
		if (hold == 0) {
			goto rx_continue;
		}
		hold--;

		// copy the previously unpacked symbols to the front of the buffer
		memcpy(unpacked, unpacked + DMA_SIZE*8, DMA_SIZE*8);

		// unpack the new packet to the end of the buffer
		for (i = 0; i < DMA_SIZE; ++i) {
			/* output one byte for each received symbol (0x00 or 0x01) */
			for (j = 0; j < 8; ++j) {
				unpacked[DMA_SIZE*8 + i * 8 + j] = (idle_rxbuf[i] & 0x80) >> 7;
				idle_rxbuf[i] <<= 1;
			}
		}

		int ret = data_cb(unpacked);
		if (!ret) break;

	rx_continue:
		rx_tc = 0;
		rx_err = 0;
	}

	// disable USB interrupts
	ICER0 = ICER0_ICE_USB;

	// reset the radio completely
	cc2400_idle();
	dio_ssp_stop();
	cs_trigger_disable();
}


void bt_le_sync(u8 active_mode)
{
	int i;
	int8_t rssi;
	static int restart_jamming = 0;

	modulation = MOD_BT_LOW_ENERGY;
	mode = active_mode;

	le.link_state = LINK_LISTENING;

	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	RXLED_CLR;

	usb_queue_init();
	dio_ssp_init();
	dma_init_le();
	dio_ssp_start();

	cc2400_rx_sync(rbit(le.access_address)); // bit-reversed access address

	while (requested_mode == active_mode) {
		if (requested_channel != 0) {
			cc2400_strobe(SRFOFF);
			while ((cc2400_status() & FS_LOCK)); // need to wait for unlock?

			/* Retune */
			cc2400_set(FSDIV, channel - 1);

			/* Wait for lock */
			cc2400_strobe(SFSON);
			while (!(cc2400_status() & FS_LOCK));

			/* RX mode */
			cc2400_strobe(SRX);

			requested_channel = 0;
		}

		RXLED_CLR;

		/* Wait for DMA. Meanwhile keep track of RSSI. */
		rssi_reset();
		while ((rx_tc == 0) && (rx_err == 0) && (do_hop == 0) && requested_mode == active_mode)
			;

		rssi = (int8_t)(cc2400_get(RSSI) >> 8);
		rssi_min = rssi_max = rssi;

		if (requested_mode != active_mode) {
			goto cleanup;
		}

		if (rx_err) {
			status |= DMA_ERROR;
		}

		if (do_hop)
			goto rx_flush;

		/* No DMA transfer? */
		if (!rx_tc)
			continue;

		/////////////////////
		// process the packet

		uint32_t packet[48/4+1] = { 0, };
		u8 *p = (u8 *)packet;
		packet[0] = le.access_address;

		const uint32_t *whit = whitening_word[btle_channel_index(channel)];
		for (i = 0; i < 4; i+= 4) {
			uint32_t v = rxbuf1[i+0] << 24
					   | rxbuf1[i+1] << 16
					   | rxbuf1[i+2] << 8
					   | rxbuf1[i+3] << 0;
			packet[i/4+1] = rbit(v) ^ whit[i/4];
		}

		unsigned len = (p[5] & 0x3f) + 2;
		if (len > 39)
			goto rx_flush;

		// transfer the minimum number of bytes from the CC2400
		// this allows us enough time to resume RX for subsequent packets on the same channel
		unsigned total_transfers = ((len + 3) + 4 - 1) / 4;
		if (total_transfers < 11) {
			while (DMACC0DestAddr < (uint32_t)rxbuf1 + 4 * total_transfers && rx_err == 0)
				;
		} else { // max transfers? just wait till DMA's done
			while (DMACC0Config & DMACCxConfig_E && rx_err == 0)
				;
		}
		DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;

		// strobe SFSON to allow the resync to occur while we process the packet
		cc2400_strobe(SFSON);

		// unwhiten the rest of the packet
		for (i = 4; i < 44; i += 4) {
			uint32_t v = rxbuf1[i+0] << 24
					   | rxbuf1[i+1] << 16
					   | rxbuf1[i+2] << 8
					   | rxbuf1[i+3] << 0;
			packet[i/4+1] = rbit(v) ^ whit[i/4];
		}

		if (le.crc_verify) {
			u32 calc_crc = btle_crcgen_lut(le.crc_init_reversed, p + 4, len);
			u32 wire_crc = (p[4+len+2] << 16)
						 | (p[4+len+1] << 8)
						 | (p[4+len+0] << 0);
			if (calc_crc != wire_crc) // skip packets with a bad CRC
				goto rx_flush;
		}


		RXLED_SET;
		packet_cb((uint8_t *)packet);

		// disable USB interrupts while we touch USB data structures
		ICER0 = ICER0_ICE_USB;
		enqueue(LE_PACKET, (uint8_t *)packet);
		ISER0 = ISER0_ISE_USB;

		le.last_packet = CLK100NS;

	rx_flush:
		// this might happen twice, but it's safe to do so
		cc2400_strobe(SFSON);

		// flush any excess bytes from the SSP's buffer
		DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;
		while (SSP1SR & SSPSR_RNE) {
			u8 tmp = (u8)DIO_SSP_DR;
		}

		// timeout - FIXME this is an ugly hack
		u32 now = CLK100NS;
		if (now < le.last_packet)
			now += 3276800000; // handle rollover
		if  ( // timeout
			((le.link_state == LINK_CONNECTED || le.link_state == LINK_CONN_PENDING)
			&& (now - le.last_packet > 50000000))
			// jam finished
			|| (le_jam_count == 1)
			)
		{
			reset_le();
			le_jam_count = 0;
			TXLED_CLR;

			if (jam_mode == JAM_ONCE) {
				jam_mode = JAM_NONE;
				requested_mode = MODE_IDLE;
				goto cleanup;
			}

			// go back to promisc if the connection dies
			if (active_mode == MODE_BT_PROMISC_LE)
				goto cleanup;

			le.link_state = LINK_LISTENING;

			cc2400_strobe(SRFOFF);
			while ((cc2400_status() & FS_LOCK));

			/* Retune */
			channel = le_adv_channel != 0 ? le_adv_channel : 2402;
			restart_jamming = 1;
		}

		cc2400_set(SYNCL, le.syncl);
		cc2400_set(SYNCH, le.synch);

		if (do_hop)
			hop();

		// ♪ you can jam but you keep turning off the light ♪
		if (le_jam_count > 0) {
#ifdef TX_ENABLE
			le_jam();
#endif
			--le_jam_count;
		} else {
			/* RX mode */
			dma_init_le();
			dio_ssp_start();

			if (restart_jamming) {
				cc2400_rx_sync(rbit(le.access_address));
				restart_jamming = 0;
			} else {
				// wait till we're in FSLOCK before strobing RX
				while (!(cc2400_status() & FS_LOCK));
				cc2400_strobe(SRX);
			}
		}

		rx_tc = 0;
		rx_err = 0;
	}

cleanup:

	// disable USB interrupts
	ICER0 = ICER0_ICE_USB;

	// reset the radio completely
	cc2400_idle();
	dio_ssp_stop();
	cs_trigger_disable();
}



/* low energy connection following
 * follows a known AA around */
int cb_follow_le() {
	int i, j, k;
	int idx = whitening_index[btle_channel_index(channel)];

	u32 access_address = 0;
	for (i = 0; i < 31; ++i) {
		access_address >>= 1;
		access_address |= (unpacked[i] << 31);
	}

	for (i = 31; i < DMA_SIZE * 8 + 32; i++) {
		access_address >>= 1;
		access_address |= (unpacked[i] << 31);
		if (access_address == le.access_address) {
			for (j = 0; j < 46; ++j) {
				u8 byte = 0;
				for (k = 0; k < 8; k++) {
					int offset = k + (j * 8) + i - 31;
					if (offset >= DMA_SIZE*8*2) break;
					int bit = unpacked[offset];
					if (j >= 4) { // unwhiten data bytes
						bit ^= whitening[idx];
						idx = (idx + 1) % sizeof(whitening);
					}
					byte |= bit << k;
				}
				idle_rxbuf[j] = byte;
			}

			// verify CRC
			if (le.crc_verify) {
				int len		 = (idle_rxbuf[5] & 0x3f) + 2;
				u32 calc_crc = btle_crcgen_lut(le.crc_init_reversed, (uint8_t*)idle_rxbuf + 4, len);
				u32 wire_crc = (idle_rxbuf[4+len+2] << 16)
							 | (idle_rxbuf[4+len+1] << 8)
							 |  idle_rxbuf[4+len+0];
				if (calc_crc != wire_crc) // skip packets with a bad CRC
					break;
			}

			// send to PC
			enqueue(LE_PACKET, (uint8_t*)idle_rxbuf);
			RXLED_SET;

			packet_cb((uint8_t*)idle_rxbuf);

			break;
		}
	}

	return 1;
}

/**
 * Called when we receive a packet in connection following mode.
 */
void connection_follow_cb(u8 *packet) {
	int i;
	u32 aa = 0;

#define ADV_ADDRESS_IDX 0
#define HEADER_IDX 4
#define DATA_LEN_IDX 5
#define DATA_START_IDX 6

	// u8 *adv_addr = &packet[ADV_ADDRESS_IDX];
	u8 header = packet[HEADER_IDX];
	u8 *data_len = &packet[DATA_LEN_IDX];
	u8 *data = &packet[DATA_START_IDX];
	// u8 *crc = &packet[DATA_START_IDX + *data_len];

	if (le.link_state == LINK_CONN_PENDING) {
		// We received a packet in the connection pending state, so now the device *should* be connected
		le.link_state = LINK_CONNECTED;
		le.conn_epoch = clkn;
		le.interval_timer = le.conn_interval - 1;
		le.conn_count = 0;
		le.update_pending = 0;

		// hue hue hue
		if (jam_mode != JAM_NONE)
			le_jam_count = JAM_COUNT_DEFAULT;

	} else if (le.link_state == LINK_CONNECTED) {
		u8 llid =  header & 0x03;

		// Apply any connection parameter update if necessary
		if (le.update_pending && le.conn_count == le.update_instant) {
			// This is the first packet received in the connection interval for which the new parameters apply
			le.conn_epoch = clkn;
			le.conn_interval = le.interval_update;
			le.interval_timer = le.interval_update - 1;
			le.win_size = le.win_size_update;
			le.win_offset = le.win_offset_update;
			le.update_pending = 0;
		}

		if (llid == 0x03 && data[0] == 0x00) {
			// This is a CONNECTION_UPDATE_REQ.
			// The host is changing the connection parameters.
			le.win_size_update = packet[7];
			le.win_offset_update = packet[8] + ((u16)packet[9] << 8);
			le.interval_update = packet[10] + ((u16)packet[11] << 8);
			le.update_instant = packet[16] + ((u16)packet[17] << 8);
			if (le.update_instant - le.conn_count < 32767)
				le.update_pending = 1;
		}

	} else if (le.link_state == LINK_LISTENING) {
		u8 pkt_type = packet[4] & 0x0F;
		if (pkt_type == 0x05) {
			uint16_t conn_interval;

			// ignore packets with incorrect length
			if (*data_len != 34)
				return;

			// conn interval must be [7.5 ms, 4.0s] in units of 1.25 ms
			conn_interval = (packet[29] << 8) | packet[28];
			if (conn_interval < 6 || conn_interval > 3200)
				return;

			// This is a connect packet
			// if we have a target, see if InitA or AdvA matches
			if (le.target_set &&
				memcmp(le.target, &packet[6], 6) &&  // Target address doesn't match Initiator.
				memcmp(le.target, &packet[12], 6)) {  // Target address doesn't match Advertiser.
				return;
			}

			le.link_state = LINK_CONN_PENDING;
			le.crc_verify = 0; // we will drop many packets if we attempt to filter by CRC

			for (i = 0; i < 4; ++i)
				aa |= packet[18+i] << (i*8);
			le_set_access_address(aa);

#define CRC_INIT (2+4+6+6+4)
			le.crc_init = (packet[CRC_INIT+2] << 16)
						| (packet[CRC_INIT+1] << 8)
						|  packet[CRC_INIT+0];
			le.crc_init_reversed = rbit(le.crc_init);

#define WIN_SIZE (2+4+6+6+4+3)
			le.win_size = packet[WIN_SIZE];

#define WIN_OFFSET (2+4+6+6+4+3+1)
			le.win_offset = packet[WIN_OFFSET];

#define CONN_INTERVAL (2+4+6+6+4+3+1+2)
			le.conn_interval = (packet[CONN_INTERVAL+1] << 8)
							 |  packet[CONN_INTERVAL+0];

#define CHANNEL_INC (2+4+6+6+4+3+1+2+2+2+2+5)
			le.channel_increment = packet[CHANNEL_INC] & 0x1f;
			le.channel_idx = le.channel_increment;

			// Hop to the initial channel immediately
			do_hop = 1;
		}
	}
}

void le_phy_main(void);
void bt_follow_le() {
	le_phy_main();

	/* old method
	reset_le();
	packet_cb = connection_follow_cb;
	bt_le_sync(MODE_BT_FOLLOW_LE);
	*/
}

// issue state change message
void le_promisc_state(u8 type, void *data, unsigned len) {
	u8 buf[50] = { 0, };
	if (len > 49)
		len = 49;

	buf[0] = type;
	memcpy(&buf[1], data, len);
	enqueue(LE_PROMISC, (uint8_t*)buf);
}

// divide, rounding to the nearest integer: round up at 0.5.
#define DIVIDE_ROUND(N, D) ((N) + (D)/2) / (D)

void promisc_recover_hop_increment(u8 *packet) {
	static u32 first_ts = 0;
	if (channel == 2404) {
		first_ts = CLK100NS;
		hop_direct_channel = 2406;
		do_hop = 1;
	} else if (channel == 2406) {
		u32 second_ts = CLK100NS;
		if (second_ts < first_ts)
			second_ts += 3276800000; // handle rollover
		// Number of channels hopped between previous and current timestamp.
		u32 channels_hopped = DIVIDE_ROUND(second_ts - first_ts,
										   le.conn_interval * LE_BASECLK);
		if (channels_hopped < 37) {
			// Get the hop increment based on the number of channels hopped.
			le.channel_increment = hop_interval_lut[channels_hopped];
			le.interval_timer = le.conn_interval / 2;
			le.conn_count = 0;
			le.conn_epoch = 0;
			do_hop = 0;
			// Move on to regular connection following.
			le.channel_idx = (1 + le.channel_increment) % 37;
			le.link_state = LINK_CONNECTED;
			le.crc_verify = 0;
			hop_mode = HOP_BTLE;
			packet_cb = connection_follow_cb;
			le_promisc_state(3, &le.channel_increment, 1);

			if (jam_mode != JAM_NONE)
				le_jam_count = JAM_COUNT_DEFAULT;

			return;
		}
		hop_direct_channel = 2404;
		do_hop = 1;
	}
	else {
		hop_direct_channel = 2404;
		do_hop = 1;
	}
}

void promisc_recover_hop_interval(u8 *packet) {
	static u32 prev_clk = 0;

	u32 cur_clk = CLK100NS;
	if (cur_clk < prev_clk)
		cur_clk += 3267800000; // handle rollover
	u32 clk_diff = cur_clk - prev_clk;
	u16 obsv_hop_interval; // observed hop interval

	// probably consecutive data packets on the same channel
	if (clk_diff < 2 * LE_BASECLK)
		return;

	if (clk_diff < le_promisc.smallest_hop_interval)
		le_promisc.smallest_hop_interval = clk_diff;

	obsv_hop_interval = DIVIDE_ROUND(le_promisc.smallest_hop_interval, 37 * LE_BASECLK);

	if (le.conn_interval == obsv_hop_interval) {
		// 5 consecutive hop intervals: consider it legit and move on
		++le_promisc.consec_intervals;
		if (le_promisc.consec_intervals == 5) {
			packet_cb = promisc_recover_hop_increment;
			hop_direct_channel = 2404;
			hop_mode = HOP_DIRECT;
			do_hop = 1;
			le_promisc_state(2, &le.conn_interval, 2);
		}
	} else {
		le.conn_interval = obsv_hop_interval;
		le_promisc.consec_intervals = 0;
	}

	prev_clk = cur_clk;
}

void promisc_follow_cb(u8 *packet) {
	int i;

	// get the CRCInit
	if (!le.crc_verify && packet[4] == 0x01 && packet[5] == 0x00) {
		u32 crc = (packet[8] << 16) | (packet[7] << 8) | packet[6];

		le.crc_init = btle_reverse_crc(crc, packet + 4, 2);
		le.crc_init_reversed = 0;
		for (i = 0; i < 24; ++i)
			le.crc_init_reversed |= ((le.crc_init >> i) & 1) << (23 - i);

		le.crc_verify = 1;
		packet_cb = promisc_recover_hop_interval;
		le_promisc_state(1, &le.crc_init, 3);
	}
}

// called when we see an AA, add it to the list
void see_aa(u32 aa) {
	int i, max = -1, killme = -1;
	for (i = 0; i < AA_LIST_SIZE; ++i)
		if (le_promisc.active_aa[i].aa == aa) {
			++le_promisc.active_aa[i].count;
			return;
		}

	// evict someone
	for (i = 0; i < AA_LIST_SIZE; ++i)
		if (le_promisc.active_aa[i].count < max || max < 0) {
			killme = i;
			max = le_promisc.active_aa[i].count;
		}

	le_promisc.active_aa[killme].aa = aa;
	le_promisc.active_aa[killme].count = 1;
}

/* le promiscuous mode */
int cb_le_promisc(char *unpacked) {
	int i, j, k;
	int idx;

	// empty data PDU: 01 00
	char desired[4][16] = {
		{ 1, 0, 0, 0, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, },
		{ 1, 0, 0, 1, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, },
		{ 1, 0, 1, 0, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, },
		{ 1, 0, 1, 1, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, },
	};

	for (i = 0; i < 4; ++i) {
		idx = whitening_index[btle_channel_index(channel)];

		// whiten the desired data
		for (j = 0; j < (int)sizeof(desired[i]); ++j) {
			desired[i][j] ^= whitening[idx];
			idx = (idx + 1) % sizeof(whitening);
		}
	}

	// then look for that bitsream in our receive buffer
	for (i = 32; i < (DMA_SIZE*8*2 - 32 - 16); i++) {
		int ok[4] = { 1, 1, 1, 1 };
		int matching = -1;

		for (j = 0; j < 4; ++j) {
			for (k = 0; k < (int)sizeof(desired[j]); ++k) {
				if (unpacked[i+k] != desired[j][k]) {
					ok[j] = 0;
					break;
				}
			}
		}

		// see if any match
		for (j = 0; j < 4; ++j) {
			if (ok[j]) {
				matching = j;
				break;
			}
		}

		// skip if no match
		if (matching < 0)
			continue;

		// found a match! unwhiten it and send it home
		idx = whitening_index[btle_channel_index(channel)];
		for (j = 0; j < 4+3+3; ++j) {
			u8 byte = 0;
			for (k = 0; k < 8; k++) {
				int offset = k + (j * 8) + i - 32;
				if (offset >= DMA_SIZE*8*2) break;
				int bit = unpacked[offset];
				if (j >= 4) { // unwhiten data bytes
					bit ^= whitening[idx];
					idx = (idx + 1) % sizeof(whitening);
				}
				byte |= bit << k;
			}
			idle_rxbuf[j] = byte;
		}

		u32 aa = (idle_rxbuf[3] << 24) |
				 (idle_rxbuf[2] << 16) |
				 (idle_rxbuf[1] <<  8) |
				 (idle_rxbuf[0]);
		see_aa(aa);

		enqueue(LE_PACKET, (uint8_t*)idle_rxbuf);

	}

	// once we see an AA 5 times, start following it
	for (i = 0; i < AA_LIST_SIZE; ++i) {
		if (le_promisc.active_aa[i].count > 3) {
			le_set_access_address(le_promisc.active_aa[i].aa);
			data_cb = cb_follow_le;
			packet_cb = promisc_follow_cb;
			le.crc_verify = 0;
			le_promisc_state(0, &le.access_address, 4);
			// quit using the old stuff and switch to sync mode
			return 0;
		}
	}

	return 1;
}

void bt_promisc_le() {
	while (requested_mode == MODE_BT_PROMISC_LE) {
		reset_le_promisc();

		// jump to a random data channel and turn up the squelch
		if ((channel & 1) == 1)
			channel = 2440;

		// if the PC hasn't given us AA, determine by listening
		if (!le.target_set) {
			// cs_threshold_req = -80;
			cs_threshold_calc_and_set(channel);
			data_cb = cb_le_promisc;
			bt_generic_le(MODE_BT_PROMISC_LE);
		}

		// could have got mode change in middle of above
		if (requested_mode != MODE_BT_PROMISC_LE)
			break;

		le_promisc_state(0, &le.access_address, 4);
		packet_cb = promisc_follow_cb;
		le.crc_verify = 0;
		bt_le_sync(MODE_BT_PROMISC_LE);
	}
}

#ifdef TX_ENABLE
void bt_slave_le() {
	u32 calc_crc;
	int i;
	uint8_t adv_ind[32] = { 0x00, };
	uint8_t adv_ind_len;

	if (le_adv_len > LE_ADV_MAX_LEN) {
		requested_mode = MODE_IDLE;
		return;
	}

	adv_ind_len = 6 + le_adv_len;
	adv_ind[1] = adv_ind_len;

	// copy the user-specified mac address
	for (i = 0; i < 6; ++i)
		adv_ind[i+2] = slave_mac_address[5-i];

	// copy in the adv data
	memcpy(adv_ind + 2 + 6, le_adv_data, le_adv_len);

	// total: 2 + 6 + le_adv_len
	adv_ind_len += 2;

	calc_crc = btle_calc_crc(le.crc_init_reversed, adv_ind, adv_ind_len);
	adv_ind[adv_ind_len + 0] = (calc_crc >>  0) & 0xff;
	adv_ind[adv_ind_len + 1] = (calc_crc >>  8) & 0xff;
	adv_ind[adv_ind_len + 2] = (calc_crc >> 16) & 0xff;

	clkn_start();

	// enable USB interrupts due to busy waits
	ISER0 = ISER0_ISE_USB;

	// spam advertising packets
	while (requested_mode == MODE_BT_SLAVE_LE) {
		le_transmit(0x8e89bed6, adv_ind_len+3, adv_ind);
		msleep(100);
	}

	// disable USB interrupts
	ICER0 = ICER0_ICE_USB;
}
#endif

void rx_generic_sync(void) {
	u8 len = 32;
	u8 buf[len+4];
	u16 reg_val;

	/* Put syncword at start of buffer
	 * DGS: fix this later, we don't know number of syncword bytes, etc
	 */
	reg_val = cc2400_get(SYNCH);
	buf[0] = (reg_val >> 8) & 0xFF;
	buf[1] = reg_val & 0xFF;
	reg_val = cc2400_get(SYNCL);
	buf[2] = (reg_val >> 8) & 0xFF;
	buf[3] = reg_val & 0xFF;

	usb_queue_init();
	clkn_start();

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	RXLED_SET;
#ifdef UBERTOOTH_ONE
		PAEN_SET;
		HGM_SET;
#endif
	while (1) {
		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		cc2400_strobe(SRX);
		USRLED_CLR;
		while (!(cc2400_status() & SYNC_RECEIVED));
		USRLED_SET;

		cc2400_fifo_read(len, buf+4);
		enqueue(BR_PACKET, buf);
		handle_usb(clkn);
	}
}

void rx_generic(void) {
	// Check for packet mode
	if(cc2400_get(GRMDM) && 0x0400) {
		rx_generic_sync();
	} else {
		modulation = MOD_NONE;
		bt_stream_rx();
	}
}

#ifdef TX_ENABLE
void tx_generic(void) {
	u16 synch, syncl;
	u8 prev_mode = mode;

	mode = MODE_TX_GENERIC;

	// Save existing syncword
	synch = cc2400_get(SYNCH);
	syncl = cc2400_get(SYNCL);

	cc2400_set(SYNCH, tx_pkt.synch);
	cc2400_set(SYNCL, tx_pkt.syncl);
	cc2400_set(MDMCTRL, 0x0057);
	cc2400_set(MDMTST0, 0x134b);
	cc2400_set(GRMDM, 0x0f61);
	cc2400_set(FSDIV, tx_pkt.channel);
	cc2400_set(FREND, tx_pkt.pa_level);

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
#ifdef UBERTOOTH_ONE
		PAEN_SET;
#endif
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);

	cc2400_fifo_write(tx_pkt.length, tx_pkt.data);
	cc2400_strobe(STX);

	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	TXLED_CLR;

	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
#endif

	// Restore state
	cc2400_set(SYNCH, synch);
	cc2400_set(SYNCL, syncl);
	requested_mode = prev_mode;
}
#endif

/* spectrum analysis */
void specan()
{
	u16 f;
	u8 i = 0;
	u8 buf[DMA_SIZE];

	RXLED_SET;

	usb_queue_init();
	clkn_start();

#ifdef UBERTOOTH_ONE
	PAEN_SET;
	//HGM_SET;
#endif
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	//FIXME maybe set RSSI.RSSI_FILT
	while (!(cc2400_status() & XOSC16M_STABLE));
	while ((cc2400_status() & FS_LOCK));

	while (requested_mode == MODE_SPECAN) {
		for (f = low_freq; f < high_freq + 1; f++) {
			cc2400_set(FSDIV, f - 1);
			cc2400_strobe(SFSON);
			while (!(cc2400_status() & FS_LOCK));
			cc2400_strobe(SRX);

			/* give the CC2400 time to acquire RSSI reading */
			volatile u32 j = 500; while (--j); //FIXME crude delay
			buf[3 * i] = (f >> 8) & 0xFF;
			buf[(3 * i) + 1] = f  & 0xFF;
			buf[(3 * i) + 2] = cc2400_get(RSSI) >> 8;
			i++;
			if (i == 16) {
				enqueue(SPECAN, buf);
				i = 0;

				handle_usb(clkn);
			}

			cc2400_strobe(SRFOFF);
			while ((cc2400_status() & FS_LOCK));
		}
	}
	RXLED_CLR;
}

/* LED based spectrum analysis */
void led_specan()
{
	int8_t lvl;
	u8 i = 0;
	u16 channels[3] = {2412, 2437, 2462};
	//void (*set[3]) = {TXLED_SET, RXLED_SET, USRLED_SET};
	//void (*clr[3]) = {TXLED_CLR, RXLED_CLR, USRLED_CLR};

#ifdef UBERTOOTH_ONE
	PAEN_SET;
	//HGM_SET;
#endif
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	cc2400_set(RSSI,    0x00F1); // RSSI Sample over 2 symbols

	while (!(cc2400_status() & XOSC16M_STABLE));
	while ((cc2400_status() & FS_LOCK));

	while (requested_mode == MODE_LED_SPECAN) {
		cc2400_set(FSDIV, channels[i] - 1);
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));
		cc2400_strobe(SRX);

		/* give the CC2400 time to acquire RSSI reading */
		volatile u32 j = 500; while (--j); //FIXME crude delay
		lvl = (int8_t)((cc2400_get(RSSI) >> 8) & 0xff);
		if (lvl > rssi_threshold) {
			switch (i) {
				case 0:
					TXLED_SET;
					break;
				case 1:
					RXLED_SET;
					break;
				case 2:
					USRLED_SET;
					break;
			}
		}
		else {
			switch (i) {
				case 0:
					TXLED_CLR;
					break;
				case 1:
					RXLED_CLR;
					break;
				case 2:
					USRLED_CLR;
					break;
			}
		}

		i = (i+1) % 3;

		handle_usb(clkn);

		cc2400_strobe(SRFOFF);
		while ((cc2400_status() & FS_LOCK));
	}
}

int main()
{
	// enable all fault handlers (see fault.c)
	SCB_SHCSR = (1 << 18) | (1 << 17) | (1 << 16);

	ubertooth_init();
	clkn_init();
	ubertooth_usb_init(vendor_request_handler);
	cc2400_idle();
	dma_poweron();

	debug_uart_init(0);
	debug_printf("\n\n****UBERTOOTH BOOT****\n%s\n", compile_info);

	while (1) {
		handle_usb(clkn);
		if(requested_mode != mode) {
			switch (requested_mode) {
				case MODE_RESET:
					/* Allow time for the USB command to return correctly */
					wait(1);
					reset();
					break;
				case MODE_AFH:
					mode = MODE_AFH;
					bt_stream_rx();
					break;
				case MODE_RX_SYMBOLS:
					mode = MODE_RX_SYMBOLS;
					bt_stream_rx();
					break;
#ifdef TX_ENABLE
				case MODE_TX_SYMBOLS:
					mode = MODE_TX_SYMBOLS;
					br_transmit();
					break;
#endif
				case MODE_BT_FOLLOW:
					mode = MODE_BT_FOLLOW;
					bt_stream_rx();
					break;
				case MODE_BT_FOLLOW_LE:
					mode = MODE_BT_FOLLOW_LE;
					bt_follow_le();
					break;
				case MODE_BT_PROMISC_LE:
					bt_promisc_le();
					break;
#ifdef TX_ENABLE
				case MODE_BT_SLAVE_LE:
					bt_slave_le();
					break;
				case MODE_TX_TEST:
					mode = MODE_TX_TEST;
					cc2400_txtest(&modulation, &channel);
					break;
				case MODE_RANGE_TEST:
					mode = MODE_RANGE_TEST;
					cc2400_rangetest(&channel);
					requested_mode = MODE_IDLE;
					break;
				case MODE_REPEATER:
					mode = MODE_REPEATER;
					cc2400_repeater(&channel);
					break;
#endif
				case MODE_SPECAN:
					specan();
					break;
				case MODE_LED_SPECAN:
					led_specan();
					break;
				case MODE_EGO:
					mode = MODE_EGO;
					ego_main(ego_mode);
					break;
				case MODE_RX_GENERIC:
					mode = MODE_RX_GENERIC;
					rx_generic();
					break;
#ifdef TX_ENABLE
				case MODE_TX_GENERIC:
					tx_generic();
					break;
#endif
				case MODE_XMAS:
					mode = MODE_XMAS;
					xmas_main();
					break;
				case MODE_IDLE:
					cc2400_idle();
					break;
				default:
					/* This is really an error state, but what can you do? */
					break;
			}
		}
	}
}
