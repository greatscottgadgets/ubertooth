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
#include "bluetooth.h"
#include "bluetooth_le.h"
#include "cc2400_rangetest.h"

#define MIN(x,y)	((x)<(y)?(x):(y))
#define MAX(x,y)	((x)>(y)?(x):(y))

/* build info */
const char compile_info[] =
	"ubertooth " GIT_REVISION " (" COMPILE_BY "@" COMPILE_HOST ") " TIMESTAMP;

/*
 * CLK100NS is a free-running clock with a period of 100 ns.  It resets every
 * 2^15 * 10^5 cycles (about 5.5 minutes) - computed from clkn and timer0 (T0TC)
 *
 * clkn is the native (local) clock as defined in the Bluetooth specification.
 * It advances 3200 times per second.  Two clkn periods make a Bluetooth time
 * slot.
 */

volatile u32 clkn;                       // clkn 3200 Hz counter
#define CLK100NS (3125*(clkn & 0xfffff) + T0TC)
#define LE_BASECLK (12500)               // 1.25 ms in units of 100ns

#define HOP_NONE      0
#define HOP_SWEEP     1
#define HOP_BLUETOOTH 2
#define HOP_BTLE      3
#define HOP_DIRECT    4

#define CS_THRESHOLD_DEFAULT (uint8_t)(-120)
#define CS_HOLD_TIME  2                     // min pkts to send on trig (>=1)
u8 hop_mode = HOP_NONE;
volatile u8 do_hop = 0;                     // set by timer interrupt

u8 cs_no_squelch = 0;                       // rx all packets if set
int8_t cs_threshold_req=CS_THRESHOLD_DEFAULT; // requested CS threshold in dBm
int8_t cs_threshold_cur=CS_THRESHOLD_DEFAULT; // current CS threshold in dBm
volatile u8 cs_trigger;                     // set by intr on P2.2 falling (CS)
volatile u8 keepalive_trigger;              // set by timer 1/s
volatile u32 cs_timestamp;                  // CLK100NS at time of cs_trigger
u16 hop_direct_channel = 0;                 // for hopping directly to a channel
int clock_trim = 0;                         // to counteract clock drift
u32 idle_buf_clkn_high;
u32 active_buf_clkn_high;
u32 idle_buf_clk100ns;
u32 active_buf_clk100ns;
u16 idle_buf_channel = 0;
u16 active_buf_channel = 0;
u8 slave_mac_address[6] = { 0, };

/* DMA buffers */
u8 rxbuf1[DMA_SIZE];
u8 rxbuf2[DMA_SIZE];

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
u8 *active_rxbuf = &rxbuf1[0];
u8 *idle_rxbuf = &rxbuf2[0];

le_state_t le = {
	.access_address = 0x8e89bed6,           // advertising channel access address
	.synch = 0x6b7d,                        // bit-reversed adv channel AA
	.syncl = 0x9171,
	.crc_init  = 0x555555,                  // advertising channel CRCInit
	.crc_init_reversed = 0xAAAAAA,
	.crc_verify = 1,

	.link_state = LINK_INACTIVE,
	.conn_epoch = 0,
	.target_set = 0,
	.last_packet = 0,
};

/* set LE access address */
static void le_set_access_address(u32 aa);

typedef int (*data_cb_t)(char *);
data_cb_t data_cb = NULL;

typedef void (*packet_cb_t)(u8 *);
packet_cb_t packet_cb = NULL;

/* Moving average (IIR) of average RSSI of packets as scaled integers (x256). */
int16_t rssi_iir[79] = {0};
#define RSSI_IIR_ALPHA 3       // 3/256 = .012

/*
 * CLK_TUNE_TIME is the duration in units of 100 ns that we reserve for tuning
 * the radio while frequency hopping.  We start the tuning process
 * CLK_TUNE_TIME * 100 ns prior to the start of an upcoming time slot.
*/
#define CLK_TUNE_TIME   2250

/* Unpacked symbol buffers (two rxbufs) */
char unpacked[DMA_SIZE*8*2];

volatile u8 mode = MODE_IDLE;
volatile u8 requested_mode = MODE_IDLE;
volatile u8 modulation = MOD_BT_BASIC_RATE;
volatile u16 channel = 2441;
volatile u16 requested_channel = 0;
volatile u16 saved_request = 0;
volatile u16 low_freq = 2400;
volatile u16 high_freq = 2483;
volatile int8_t rssi_threshold = -30;  // -54dBm - 30 = -84dBm

/* DMA linked list items */
typedef struct {
	u32 src;
	u32 dest;
	u32 next_lli;
	u32 control;
} dma_lli;

dma_lli rx_dma_lli1;
dma_lli rx_dma_lli2;

dma_lli le_dma_lli[11]; // 11 x 4 bytes

/* rx terminal count and error interrupt counters */
volatile u32 rx_tc;
volatile u32 rx_err;

/* number of rx USB packets to send */
volatile u32 rx_pkts = 0;

/* status information byte */
volatile u8 status = 0;

#define DMA_OVERFLOW  0x01
#define DMA_ERROR     0x02
#define FIFO_OVERFLOW 0x04
#define CS_TRIGGER    0x08
#define RSSI_TRIGGER  0x10

/*
 * RSSI
 */
int8_t rssi_max;
int8_t rssi_min;
uint8_t rssi_count = 0;
int32_t rssi_sum = 0;

static void rssi_reset(void)
{
	rssi_count = 0;
	rssi_sum = 0;
	rssi_max = INT8_MIN;
	rssi_min = INT8_MAX;
}

static void rssi_add(int8_t v)
{
	rssi_max = (v > rssi_max) ? v : rssi_max;
	rssi_min = (v < rssi_min) ? v : rssi_min;
	rssi_sum += ((int32_t)v * 256);  // scaled int math (x256)
	rssi_count += 1;
}

/* For sweep mode, update IIR per channel. Otherwise, use single value. */
static void rssi_iir_update(void)
{
	int i;
	int32_t avg;
	int32_t rssi_iir_acc;

	/* Use array to track 79 Bluetooth channels, or just first
	 * slot of array if not sweeping. */
	if (hop_mode > 0)
		i = channel - 2402;
	else
		i = 0;

	// IIR using scaled int math (x256)
	if (rssi_count != 0)
		avg = (rssi_sum  + 128) / rssi_count;
	else
		avg = 0; // really an error
	rssi_iir_acc = rssi_iir[i] * (256-RSSI_IIR_ALPHA);
	rssi_iir_acc += avg * RSSI_IIR_ALPHA;
	rssi_iir[i] = (int16_t)((rssi_iir_acc + 128) / 256);
}

#define CS_SAMPLES_1 1
#define CS_SAMPLES_2 2
#define CS_SAMPLES_4 3
#define CS_SAMPLES_8 4
/* Set CC2400 carrier sense threshold and store value to
 * global. CC2400 RSSI is determined by 54dBm + level. CS threshold is
 * in 4dBm steps, so the provided level is rounded to the nearest
 * multiple of 4 by adding 56. Useful range is -100 to -20. */
static void cs_threshold_set(int8_t level, u8 samples)
{
	level = MIN(MAX(level,-120),(-20));
	cc2400_set(RSSI, (uint8_t)((level + 56) & (0x3f << 2)) | (samples&3));
	cs_threshold_cur = level;
	cs_no_squelch = (level <= -120);
}

static int enqueue(u8 type, u8 *buf)
{
	usb_pkt_rx *f = usb_enqueue();

	/* fail if queue is full */
	if (f == NULL) {
		status |= FIFO_OVERFLOW;
		return 0;
	}

	f->pkt_type = type;
	f->clkn_high = idle_buf_clkn_high;
	f->clk100ns = idle_buf_clk100ns;
	f->channel = idle_buf_channel - 2402;
	f->rssi_min = rssi_min;
	f->rssi_max = rssi_max;
	if (hop_mode != HOP_NONE)
		f->rssi_avg = (int8_t)((rssi_iir[idle_buf_channel-2402] + 128)/256);
	else
		f->rssi_avg = (int8_t)((rssi_iir[0] + 128)/256);
	f->rssi_count = rssi_count;

	USRLED_SET;

	// Unrolled copy of 50 bytes from buf to fifo
	u32 *p1 = (u32 *)f->data;
	u32 *p2 = (u32 *)buf;
	p1[0] = p2[0];
	p1[1] = p2[1];
	p1[2] = p2[2];
	p1[3] = p2[3];
	p1[4] = p2[4];
	p1[5] = p2[5];
	p1[6] = p2[6];
	p1[7] = p2[7];
	p1[8] = p2[8];
	p1[9] = p2[9];
	p1[10] = p2[10];
	p1[11] = p2[11];
	/* Avoid gcc warning about strict-aliasing */
	u16 *p3 = (u16 *)f->data;
	u16 *p4 = (u16 *)buf;
	p3[24] = p4[24];

	f->status = status;
	status = 0;

	return 1;
}

static void cs_threshold_calc_and_set(void)
{
	int8_t level;

	/* If threshold is max/avg based (>0), reset here while rx is
	 * off.  TODO - max-to-iir only works in SWEEP mode, where the
	 * channel is known to be in the BT band, i.e., rssi_iir has a
	 * value for it. */
	if ((hop_mode > 0) && (cs_threshold_req > 0)) {
		int8_t rssi = (int8_t)((rssi_iir[channel-2402] + 128)/256);
		level = rssi - 54 + cs_threshold_req;
	}
	else {
		level = cs_threshold_req;
	}
	cs_threshold_set(level, CS_SAMPLES_4);
}

/* CS comes from CC2400 GIO6, which is LPC P2.2, active low. GPIO
 * triggers EINT3, which could be used for other things (but is not
 * currently). TODO - EINT3 should be managed globally, not turned on
 * and off here. */
static void cs_trigger_enable(void)
{
	cs_trigger = 0;
	ISER0 = ISER0_ISE_EINT3;
	IO2IntClr = PIN_GIO6;      // Clear pending
	IO2IntEnF |= PIN_GIO6;     // Enable port 2.2 falling (CS active low)
}

static void cs_trigger_disable(void)
{
	IO2IntEnF &= ~PIN_GIO6;    // Disable port 2.2 falling (CS active low)
	IO2IntClr = PIN_GIO6;      // Clear pending
	ICER0 = ICER0_ICE_EINT3;
	cs_trigger = 0;
}

static int vendor_request_handler(u8 request, u16 *request_params, u8 *data, int *data_len)
{
	u32 command[5];
	u32 result[5];
	u64 ac_copy;
	int i; // loop counter
	u32 clock;
	int clock_offset;
	u8 length; // string length
	usb_pkt_rx *p = NULL;
	u16 reg_val;

	switch (request) {

	case UBERTOOTH_PING:
		*data_len = 0;
		break;

	case UBERTOOTH_RX_SYMBOLS:
		requested_mode = MODE_RX_SYMBOLS;
		rx_pkts += request_params[0];
		if (rx_pkts == 0)
			rx_pkts = 0xFFFFFFFF;
		*data_len = 0;
		break;

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
		command[0] = 54; /* read part number */
		iap_entry(command, result);
		data[0] = result[0] & 0xFF; /* status */
		data[1] = result[1] & 0xFF;
		data[2] = (result[1] >> 8) & 0xFF;
		data[3] = (result[1] >> 16) & 0xFF;
		data[4] = (result[1] >> 24) & 0xFF;
		*data_len = 5;
		break;

	case UBERTOOTH_RESET:
		requested_mode = MODE_RESET;
		break;

	case UBERTOOTH_GET_SERIAL:
		command[0] = 58; /* read device serial number */
		iap_entry(command, result);
		data[0] = result[0] & 0xFF; /* status */
		data[1] = result[1] & 0xFF;
		data[2] = (result[1] >> 8) & 0xFF;
		data[3] = (result[1] >> 16) & 0xFF;
		data[4] = (result[1] >> 24) & 0xFF;
		data[5] = result[2] & 0xFF;
		data[6] = (result[2] >> 8) & 0xFF;
		data[7] = (result[2] >> 16) & 0xFF;
		data[8] = (result[2] >> 24) & 0xFF;
		data[9] = result[3] & 0xFF;
		data[10] = (result[3] >> 8) & 0xFF;
		data[11] = (result[3] >> 16) & 0xFF;
		data[12] = (result[3] >> 24) & 0xFF;
		data[13] = result[4] & 0xFF;
		data[14] = (result[4] >> 8) & 0xFF;
		data[15] = (result[4] >> 16) & 0xFF;
		data[16] = (result[4] >> 24) & 0xFF;
		*data_len = 17;
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

		if (mode != MODE_BT_FOLLOW_LE) {
			channel = requested_channel;
			requested_channel = 0;

			/* CS threshold is mode-dependent. Update it after
			 * possible mode change. TODO - kludgy. */
			cs_threshold_calc_and_set();
		}
		break;

	case UBERTOOTH_SET_ISP:
		command[0] = 57;
		iap_entry(command, result);
		*data_len = 0; /* should never return */
		break;

	case UBERTOOTH_FLASH:
		bootloader_ctrl = DFU_MODE;
		reset();
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

	case UBERTOOTH_LED_SPECAN:
		if (request_params[0] > 256)
			return 0;
		rssi_threshold = (int8_t)request_params[0];
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
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_GET_SQUELCH:
		data[0] = cs_threshold_req;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_BDADDR:
		target.address = 0;
		target.access_code = 0;
		for(i=0; i < 8; i++) {
			target.address |= (uint64_t)data[i] << 8*i;
		}
		for(i=0; i < 8; i++) {
			target.access_code |= (uint64_t)data[i+8] << 8*i;
		}
		precalc();
		break;

	case UBERTOOTH_START_HOPPING:
		clock_offset = 0;
		for(i=0; i < 4; i++) {
			clock_offset <<= 8;
			clock_offset |= data[i];
		}
		clkn += clock_offset;
		hop_mode = HOP_BLUETOOTH;
		requested_mode = MODE_BT_FOLLOW;
		break;

	case UBERTOOTH_SET_CLOCK:
		clock = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
		clkn = clock;
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_SET_AFHMAP:
		for(i=0; i < 10; i++) {
			afh_map[i] = data[i];
		}
		afh_enabled = 1;
		*data_len = 10;
		precalc();
		break;

	case UBERTOOTH_CLEAR_AFHMAP:
		for(i=0; i < 10; i++) {
			afh_map[i] = 0;
		}
		afh_enabled = 0;
		*data_len = 10;
		precalc();
		break;

	case UBERTOOTH_GET_CLOCK:
		clock = clkn;
		for(i=0; i < 4; i++) {
			data[i] = (clock >> (8*i)) & 0xff;
		}
		*data_len = 4;
		break;

	case UBERTOOTH_BTLE_SNIFFING:
		rx_pkts += request_params[0];
		if (rx_pkts == 0)
			rx_pkts = 0xFFFFFFFF;
		*data_len = 0;

		do_hop = 0;
		hop_mode = HOP_BTLE;
		requested_mode = MODE_BT_FOLLOW_LE;

		queue_init();
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_GET_ACCESS_ADDRESS:
		for(i=0; i < 4; i++) {
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

		queue_init();
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_READ_REGISTER:
		reg_val = cc2400_get(request_params[0]);
		data[0] = (reg_val >> 8) & 0xff;
		data[1] = reg_val & 0xff;
		*data_len = 2;
		break;

	case UBERTOOTH_BTLE_SLAVE:
		memcpy(slave_mac_address, data, 6);
		requested_mode = MODE_BT_SLAVE_LE;
		break;

	case UBERTOOTH_BTLE_SET_TARGET:
		// Addresses appear in packets in reverse-octet order.
		// Store the target address in reverse order so that we can do a simple memcmp later
		le.target[0] = data[5];
		le.target[1] = data[4];
		le.target[2] = data[3];
		le.target[3] = data[2];
		le.target[4] = data[1];
		le.target[5] = data[0];
		le.target_set = 1;
		break;

	default:
		return 0;
	}
	return 1;
}

static void clkn_init()
{
	/*
	 * Because these are reset defaults, we're assuming TIMER0 is powered on
	 * and in timer mode.  The TIMER0 peripheral clock should have been set by
	 * clock_start().
	 */

	/* stop and reset the timer to zero */
	T0TCR = TCR_Counter_Reset;
	clkn = 0;

#ifdef TC13BADGE
	/*
	 * The peripheral clock has a period of 33.3ns.  3 pclk periods makes one
	 * CLK100NS period (100 ns).
	 */
	T0PR = 2;
#else
	/*
	 * The peripheral clock has a period of 20ns.  5 pclk periods
	 * makes one CLK100NS period (100 ns).
	 */
	T0PR = 4;
#endif
	/* 3125 * 100 ns = 312.5 us, the Bluetooth clock (CLKN). */
	T0MR0 = 3124;
	T0MCR = TMCR_MR0R | TMCR_MR0I;
	ISER0 = ISER0_ISE_TIMER0;

	/* start timer */
	T0TCR = TCR_Counter_Enable;
}

/* Update CLKN. */
void TIMER0_IRQHandler()
{
	// Use non-volatile working register to shave off a couple instructions
	u32 next;
	u32 le_clk;

	if (T0IR & TIR_MR0_Interrupt) {

		clkn++;
		next = clkn;
		le_clk = (next - le.conn_epoch) & 0x03;

		/* Trigger hop based on mode */

		/* NONE or SWEEP -> 25 Hz */
		if (hop_mode == HOP_NONE || hop_mode == HOP_SWEEP) {
			if ((next & 0x7f) == 0)
				do_hop = 1;
		}
		/* BLUETOOTH -> 1600 Hz */
		else if (hop_mode == HOP_BLUETOOTH) {
			if ((next & 0x1) == 0)
				do_hop = 1;
		}
		/* BLUETOOTH Low Energy -> 7.5ms - 4.0s in multiples of 1.25 ms */
		else if (hop_mode == HOP_BTLE) {
			//Only hop if connected
			if ((LINK_CONNECTED == le.link_state) && (0 == le_clk)) {
				--le.interval_timer;
				if (0 == le.interval_timer) {
					do_hop = 1;
					++le.conn_count;
					le.interval_timer = le.conn_interval;
				} else {
					TXLED_CLR; // hack!
				}
			}
		}

		/* Keepalive trigger fires at 3200/2^9 = 6.25 Hz */
		if ((next & 0x1ff) == 0)
			keepalive_trigger = 1;

		/* Ack interrupt */
		T0MR0 = 3124 - clock_trim;
		clock_trim = 0;
		T0IR = TIR_MR0_Interrupt;
	}
}

/* EINT3 handler is also defined in ubertooth.c for TC13BADGE. */
#ifndef TC13BADGE
//static volatile u8 txledstate = 1;
void EINT3_IRQHandler()
{
	/* TODO - check specific source of shared interrupt */
	IO2IntClr = PIN_GIO6;            // clear interrupt
	cs_trigger = 1;                  // signal trigger
	cs_timestamp = CLK100NS;         // time at trigger
}
#endif // TC13BADGE

/* Sleep (busy wait) for 'millis' milliseconds. The 'wait' routines in
 * ubertooth.c are matched to the clock setup at boot time and can not
 * be used while the board is running at 100MHz. */
static void msleep(uint32_t millis)
{
	uint32_t stop_at = clkn + millis * 3125 / 1000;  // millis -> clkn ticks
	do { } while (clkn < stop_at);                   // TODO: handle wrapping
}

static void dma_init()
{
	/* power up GPDMA controller */
	PCONP |= PCONP_PCGPDMA;

	/* zero out channel configs and clear interrupts */
	DMACC0Config = 0;
	DMACC1Config = 0;
	DMACC2Config = 0;
	DMACC3Config = 0;
	DMACC4Config = 0;
	DMACC5Config = 0;
	DMACC6Config = 0;
	DMACC7Config = 0;
	DMACIntTCClear = 0xFF;
	DMACIntErrClr = 0xFF;

	/* DMA linked lists */
	rx_dma_lli1.src = (u32)&(DIO_SSP_DR);
	rx_dma_lli1.dest = (u32)&rxbuf1[0];
	rx_dma_lli1.next_lli = (u32)&rx_dma_lli2;
	rx_dma_lli1.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	rx_dma_lli2.src = (u32)&(DIO_SSP_DR);
	rx_dma_lli2.dest = (u32)&rxbuf2[0];
	rx_dma_lli2.next_lli = (u32)&rx_dma_lli1;
	rx_dma_lli2.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	/* disable DMA interrupts */
	ICER0 = ICER0_ICE_DMA;

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	/* configure DMA channel 1 */
	DMACC0SrcAddr = rx_dma_lli1.src;
	DMACC0DestAddr = rx_dma_lli1.dest;
	DMACC0LLI = rx_dma_lli1.next_lli;
	DMACC0Control = rx_dma_lli1.control;
	DMACC0Config =
			DIO_SSP_SRC |
			(0x2 << 11) |           /* peripheral to memory */
			DMACCxConfig_IE |       /* allow error interrupts */
			DMACCxConfig_ITC;       /* allow terminal count interrupts */

	active_buf_clkn_high = (clkn >> 20) & 0xff;
	active_buf_clk100ns = CLK100NS;
	active_buf_channel = channel;

	/* reset interrupt counters */
	rx_tc = 0;
	rx_err = 0;
}

static void dma_init_le()
{
	int i;

	/* power up GPDMA controller */
	PCONP |= PCONP_PCGPDMA;

	/* zero out channel configs and clear interrupts */
	DMACC0Config = 0;
	DMACC1Config = 0;
	DMACC2Config = 0;
	DMACC3Config = 0;
	DMACC4Config = 0;
	DMACC5Config = 0;
	DMACC6Config = 0;
	DMACC7Config = 0;
	DMACIntTCClear = 0xFF;
	DMACIntErrClr = 0xFF;

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	for (i = 0; i < 11; ++i) {
		le_dma_lli[i].src = (u32)&(DIO_SSP_DR);
		le_dma_lli[i].dest = (u32)&rxbuf1[4 * i];
		le_dma_lli[i].next_lli = i < 10 ? (u32)&le_dma_lli[i+1] : 0;
		le_dma_lli[i].control = 4 |
				(1 << 12) |        /* source burst size = 4 */
				(0 << 15) |        /* destination burst size = 1 */
				(0 << 18) |        /* source width 8 bits */
				(0 << 21) |        /* destination width 8 bits */
				DMACCxControl_DI | /* destination increment */
				DMACCxControl_I;   /* terminal count interrupt enable */
	}

	/* configure DMA channel 0 */
	DMACC0SrcAddr = le_dma_lli[0].src;
	DMACC0DestAddr = le_dma_lli[0].dest;
	DMACC0LLI = le_dma_lli[0].next_lli;
	DMACC0Control = le_dma_lli[0].control;
	DMACC0Config =
			DIO_SSP_SRC |
			(0x2 << 11) |           /* peripheral to memory */
			DMACCxConfig_IE |       /* allow error interrupts */
			DMACCxConfig_ITC;       /* allow terminal count interrupts */

	active_buf_clkn_high = (clkn >> 20) & 0xff;
	active_buf_clk100ns = CLK100NS;
	active_buf_channel = channel;

	/* reset interrupt counters */
	rx_tc = 0;
	rx_err = 0;
}

void bt_stream_dma_handler(void) {
	idle_buf_clkn_high = active_buf_clkn_high;
	active_buf_clkn_high = (clkn >> 20) & 0xff;

	idle_buf_clk100ns = active_buf_clk100ns;
	active_buf_clk100ns = CLK100NS;

	idle_buf_channel = active_buf_channel;
	active_buf_channel = channel;

	/* interrupt on channel 0 */
	if (DMACIntStat & (1 << 0)) {
		if (DMACIntTCStat & (1 << 0)) {
			DMACIntTCClear = (1 << 0);
			++rx_tc;
		}
		if (DMACIntErrStat & (1 << 0)) {
			DMACIntErrClr = (1 << 0);
			++rx_err;
		}
	}
}

void DMA_IRQHandler()
{
	switch (mode) {
		case MODE_RX_SYMBOLS:
		case MODE_SPECAN:
		case MODE_BT_FOLLOW:
		case MODE_BT_FOLLOW_LE:
		case MODE_BT_PROMISC_LE:
		case MODE_BT_SLAVE_LE:
			bt_stream_dma_handler();
			break;
	}
}

static void dio_ssp_start()
{
	/* make sure the (active low) slave select signal is not active */
	DIO_SSEL_SET;

	/* enable rx DMA on DIO_SSP */
	DIO_SSP_DMACR |= SSPDMACR_RXDMAE;
	DIO_SSP_CR1 |= SSPCR1_SSE;
	
	/* enable DMA */
	DMACC0Config |= DMACCxConfig_E;
	ISER0 = ISER0_ISE_DMA;

	/* activate slave select pin */
	DIO_SSEL_CLR;
}

static void dio_ssp_stop()
{
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable DMA on SSP; disable SSP
	DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;
	DIO_SSP_CR1 &= ~SSPCR1_SSE;


	// disable DMA engine:
	// refer to UM10360 LPC17xx User Manual Ch 31 Sec 31.6.1, PDF page 607

	// disable DMA interrupts
	ICER0 = ICER0_ICE_DMA;

	// disable active channels
	DMACC0Config = 0;
	DMACC1Config = 0;
	DMACC2Config = 0;
	DMACC3Config = 0;
	DMACC4Config = 0;
	DMACC5Config = 0;
	DMACC6Config = 0;
	DMACC7Config = 0;
	DMACIntTCClear = 0xFF;
	DMACIntErrClr = 0xFF;

	// Disable the DMA controller by writing 0 to the DMA Enable bit in the DMACConfig
	// register.
	DMACConfig &= ~DMACConfig_E;
	while (DMACConfig & DMACConfig_E);
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
	mode = MODE_IDLE;
}

/* start un-buffered rx */
static void cc2400_rx()
{
	u16 mdmctrl;
	if (modulation == MOD_BT_BASIC_RATE) {
		mdmctrl = 0x0029; // 160 kHz frequency deviation
	} else if (modulation == MOD_BT_LOW_ENERGY) {
		mdmctrl = 0x0040; // 250 kHz frequency deviation
	} else {
		/* oops */
		return;
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

	// Set up CS register
	cs_threshold_calc_and_set();

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
	cs_threshold_calc_and_set();

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
#ifdef UBERTOOTH_ONE
	PAEN_SET;
	HGM_SET;
#endif
}

/*
 * Transmit a BTLE packet with the specified access address.
 *
 * All modulation parameters are set within this function. The data
 * should not be pre-whitened, but the CRC should be calculated and
 * included in the data length.
 */
void le_transmit(u32 aa, u8 len, u8 *data)
{
	unsigned i, j;
	int bit;
	u8 txbuf[64];
	u8 tx_len;
	u8 byte;
	u16 gio_save;

	// first four bytes: AA
	for (i = 0; i < 4; ++i) {
		byte = aa & 0xff;
		aa >>= 8;
		txbuf[i] = 0;
		for (j = 0; j < 8; ++j) {
			txbuf[i] |= (byte & 1) << (7 - j);
			byte >>= 1;
		}
	}

	// whiten the data and copy it into the txbuf
	int idx = whitening_index[btle_channel_index(channel-2402)];
	for (i = 0; i < len; ++i) {
		byte = data[i];
		txbuf[i+4] = 0;
		for (j = 0; j < 8; ++j) {
			bit = (byte & 1) ^ whitening[idx];
			idx = (idx + 1) % sizeof(whitening);
			byte >>= 1;
			txbuf[i+4] |= bit << (7 - j);
		}
	}

	len += 4; // include the AA in len

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

	cc2400_set(FSDIV,   channel);
	cc2400_set(FREND,   0b1011);    // amplifier level (-7 dBm, picked from hat)
	cc2400_set(MDMCTRL, 0x0040);    // 250 kHz frequency deviation
	cc2400_set(INT,     0x0014);	// FIFO_THRESHOLD: 20 bytes

	// sync byte depends on the first transmitted bit of the AA
	if (aa & 1)
		cc2400_set(SYNCH,   0xaaaa);
	else
		cc2400_set(SYNCH,   0x5555);

	// set GIO to FIFO_FULL
	gio_save = cc2400_get(IOCFG);
	cc2400_set(IOCFG, (GIO_FIFO_FULL << 9) | (gio_save & 0x1ff));

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	cc2400_strobe(STX);

	// put the packet into the FIFO
	for (i = 0; i < len; i += 16) {
		while (GIO6) ; // wait for the FIFO to drain (FIFO_FULL false)
		tx_len = len - i;
		if (tx_len > 16)
			tx_len = 16;
		cc2400_spi_buf(FIFOREG, tx_len, txbuf + i);
	}

	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	TXLED_CLR;

	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
#endif

	// reset GIO
	cc2400_set(IOCFG, gio_save);
}

/* TODO - return whether hop happened, or should caller have to keep
 * track of this? */
void hop(void)
{
	do_hop = 0;

	// No hopping, if channel is set correctly, do nothing
	if (hop_mode == HOP_NONE) {
		if (cc2400_get(FSDIV) == (channel - 1))
			return;
	}

	// Slow sweep (100 hops/sec)
	else if (hop_mode == HOP_SWEEP) {
		channel += 32;
		if (channel > 2480)
			channel -= 79;
	}

	else if (hop_mode == HOP_BLUETOOTH) {
		TXLED_SET;
		channel = next_hop(clkn);
	}

	else if (hop_mode == HOP_BTLE) {
		TXLED_SET;
		channel = btle_next_hop(&le);
	}

	else if (hop_mode == HOP_DIRECT) {
		TXLED_SET;
		channel = hop_direct_channel;
	}

        /* IDLE mode, but leave amp on, so don't call cc2400_idle(). */
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK)); // need to wait for unlock?

	/* Retune */
	cc2400_set(FSDIV, channel - 1);
	
	/* Update CS register if hopping.  */
	if (hop_mode > 0) {
		cs_threshold_calc_and_set();
	}

	/* Wait for lock */
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	
	/* RX mode */
	cc2400_strobe(SRX);

}

/* Bluetooth packet monitoring */
void bt_stream_rx()
{
	u8 *tmp = NULL;
	u8 hold;
	int8_t rssi;
	int i;
	int8_t rssi_at_trigger;

	mode = MODE_RX_SYMBOLS;

	RXLED_CLR;

	queue_init();
	dio_ssp_init();
	dma_init();
	dio_ssp_start();
	cc2400_rx();

	cs_trigger_enable();

	hold = 0;

	while (rx_pkts && (requested_mode == MODE_RX_SYMBOLS)) {

		/* If timer says time to hop, do it. TODO - set
		 * per-channel carrier sense threshold. Set by
		 * firmware or host. TODO - if hop happened, clear
		 * hold. */
		if (do_hop)
			hop();

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
		while ((rx_tc == 0) && (rx_err == 0)) {
			rssi = (int8_t)(cc2400_get(RSSI) >> 8);
			if (cs_trigger && (rssi_at_trigger == INT8_MIN)) {
				rssi = MAX(rssi,(cs_threshold_cur+54));
				rssi_at_trigger = rssi;
			}
			rssi_add(rssi);
		}

		/* Keep buffer swapping in sync with DMA. */
		if (rx_tc % 2) {
			tmp = active_rxbuf;
			active_rxbuf = idle_rxbuf;
			idle_rxbuf = tmp;
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

		rssi_iir_update();

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

		/* Send a packet once in a while (6.25 Hz) to keep
		 * host USB reads from timing out. */
		if (keepalive_trigger) {
			if (hold == 0)
				hold = 1;
			keepalive_trigger = 0;
		}

		/* Hold expired? Ignore data. */
		if (hold == 0) {
			goto rx_continue;
		}
		hold--;

		/* Queue data from DMA buffer. */
		switch (hop_mode) {
			case HOP_BLUETOOTH:
				//if (find_access_code(idle_rxbuf) >= 0)
						if (enqueue(BR_PACKET, idle_rxbuf)) {
								RXLED_SET;
								--rx_pkts;
						}
				break;

			default:
				if (enqueue(BR_PACKET, idle_rxbuf)) {
						RXLED_SET;
						--rx_pkts;
				}
		}

	rx_continue:
		handle_usb(clkn);
		rx_tc = 0;
		rx_err = 0;
	}

	/* This call is a nop so far. Since bt_rx_stream() starts the
	 * stream, it makes sense that it would stop it. TODO - how
	 * should setup/teardown be handled? Should every new mode be
	 * starting from scratch? */
	dio_ssp_stop();
	cs_trigger_disable();
	rx_pkts = 0; // Already 0, or requested mode changed
}


/* Bluetooth packet monitoring */
void bt_follow()
{
	u8 *tmp = NULL;
	u8 hold;
	int8_t rssi;
	int i;
	int16_t packet_offset;
	int8_t rssi_at_trigger;

	mode = MODE_BT_FOLLOW;

	RXLED_CLR;

	queue_init();
	dio_ssp_init();
	dma_init();
	dio_ssp_start();
	cc2400_rx_sync((syncword >> 32) & 0xffffffff);

	cs_trigger_enable();

	hold = 0;

	while (requested_mode == MODE_BT_FOLLOW) {

		/* If timer says time to hop, do it. TODO - set
		 * per-channel carrier sense threshold. Set by
		 * firmware or host. TODO - if hop happened, clear
		 * hold. */
		if (do_hop)
			hop();

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
		while ((rx_tc == 0) && (rx_err == 0)) {
			rssi = (int8_t)(cc2400_get(RSSI) >> 8);
			if (cs_trigger && (rssi_at_trigger == INT8_MIN)) {
				rssi = MAX(rssi,(cs_threshold_cur+54));
				rssi_at_trigger = rssi;
			}
			rssi_add(rssi);
		}

		/* Keep buffer swapping in sync with DMA. */
		if (rx_tc % 2) {
			tmp = active_rxbuf;
			active_rxbuf = idle_rxbuf;
			idle_rxbuf = tmp;
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

		rssi_iir_update();

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

		/* Send a packet once in a while (6.25 Hz) to keep
		 * host USB reads from timing out. */
		if (keepalive_trigger) {
			if (hold == 0)
				hold = 1;
			keepalive_trigger = 0;
		}

		/* Hold expired? Ignore data. */
		if (hold == 0) {
			goto rx_continue;
		}
		hold--;

		/* Queue data from DMA buffer. */
		//if ((packet_offset = find_access_code(idle_rxbuf)) >= 0) {
		//	clock_trim = 20 - packet_offset;
			if (enqueue(BR_PACKET, idle_rxbuf)) {
				RXLED_SET;
				--rx_pkts;
			}
		//}

	rx_continue:
		handle_usb(clkn);
		rx_tc = 0;
		rx_err = 0;
	}

	/* This call is a nop so far. Since bt_rx_stream() starts the
	 * stream, it makes sense that it would stop it. TODO - how
	 * should setup/teardown be handled? Should every new mode be
	 * starting from scratch? */
	dio_ssp_stop();
	cs_trigger_disable();
}

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
	le.crc_init  = 0x555555;	       // advertising channel CRCInit
	le.crc_init_reversed = 0xAAAAAA;
	le.crc_verify = 1;
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
	le.win_offset_update;

	do_hop = 0;
}

/* generic le mode */
void bt_generic_le(u8 active_mode)
{
	u8 *tmp = NULL;
	u8 hold;
	int i, j;
	int8_t rssi, rssi_at_trigger;

	modulation = MOD_BT_LOW_ENERGY;
	mode = active_mode;

	reset_le();

	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	RXLED_CLR;

	queue_init();
	dio_ssp_init();
	dma_init();
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

		/* Keep buffer swapping in sync with DMA. */
		if (rx_tc % 2) {
			tmp = active_rxbuf;
			active_rxbuf = idle_rxbuf;
			idle_rxbuf = tmp;
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

		rssi_iir_update();

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

		/* Send a packet once in a while (6.25 Hz) to keep
		 * host USB reads from timing out. */
		if (keepalive_trigger) {
			if (hold == 0)
				hold = 1;
			keepalive_trigger = 0;
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

	modulation = MOD_BT_LOW_ENERGY;
	mode = active_mode;

	le.link_state = LINK_LISTENING;

	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	RXLED_CLR;

	queue_init();
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

			saved_request = requested_channel;
			requested_channel = 0;
		}

		RXLED_CLR;

		/* Wait for DMA. Meanwhile keep track of RSSI. */
		rssi_reset();
		while ((rx_tc == 0) && (rx_err == 0) && (do_hop == 0) && requested_mode == active_mode)
			;

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

		uint32_t packet[48/4+1];
		u8 *p = (u8 *)packet;
		packet[0] = le.access_address;

		const uint32_t *whit = whitening_word[btle_channel_index(channel-2402)];
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
		enqueue(LE_PACKET, (uint8_t *)packet);
		le.last_packet = CLK100NS;

	rx_flush:
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));

		// flush any excess bytes from the SSP's buffer
		DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;
		while (SSP1SR & SSPSR_RNE) {
			u8 tmp = (u8)DIO_SSP_DR;
		}

		// timeout - FIXME this is an ugly hack
		if (((LINK_CONNECTED == le.link_state) || (LINK_CONN_PENDING == le.link_state)) 
			&& ((CLK100NS - le.last_packet) > 50000000))
		{
			reset_le();

			cc2400_strobe(SRFOFF);
			while ((cc2400_status() & FS_LOCK));

			/* Retune */
			channel = saved_request != 0 ? saved_request : 2402;
			cc2400_set(FSDIV, channel - 1);

			/* Wait for lock */
			cc2400_strobe(SFSON);
			while (!(cc2400_status() & FS_LOCK));
		}

		cc2400_set(SYNCL, le.syncl);
		cc2400_set(SYNCH, le.synch);

		if (do_hop)
			hop();

		/* RX mode */
		dma_init_le();
		dio_ssp_start();
		cc2400_strobe(SRX);

	rx_continue:
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
	int idx = whitening_index[btle_channel_index(channel-2402)];

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
				u32 calc_crc = btle_crcgen_lut(le.crc_init_reversed, idle_rxbuf + 4, len);
				u32 wire_crc = (idle_rxbuf[4+len+2] << 16)
							 | (idle_rxbuf[4+len+1] << 8)
							 |  idle_rxbuf[4+len+0];
				if (calc_crc != wire_crc) // skip packets with a bad CRC
					break;
			}

			// send to PC
			enqueue(LE_PACKET, idle_rxbuf);
			RXLED_SET;
			--rx_pkts;

			packet_cb(idle_rxbuf);

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

	u8 *adv_addr = &packet[ADV_ADDRESS_IDX];
	u8 header = packet[HEADER_IDX];
	u8 *data_len = &packet[DATA_LEN_IDX];
	u8 *data = &packet[DATA_START_IDX];
	u8 *crc = &packet[DATA_START_IDX + *data_len];

	if (LINK_CONN_PENDING == le.link_state) {
		// We received a packet in the connection pending state, so now the device *should* be connected
		le.link_state = LINK_CONNECTED;
		le.conn_epoch = clkn;
		le.interval_timer = le.conn_interval - 1;
		le.conn_count = 0;
		le.update_pending = 0;

	} else if (LINK_CONNECTED == le.link_state) {
		u8 llid =  header & 0x03;

		// Apply any connection parameter update if necessary
		if (le.update_pending && (le.conn_count == le.update_instant)) {
			// This is the first packet received in the connection interval for which the new parameters apply
			le.conn_epoch = clkn;
			le.conn_interval = le.interval_update;
			le.interval_timer = le.interval_update - 1;
			le.win_size = le.win_size_update;
			le.win_offset = le.win_offset_update;
			le.update_pending = 0;
		}

		if ((0x03 == llid) && (0x00 == data[0])) {
			// This is a CONNECTION_UPDATE_REQ.
			// The host is changing the connection parameters.
			le.win_size_update = packet[7];
			le.win_offset_update = packet[8] + ((u16)packet[9] << 8);
			le.interval_update = packet[10] + ((u16)packet[11] << 8);
			le.update_instant = packet[16] + ((u16)packet[17] << 8);
			if ((le.update_instant - le.conn_count) < 32767) {
				le.update_pending = 1;
			}
		}

	} else if (LINK_LISTENING == le.link_state) {
		u8 pkt_type = packet[4] & 0x0F;
		if (0x05 == pkt_type) {
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
			le.conn_interval = packet[CONN_INTERVAL];

#define CHANNEL_INC (2+4+6+6+4+3+1+2+2+2+2+5)
			le.channel_increment = packet[CHANNEL_INC] & 0x1f;
			le.channel_idx = le.channel_increment;

			//Hop to the initial channel immediately
			do_hop = 1;
		}
	}
}

void bt_follow_le() {
	reset_le();
	packet_cb = connection_follow_cb;
	bt_le_sync(MODE_BT_FOLLOW_LE);

	/* old non-sync mode
	data_cb = cb_follow_le;
	packet_cb = connection_follow_cb;
	bt_generic_le(MODE_BT_FOLLOW_LE);
	*/
}

// issue state change message
void le_promisc_state(u8 type, void *data, unsigned len) {
	u8 buf[50] = { 0, };
	if (len > 49)
		len = 49;

	buf[0] = type;
	memcpy(&buf[1], data, len);
	enqueue(LE_PROMISC, buf);
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
	static u32 smallest_interval = 0xffffffff;
	static int consec = 0;

	u32 cur_clk = CLK100NS;
	u32 clk_diff = cur_clk - prev_clk;
	u16 obsv_hop_interval; // observed hop interval

	// probably consecutive data packets on the same channel
	if (clk_diff < 2 * LE_BASECLK)
		return;

	if (clk_diff < smallest_interval)
		smallest_interval = clk_diff;

	obsv_hop_interval = DIVIDE_ROUND(smallest_interval, 37 * LE_BASECLK);

	if (le.conn_interval == obsv_hop_interval) {
		// 5 consecutive hop intervals: consider it legit and move on
		++consec;
		if (consec == 5) {
			packet_cb = promisc_recover_hop_increment;
			hop_direct_channel = 2404;
			hop_mode = HOP_DIRECT;
			do_hop = 1;
			le_promisc_state(2, &le.conn_interval, 2);
		}
	} else {
		le.conn_interval = obsv_hop_interval;
		consec = 0;
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

// LFU cache of recently seen AA's
struct active_aa {
	u32 aa;
	int freq;
} active_aa_list[32] = { { 0, 0, }, };
#define AA_LIST_SIZE (int)(sizeof(active_aa_list) / sizeof(struct active_aa))

// called when we see an AA, add it to the list
void see_aa(u32 aa) {
	int i, max = -1, killme = -1;
	for (i = 0; i < AA_LIST_SIZE; ++i)
		if (active_aa_list[i].aa == aa) {
			++active_aa_list[i].freq;
			return;
		}

	// evict someone
	for (i = 0; i < AA_LIST_SIZE; ++i)
		if (active_aa_list[i].freq < max || max < 0) {
			killme = i;
			max = active_aa_list[i].freq;
		}

	active_aa_list[killme].aa = aa;
	active_aa_list[killme].freq = 1;
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
		idx = whitening_index[btle_channel_index(channel-2402)];

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
		idx = whitening_index[btle_channel_index(channel-2402)];
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

		enqueue(LE_PACKET, idle_rxbuf);

	}

	// once we see an AA 5 times, start following it
	for (i = 0; i < AA_LIST_SIZE; ++i) {
		if (active_aa_list[i].freq > 3) {
			le_set_access_address(active_aa_list[i].aa);
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
	// jump to a random data channel and turn up the squelch
	channel = 2440;

	// if the PC hasn't given us AA, determine by listening
	if (!le.target_set) {
		cs_threshold_req = -70;
		cs_threshold_calc_and_set();
		data_cb = cb_le_promisc;
		bt_generic_le(MODE_BT_PROMISC_LE);
	}

	le_promisc_state(0, &le.access_address, 4);
	packet_cb = promisc_follow_cb;
	le.crc_verify = 0;
	bt_le_sync(MODE_BT_PROMISC_LE);
}

void bt_slave_le() {
	u32 calc_crc;
	int i;

	u8 adv_ind[] = {
		// LL header
		0x00, 0x09,

		// advertising address
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

		// advertising data
		0x02, 0x01, 0x05,

		// CRC (calc)
		0xff, 0xff, 0xff,
	};

	u8 adv_ind_len = sizeof(adv_ind) - 3;

	// copy the user-specified mac address
	for (i = 0; i < 6; ++i)
		adv_ind[i+2] = slave_mac_address[5-i];

	calc_crc = btle_calc_crc(le.crc_init_reversed, adv_ind, adv_ind_len);
	adv_ind[adv_ind_len+0] = (calc_crc >>  0) & 0xff;
	adv_ind[adv_ind_len+1] = (calc_crc >>  8) & 0xff;
	adv_ind[adv_ind_len+2] = (calc_crc >> 16) & 0xff;

	// spam advertising packets
	while (requested_mode == MODE_BT_SLAVE_LE) {
		ICER0 = ICER0_ICE_USB;
		ICER0 = ICER0_ICE_DMA;
		le_transmit(0x8e89bed6, adv_ind_len+3, adv_ind);
		ISER0 = ISER0_ISE_USB;
		ISER0 = ISER0_ISE_DMA;
		msleep(100);
	}
}

/* spectrum analysis */
void specan()
{
	u8 epstat;
	u16 f;
	u8 i = 0;
	u8 buf[DMA_SIZE];

	RXLED_SET;

	queue_init();

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
	mode = MODE_IDLE;
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
		lvl = cc2400_get(RSSI) >> 8;
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
        //wait(1);
		cc2400_strobe(SRFOFF);
		while ((cc2400_status() & FS_LOCK));
	}
	mode = MODE_IDLE;
}

int main()
{
	ubertooth_init();
	clkn_init();
	ubertooth_usb_init(vendor_request_handler);

	while (1) {
		handle_usb(clkn);
		if(requested_mode != mode)
			switch (requested_mode) {
				 case MODE_RESET:
					/* Allow time for the USB command to return correctly */
					wait(1);
					reset();
					break;
				case MODE_RX_SYMBOLS:
					if (rx_pkts)
						bt_stream_rx();
					break;
				case MODE_BT_FOLLOW:
					bt_follow();
					break;
				case MODE_BT_FOLLOW_LE:
					bt_follow_le();
					break;
				case MODE_BT_PROMISC_LE:
					bt_promisc_le();
					break;
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
					mode = MODE_IDLE;
					if (requested_mode == MODE_RANGE_TEST)
						requested_mode = MODE_IDLE;
					break;
				case MODE_REPEATER:
					mode = MODE_REPEATER;
					cc2400_repeater(&channel);
					break;
				case MODE_SPECAN:
					specan();
					break;
				case MODE_LED_SPECAN:
					led_specan();
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
