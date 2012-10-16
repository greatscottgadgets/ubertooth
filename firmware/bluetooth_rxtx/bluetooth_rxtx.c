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

/*
	LPCUSB, an USB device driver for LPC microcontrollers	
	Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>

#include "ubertooth.h"
#include "usbapi.h"
#include "usbhw_lpc.h"
#include "ubertooth_interface.h"
#include "bluetooth.h"
#include "bluetooth_le.h"

#define IAP_LOCATION 0x1FFF1FF1
typedef void (*IAP)(u32[], u32[]);
IAP iap_entry = (IAP)IAP_LOCATION;

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
u8 do_hop = 0;                              // set by timer interrupt
u8 le_hop_after = 0;                        // hop after this many 1.25 ms cycles
u8 cs_no_squelch = 0;                       // rx all packets if set
int8_t cs_threshold_req=CS_THRESHOLD_DEFAULT; // requested CS threshold in dBm
int8_t cs_threshold_cur=CS_THRESHOLD_DEFAULT; // current CS threshold in dBm
volatile u8 cs_trigger;                     // set by intr on P2.2 falling (CS)
volatile u8 keepalive_trigger;              // set by timer 1/s
volatile u32 cs_timestamp;                  // CLK100NS at time of cs_trigger
u32 desired_address = 0x8e89bed6;
u32 crc_init = 0x555555;					// advertising channel CRCInit
u32 crc_init_reversed = 0xAAAAAA;
int crc_verify = 1;							// reject packets with bad CRC
int le_connected = 0;                       // true if LE is connected
u8 le_hop_amount = 0;                       // amount to hop in LE
u8 le_channel_idx = 0;                      // current channel index in LE
u16 le_hop_interval = 0;                    // connection-specific hop interval
u16 hop_direct_channel = 0;                 // for hopping directly to a channel
u32 last_usb_pkt = 0;                       // for keep alive packets
int clock_trim = 0;                         // to counteract clock drift
u32 idle_buf_clkn = 0;
u32 active_buf_clkn = 0;
u32 idle_buf_channel = 0;
u32 active_buf_channel = 0;

typedef void (*data_cb_t)(char *);
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

#define BULK_IN_EP		0x82
#define BULK_OUT_EP		0x05

#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

/* DMA buffers */
u8 rxbuf1[DMA_SIZE];
u8 rxbuf2[DMA_SIZE];

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
u8 *active_rxbuf = &rxbuf1[0];
u8 *idle_rxbuf = &rxbuf2[0];

/* Unpacked symbol buffers (two rxbufs) */
char unpacked[DMA_SIZE*8*2];

rangetest_result rr;

volatile u8 mode = MODE_IDLE;
volatile u8 requested_mode = MODE_IDLE;
volatile u8 modulation = MOD_BT_BASIC_RATE;
volatile u16 channel = 2441;
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
	ISER0 |= ISER0_ISE_EINT3;
	IO2IntClr = PIN_GIO6;      // Clear pending
	IO2IntEnF |= PIN_GIO6;     // Enable port 2.2 falling (CS active low)
}

static void cs_trigger_disable(void)
{
	IO2IntEnF &= ~PIN_GIO6;    // Disable port 2.2 falling (CS active low)
	IO2IntClr = PIN_GIO6;      // Clear pending
	ISER0 &= ~ISER0_ISE_EINT3;
	cs_trigger = 0;
}

/*
 * This is supposed to be a lock-free ring buffer, but I haven't verified
 * atomicity of the operations on head and tail.
 */

usb_pkt_rx fifo[128];

volatile u32 head = 0;
volatile u32 tail = 0;

void queue_init()
{
	head = 0;
	tail = 0;
}

static int enqueue(u8 *buf)
{
	int i;
	u8 h = head & 0x7F;
	u8 t = tail & 0x7F;
	u8 n = (t + 1) & 0x7F;

	usb_pkt_rx *f = &fifo[t];

	/* fail if queue is full */
	if (h == n) {
		status |= FIFO_OVERFLOW;
		return 0;
	}

    f->pkt_type = BR_PACKET;
	f->clkn_high = (clkn >> 20) & 0xff;
	if (hop_mode == HOP_BLUETOOTH)
		f->clk100ns = idle_buf_clkn;
	else
		f->clk100ns = CLK100NS;
	f->channel = idle_buf_channel - 2402;
	f->rssi_min = rssi_min;
	f->rssi_max = rssi_max;
	if (hop_mode != HOP_NONE)
		f->rssi_avg = (int8_t)((rssi_iir[channel-2402] + 128)/256);
	else
		f->rssi_avg = (int8_t)((rssi_iir[0] + 128)/256);
	f->rssi_count = rssi_count;

	USRLED_SET;

	// Unrolled copy of 50 bytes from buf to fifo
	u32 *p1 = (u32 *)fifo[t].data;
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
	*(u16 *)&(fifo[t].data[48]) = *(u16 *)&(buf[48]);

	fifo[t].status = status;
	status = 0;
	++tail;

	return 1;
}

static usb_pkt_rx *dequeue()
{
	u8 h = head & 0x7F;
	u8 t = tail & 0x7F;

	/* fail if queue is empty */
	if (h == t) {
		USRLED_CLR;
		return NULL;
	}

	++head;

	return &fifo[h];
}

#define USB_KEEP_ALIVE 400000
static int dequeue_send()
{
	usb_pkt_rx *pkt = dequeue(&pkt);
	if (pkt != NULL) {
		last_usb_pkt = clkn;
		USBHwEPWrite(BULK_IN_EP, (u8 *)pkt, sizeof(usb_pkt_rx));
		return 1;
	} else {
		if (clkn - last_usb_pkt > USB_KEEP_ALIVE) {
			u8 pkt_type = KEEP_ALIVE;
			last_usb_pkt = clkn;
			USBHwEPWrite(BULK_IN_EP, &pkt_type, 1);
		}
		return 0;
	}
}


#ifdef UBERTOOTH_ZERO
#define ID_VENDOR 0x1D50
#define ID_PRODUCT 0x6000
#elif defined UBERTOOTH_ONE
#define ID_VENDOR 0x1D50
#define ID_PRODUCT 0x6002
#elif defined TC13BADGE
#define ID_VENDOR 0xFFFF
#define ID_PRODUCT 0x0004
#else
#define ID_VENDOR 0xFFFF
#define ID_PRODUCT 0x0004
#endif


static const u8 abDescriptors[] = {

/* Device descriptor */
	0x12,              		
	DESC_DEVICE,       		
	LE_WORD(0x0200),		// bcdUSB	
	0xFF,              		// bDeviceClass
	0x00,              		// bDeviceSubClass
	0x00,              		// bDeviceProtocol
	MAX_PACKET_SIZE0,  		// bMaxPacketSize
	LE_WORD(ID_VENDOR),		// idVendor
	LE_WORD(ID_PRODUCT),	// idProduct
	LE_WORD(0x0100),		// bcdDevice
	0x01,              		// iManufacturer
	0x02,              		// iProduct
	0x03,              		// iSerialNumber
	0x01,              		// bNumConfigurations

// configuration
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(0x20),  		// wTotalLength
	0x01,  					// bNumInterfaces
	0x01,  					// bConfigurationValue
	0x00,  					// iConfiguration
	0x80,  					// bmAttributes
	0x32,  					// bMaxPower

// interface
	0x09,   				
	DESC_INTERFACE, 
	0x00,  		 			// bInterfaceNumber
	0x00,   				// bAlternateSetting
	0x02,   				// bNumEndPoints
	0xFF,   				// bInterfaceClass
	0x00,   				// bInterfaceSubClass
	0x00,   				// bInterfaceProtocol
	0x00,   				// iInterface

// bulk in
	0x07,   		
	DESC_ENDPOINT,   		
	BULK_IN_EP,				// bEndpointAddress
	0x02,   				// bmAttributes = BULK
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0,						// bInterval   		

// bulk out
	0x07,   		
	DESC_ENDPOINT,   		
	BULK_OUT_EP,			// bEndpointAddress
	0x02,   				// bmAttributes = BULK
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0,						// bInterval 

// string descriptors
	0x04,
	DESC_STRING,
	LE_WORD(0x0409),

	// manufacturer string
	0x44,
	DESC_STRING,
	'h', 0, 't', 0, 't', 0, 'p', 0, ':', 0, '/', 0, '/', 0, 'u', 0,
	'b', 0, 'e', 0, 'r', 0, 't', 0, 'o', 0, 'o', 0, 't', 0, 'h', 0,
	'.', 0, 's', 0, 'o', 0, 'u', 0, 'r', 0, 'c', 0, 'e', 0, 'f', 0,
	'o', 0, 'r', 0, 'g', 0, 'e', 0, '.', 0, 'n', 0, 'e', 0, 't', 0,
	'/', 0,

	// product string
	0x1E,
	DESC_STRING,
	'b', 0, 'l', 0, 'u', 0, 'e', 0, 't', 0, 'o', 0, 'o', 0, 't', 0, 'h', 0, '_', 0,
	'r', 0, 'x', 0, 't', 0, 'x', 0,

	// serial number string
	0x12,
	DESC_STRING,
	'0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0,

	// terminator
	0
};

static u8 abVendorReqData[258];

static void usb_bulk_in_handler(u8 bEP, u8 bEPStatus)
{
	if (!(bEPStatus & EP_STATUS_DATA))
		dequeue_send();
}

static void usb_bulk_out_handler(u8 bEP, u8 bEPStatus)
{
}

static BOOL usb_vendor_request_handler(TSetupPacket *pSetup, int *piLen, u8 **ppbData)
{
	u8 *pbData = *ppbData;
	u32 command[5];
	u32 result[5];
	u64 ac_copy;
	int i; // loop counter
	u32 clock;
	int clock_offset;
	u8 length; // string length
	usb_pkt_rx *p = NULL;

	switch (pSetup->bRequest) {

	case UBERTOOTH_PING:
		*piLen = 0;
		break;

	case UBERTOOTH_RX_SYMBOLS:
		requested_mode = MODE_RX_SYMBOLS;
		rx_pkts += pSetup->wValue;
		if (rx_pkts == 0)
			rx_pkts = 0xFFFFFFFF;
		*piLen = 0;
		break;

	case UBERTOOTH_GET_USRLED:
		pbData[0] = (USRLED) ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_USRLED:
		if (pSetup->wValue)
			USRLED_SET;
		else
			USRLED_CLR;
		break;

	case UBERTOOTH_GET_RXLED:
		pbData[0] = (RXLED) ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_RXLED:
		if (pSetup->wValue)
			RXLED_SET;
		else
			RXLED_CLR;
		break;

	case UBERTOOTH_GET_TXLED:
		pbData[0] = (TXLED) ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_TXLED:
		if (pSetup->wValue)
			TXLED_SET;
		else
			TXLED_CLR;
		break;

	case UBERTOOTH_GET_1V8:
		pbData[0] = (CC1V8) ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_1V8:
		if (pSetup->wValue)
			CC1V8_SET;
		else
			CC1V8_CLR;
		break;

	case UBERTOOTH_GET_PARTNUM:
		command[0] = 54; /* read part number */
		iap_entry(command, result);
		pbData[0] = result[0] & 0xFF; /* status */
		pbData[1] = result[1] & 0xFF;
		pbData[2] = (result[1] >> 8) & 0xFF;
		pbData[3] = (result[1] >> 16) & 0xFF;
		pbData[4] = (result[1] >> 24) & 0xFF;
		*piLen = 5;
		break;

	case UBERTOOTH_RESET:
		reset();
		break;

	case UBERTOOTH_GET_SERIAL:
		command[0] = 58; /* read device serial number */
		iap_entry(command, result);
		pbData[0] = result[0] & 0xFF; /* status */
		pbData[1] = result[1] & 0xFF;
		pbData[2] = (result[1] >> 8) & 0xFF;
		pbData[3] = (result[1] >> 16) & 0xFF;
		pbData[4] = (result[1] >> 24) & 0xFF;
		pbData[5] = result[2] & 0xFF;
		pbData[6] = (result[2] >> 8) & 0xFF;
		pbData[7] = (result[2] >> 16) & 0xFF;
		pbData[8] = (result[2] >> 24) & 0xFF;
		pbData[9] = result[3] & 0xFF;
		pbData[10] = (result[3] >> 8) & 0xFF;
		pbData[11] = (result[3] >> 16) & 0xFF;
		pbData[12] = (result[3] >> 24) & 0xFF;
		pbData[13] = result[4] & 0xFF;
		pbData[14] = (result[4] >> 8) & 0xFF;
		pbData[15] = (result[4] >> 16) & 0xFF;
		pbData[16] = (result[4] >> 24) & 0xFF;
		*piLen = 17;
		break;

#ifdef UBERTOOTH_ONE
	case UBERTOOTH_GET_PAEN:
		pbData[0] = (PAEN) ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_PAEN:
		if (pSetup->wValue)
			PAEN_SET;
		else
			PAEN_CLR;
		break;

	case UBERTOOTH_GET_HGM:
		pbData[0] = (HGM) ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_HGM:
		if (pSetup->wValue)
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
		pbData[0] = cc2400_get(FREND) & 0x7;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_PALEVEL:
		if( pSetup->wValue < 8 ) {
			cc2400_set(FREND, 8 | pSetup->wValue);
		} else {
			return FALSE;
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
		pbData[0] = rr.valid;
		pbData[1] = rr.request_pa;
		pbData[2] = rr.request_num;
		pbData[3] = rr.reply_pa;
		pbData[4] = rr.reply_num;
		*piLen = 5;
		break;

	case UBERTOOTH_STOP:
		requested_mode = MODE_IDLE;
		break;

	case UBERTOOTH_GET_MOD:
		pbData[0] = modulation;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_MOD:
		modulation = pSetup->wValue;
		break;

	case UBERTOOTH_GET_CHANNEL:
		pbData[0] = channel & 0xFF;
		pbData[1] = (channel >> 8) & 0xFF;
		*piLen = 2;
		break;

	case UBERTOOTH_SET_CHANNEL:
		channel = pSetup->wValue;
		/* bluetooth band sweep mode, start at channel 2402 */
		if (channel == 9999) {
			hop_mode = HOP_SWEEP;
			channel = 2402;
		}
		/* fixed channel mode, can be outside blueooth band */
		else {
			hop_mode = HOP_NONE;
			channel = MAX(channel, MIN_FREQ);
			channel = MIN(channel, MAX_FREQ);
		}

		/* CS threshold is mode-dependent. Update it after
		 * possible mode change. TODO - kludgy. */
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_SET_ISP:
		command[0] = 57;
		iap_entry(command, result);
		*piLen = 0; /* should never return */
		break;

	case UBERTOOTH_FLASH:
		bootloader_ctrl = DFU_MODE;
		reset();
		break;

	case UBERTOOTH_SPECAN:
		if (pSetup->wValue < 2049 || pSetup->wValue > 3072 || 
				pSetup->wIndex < 2049 || pSetup->wIndex > 3072 ||
				pSetup->wIndex < pSetup->wValue)
			return FALSE;
		low_freq = pSetup->wValue;
		high_freq = pSetup->wIndex;
		requested_mode = MODE_SPECAN;
		*piLen = 0;
		break;

	case UBERTOOTH_LED_SPECAN:
		if (pSetup->wValue > 256)
			return FALSE;
		rssi_threshold = (int8_t)pSetup->wValue;
		requested_mode = MODE_LED_SPECAN;
		*piLen = 0;
		break;

	case UBERTOOTH_GET_REV_NUM:
		pbData[0] = 0x00;
		pbData[1] = 0x00;

		length = (u8)strlen(GIT_DESCRIBE);
		pbData[2] = length;

		memcpy(&pbData[3], GIT_DESCRIBE, length);

		*piLen = 2 + 1 + length;
		break;

	case UBERTOOTH_GET_BOARD_ID:
		pbData[0] = BOARD_ID;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_SQUELCH:
		cs_threshold_req = (int8_t)pSetup->wValue;
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_GET_SQUELCH:
		pbData[0] = cs_threshold_req;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_BDADDR:
		target.address = 0;
		target.access_code = 0;
		for(i=0; i < 8; i++) {
			target.address |= pbData[i] << 8*i;
		}
		for(0; i < 8; i++) {
			target.access_code |= pbData[i+8] << 8*i;
		}
		precalc();
		break;

	case UBERTOOTH_START_HOPPING:
		clock_offset = 0;
		for(i=0; i < 4; i++) {
			clock_offset << 8;
			clock_offset |= pbData[i];
		}
		clkn += clock_offset;
		hop_mode = HOP_BLUETOOTH;
		requested_mode = MODE_BT_FOLLOW;
		break;

	case UBERTOOTH_SET_CLOCK:
		clock = pbData[0] | pbData[1] << 8 | pbData[2] << 16 | pbData[3] << 24;
		clkn = clock;
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_GET_CLOCK:
		clock = clkn;
		for(i=0; i < 4; i++) {
			pbData[i] = (clock >> (8*i)) & 0xff;
		}
		*piLen = 4;
		break;

	case UBERTOOTH_BTLE_SNIFFING:
		rx_pkts += pSetup->wValue;
		if (rx_pkts == 0)
			rx_pkts = 0xFFFFFFFF;
		*piLen = 0;

		do_hop = 0;
		hop_mode = HOP_BTLE;
		requested_mode = MODE_BT_FOLLOW_LE;

		queue_init();
		cs_threshold_calc_and_set();
		break;

	case UBERTOOTH_GET_ACCESS_ADDRESS:
		for(i=0; i < 4; i++) {
			pbData[i] = (desired_address >> (8*i)) & 0xff;
		}
		*piLen = 4;
		break;

	case UBERTOOTH_SET_ACCESS_ADDRESS:
		desired_address = pbData[0] | pbData[1] << 8 | pbData[2] << 16 | pbData[3] << 24;
		break;

	case UBERTOOTH_DO_SOMETHING:
		// do something! just don't commit anything here
		break;

	case UBERTOOTH_DO_SOMETHING_REPLY:
		// after you do something, tell me what you did!
		// don't commit here please
		pbData[0] = 0x13;
		pbData[1] = 0x37;
		*piLen = 2;
		break;

	case UBERTOOTH_GET_CRC_VERIFY:
		pbData[0] = crc_verify ? 1 : 0;
		*piLen = 1;
		break;

	case UBERTOOTH_SET_CRC_VERIFY:
		crc_verify = pSetup->wValue ? 1 : 0;
		break;

	case UBERTOOTH_POLL:
		p = dequeue();
		if (p != NULL) {
			memcpy(pbData, (void *)p, sizeof(usb_pkt_rx));
			*piLen = sizeof(usb_pkt_rx);
		} else {
			pbData[0] = 0;
			*piLen = 1;
		}
		break;

	case UBERTOOTH_BTLE_PROMISC:
		*piLen = 0;

		hop_mode = HOP_NONE;
		requested_mode = MODE_BT_PROMISC_LE;

		queue_init();
		cs_threshold_calc_and_set();
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static int ubertooth_usb_init()
{
	// initialise stack
	USBInit();
	
	// register device descriptors
	USBRegisterDescriptors(abDescriptors);

	// override standard request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_VENDOR, usb_vendor_request_handler, abVendorReqData);

	// register endpoints
	//USBHwRegisterEPIntHandler(BULK_IN_EP, usb_bulk_in_handler);
	//USBHwRegisterEPIntHandler(BULK_OUT_EP, usb_bulk_out_handler);

	// enable USB interrupts
	//ISER0 |= ISER0_ISE_USB;

	// connect to bus
	USBHwConnect(TRUE);

	return 0;
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
	ISER0 |= ISER0_ISE_TIMER0;

	/* start timer */
	T0TCR = TCR_Counter_Enable;
}

/* Update CLKN. */
void TIMER0_IRQHandler()
{
	// Use non-volatile working register to shave off a couple instructions
	u32 next;

	if (T0IR & TIR_MR0_Interrupt) {

		clkn++;
		next = clkn;

		/* Trigger hop based on mode */

		/* NONE or SWEEP -> 100 Hz */
		if (hop_mode == HOP_NONE || hop_mode == HOP_SWEEP) {
			if ((next & 0x1f) == 0)
				do_hop = 1;
		}
		/* BLUETOOTH -> 1600 Hz */
		else if (hop_mode == HOP_BLUETOOTH) {
			if ((next & 0x1) == 0)
				do_hop = 1;
		}
		/* BLUETOOTH Low Energy -> 7.5ms - 4.0s in multiples of 1.25 ms */
		else if (hop_mode == HOP_BTLE) {
			if ((next & 0x3) == 0) {
				if (le_hop_after > 0 && --le_hop_after == 0) {
					do_hop = 1;
					le_hop_after = le_hop_interval;
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
	ISER0 &= ~ISER0_ISE_DMA;

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

	active_buf_clkn = clkn;
	/* reset interrupt counters */
	rx_tc = 0;
	rx_err = 0;
}

void DMA_IRQHandler()
{
	idle_buf_clkn = active_buf_clkn;
	active_buf_clkn = clkn;

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

static void dio_ssp_start()
{
	/* make sure the (active low) slave select signal is not active */
	DIO_SSEL_SET;

	/* enable rx DMA on DIO_SSP */
	DIO_SSP_DMACR |= SSPDMACR_RXDMAE;
	DIO_SSP_CR1 |= SSPCR1_SSE;
	
	/* enable DMA */
	DMACC0Config |= DMACCxConfig_E;
	ISER0 |= ISER0_ISE_DMA;

	/* activate slave select pin */
	DIO_SSEL_CLR;
}

static void dio_ssp_stop()
{
	; // TBD
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
	if (modulation == MOD_BT_BASIC_RATE) {
		cc2400_set(MANAND,  0x7fff);
		cc2400_set(LMTST,   0x2b22);
		cc2400_set(MDMTST0, 0x134b); // without PRNG
		cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
		cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
		cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	} else if (modulation == MOD_BT_LOW_ENERGY) {
		cc2400_set(MANAND,  0x7fff);
		cc2400_set(LMTST,   0x2b22);
		cc2400_set(MDMTST0, 0x134b); // without PRNG
		cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
		cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
		cc2400_set(MDMCTRL, 0x0040); // 250 kHz frequency deviation
	} else {
		/* oops */
		return;
	}

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

void cc2400_txtest()
{
#ifdef TX_ENABLE
	if (modulation == MOD_BT_BASIC_RATE) {
		cc2400_set(MANAND,  0x7fff);
		cc2400_set(LMTST,   0x2b22);
		cc2400_set(MDMTST0, 0x334b); // with PRNG
		cc2400_set(FSDIV,   channel);
		cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	} else if (modulation == MOD_BT_LOW_ENERGY) {
		cc2400_set(MANAND,  0x7fff);
		cc2400_set(LMTST,   0x2b22);
		cc2400_set(MDMTST0, 0x334b); // with PRNG
		cc2400_set(FSDIV,   channel);
		cc2400_set(MDMCTRL, 0x0040); // 250 kHz frequency deviation
	} else {
		/* oops */
		return;
	}
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
	cc2400_strobe(STX);
#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif
	mode = MODE_TX_TEST;
#endif
}

/*
 * This range test transmits a Bluetooth-like (but not Bluetooth compatible)
 * packet to a repeater and then receives the repeated packet.  It is only
 * useful if another device is running cc2400_repeater() some distance away.
 *
 * The outgoing packet is transmitted at each power amplifier (PA) level from
 * lowest to highest.  It is sent several times at each level.  The repeater
 * takes the first packet to be received correctly and repeats it in a similar
 * manner.  The result should indicate the lowest successful PA level in each
 * direction.
 *
 * The range test packet consists of:
 *   preamble: 4 bytes
 *   sync word: 4 bytes
 *   payload:
 *     length: 1 byte (21)
 *     packet type: 1 byte (0 = request; 1 = reply)
 *     LPC17xx serial number: 16 bytes
 *     request pa: 1 byte
 *     request number: 1 byte
 *     reply pa: 1 byte
 *     reply number: 1 byte
 *   crc: 2 bytes
 */

void cc2400_rangetest()
{
#ifdef TX_ENABLE
	u32 command[5];
	u32 result[5];
	int i;
	int j;
	u8 len = 22;
	u8 pa = 0;
	u8 txbuf[len];
	u8 rxbuf[len];

	mode = MODE_RANGE_TEST;

	txbuf[0] = len - 1; // length of data (rest of payload)
	txbuf[1] = 0; // request

	// read device serial number
	command[0] = 58;
	iap_entry(command, result);
	if ((result[0] & 0xFF) != 0) //status check
		return;
	txbuf[2] = (result[1] >> 24) & 0xFF;
	txbuf[3] = (result[1] >> 16) & 0xFF;
	txbuf[4] = (result[1] >> 8) & 0xFF;
	txbuf[5] = result[1] & 0xFF;
	txbuf[6] = (result[2] >> 24) & 0xFF;
	txbuf[7] = (result[2] >> 16) & 0xFF;
	txbuf[8] = (result[2] >> 8) & 0xFF;
	txbuf[9] = result[2] & 0xFF;
	txbuf[10] = (result[3] >> 24) & 0xFF;
	txbuf[11] = (result[3] >> 16) & 0xFF;
	txbuf[12] = (result[3] >> 8) & 0xFF;
	txbuf[13] = result[3] & 0xFF;
	txbuf[14] = (result[4] >> 24) & 0xFF;
	txbuf[15] = (result[4] >> 16) & 0xFF;
	txbuf[16] = (result[4] >> 8) & 0xFF;
	txbuf[17] = result[4] & 0xFF;

	txbuf[18] = pa; // request pa
	txbuf[19] = 0; // request number
	txbuf[20] = 0xff; // reply pa
	txbuf[21] = 0xff; // reply number

	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b);
	cc2400_set(FSDIV,   channel);
	cc2400_set(SYNCH,   0xf9ae);
	cc2400_set(SYNCL,   0x1584);
	cc2400_set(FREND,   8 | pa);
	cc2400_set(MDMCTRL, 0x0029);
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif
	for (pa = 0; pa < 8; pa++) {
		cc2400_set(FREND, 8 | pa);
		txbuf[18] = pa;
		for (i = 0; i < 16; i++) {
			txbuf[19] = i;
			while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
			// transmit a packet
			for (j = 0; j < len; j++)
				cc2400_set8(FIFOREG, txbuf[j]);
			cc2400_strobe(STX);
		}
	}
	// sent packet, now look for repeated packet
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	TXLED_CLR;
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));
	cc2400_set(FSDIV, channel - 1);
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	RXLED_SET;
	while (1) {
		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		cc2400_strobe(SRX);
		while (!(cc2400_status() & SYNC_RECEIVED));
		USRLED_SET;
		for (j = 0; j < len; j++)
			rxbuf[j] = cc2400_get8(FIFOREG);
		if (cc2400_status() & STATUS_CRC_OK)
			break;
		USRLED_CLR;
	}

	// done
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));
#ifdef UBERTOOTH_ONE
	PAEN_CLR;
#endif
	RXLED_CLR;

	// get test result
	rr.valid       = 1;
	rr.request_pa  = rxbuf[18];
	rr.request_num = rxbuf[19];
	rr.reply_pa    = rxbuf[20];
	rr.reply_num   = rxbuf[21];

	// make sure rx packet is as expected
	txbuf[1] = 1; // expected value in rxbuf
	for (i = 0; i < 18; i++)
		if (rxbuf[i] != txbuf[i])
			rr.valid = 0;

	USRLED_CLR;
	mode = MODE_IDLE;
	if (requested_mode == MODE_RANGE_TEST)
		requested_mode = MODE_IDLE;
#endif
}

/* This is the repeater implementation to be used with cc2400_rangetest(). */
void cc2400_repeater()
{
#ifdef TX_ENABLE
	int i;
	int j;
	u8 len = 22;
	u8 pa = 0;
	u8 buf[len];

	mode = MODE_REPEATER;

	//FIXME allow to be turned off
	while (1) {
		cc2400_set(MANAND,  0x7fff);
		cc2400_set(LMTST,   0x2b22);
		cc2400_set(MDMTST0, 0x134b);
		cc2400_set(FSDIV,   channel - 1);
		cc2400_set(SYNCH,   0xf9ae);
		cc2400_set(SYNCL,   0x1584);
		cc2400_set(FREND,   0x0008); // minimum tx power
		cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
		while (!(cc2400_status() & XOSC16M_STABLE));
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));
		RXLED_SET;
		TXLED_CLR;
		USRLED_CLR;
#ifdef UBERTOOTH_ONE
		PAEN_SET;
#endif
		while (1) {
			while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
			USRLED_CLR;
			cc2400_strobe(SRX);
			while (!(cc2400_status() & SYNC_RECEIVED));
			USRLED_SET;
			for (i = 0; i < len; i++)
				buf[i] = cc2400_get8(FIFOREG);
			if (cc2400_status() & STATUS_CRC_OK)
				break;
		}
		// got packet, now repeat it
		i = 2000000; while (--i); // allow time for requester to switch to rx
		USRLED_CLR;
		RXLED_CLR;
		cc2400_strobe(SRFOFF);
		while ((cc2400_status() & FS_LOCK));
		while (!(cc2400_status() & XOSC16M_STABLE));
		cc2400_set(FSDIV, channel);
		while (!(cc2400_status() & XOSC16M_STABLE));
		cc2400_strobe(SFSON);
		TXLED_SET;
		buf[0] = len - 1; // length of data (rest of payload)
		buf[1] = 1; // reply
		for (pa = 0; pa < 8; pa++) {
			cc2400_set(FREND, 8 | pa);
			buf[20] = pa;
			for (i = 0; i < 16; i++) {
				buf[21] = i;
				while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
					for (j = 0; j < len; j++)
						cc2400_set8(FIFOREG, buf[j]);
				cc2400_strobe(STX);
			}
		}
		TXLED_CLR;
		cc2400_strobe(SRFOFF);
		while ((cc2400_status() & FS_LOCK));
	}
#endif
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
		channel += 1;
		if (channel > 2480)
			channel = 2402;
	}

	else if (hop_mode == HOP_BLUETOOTH) {
		TXLED_SET;
		channel = next_hop(clkn);
	}

	else if (hop_mode == HOP_BTLE) {
		TXLED_SET;
		channel = btle_next_hop();
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

static void handle_usb(void)
{
	u8 epstat;

	/* write queued packets to USB if possible */
	epstat = USBHwEPGetStatus(BULK_IN_EP);
	if (!(epstat & EPSTAT_B1FULL)) {
		dequeue_send();
	}
	if (!(epstat & EPSTAT_B2FULL)) {
		dequeue_send();
	}

	/* polled "interrupt" */
	USBHwISR();
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
						if (enqueue(idle_rxbuf)) {
								RXLED_SET;
								--rx_pkts;
						}
				break;

			case HOP_BTLE:
				if (btle_find_access_address(idle_rxbuf))
						if (enqueue(idle_rxbuf)) {
								RXLED_SET;
								--rx_pkts;
						}
				break;

			default:
				if (enqueue(idle_rxbuf)) {
						RXLED_SET;
						--rx_pkts;
				}
		}

	rx_continue:
		handle_usb();
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
	cc2400_rx();

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

		/* Queue data from DMA buffer. */
		if ((packet_offset = find_access_code(idle_rxbuf)) >= 0) {
			clock_trim = 20 - packet_offset;
			if (enqueue(idle_rxbuf)) {
				RXLED_SET;
				--rx_pkts;
			}
		}

	rx_continue:
		handle_usb();
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

/* generic le mode */
void bt_generic_le(u8 active_mode)
{
	u8 *tmp = NULL;
	u8 hold;
	int i, j;

	mode = active_mode;

	// enable USB interrupts
	ISER0 |= ISER0_ISE_USB;

	RXLED_CLR;

	queue_init();
	dio_ssp_init();
	dma_init();
	dio_ssp_start();
	cc2400_rx();

	cs_trigger_enable();

	hold = 0;

	while (requested_mode == active_mode) {

		if (do_hop) {
			hop();
		} else {
			TXLED_CLR;
		}

		RXLED_CLR;

		/* Wait for DMA transfer. */
		while ((rx_tc == 0) && (rx_err == 0))
			;

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

		data_cb(unpacked);

	rx_continue:
		rx_tc = 0;
		rx_err = 0;
	}

	dio_ssp_stop();
	cs_trigger_disable();
}

/* low energy connection following
 * follows a known AA around */
void cb_follow_le() {
	int i, j, k;
	int idx = whitening_index[btle_channel_index(channel-2402)];

	u32 access_address = 0;
	for (i = 32; i < (DMA_SIZE*8 + 32); i++) {
		access_address >>= 1;
		access_address |= (unpacked[i] << 31);
		if (access_address == desired_address) {
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
			if (crc_verify) {
				int len		 = (idle_rxbuf[5] & 0x3f) + 2;
				u32 calc_crc = btle_calc_crc(crc_init_reversed, idle_rxbuf + 4, len);
				u32 wire_crc = (idle_rxbuf[4+len+2] << 16)
							 | (idle_rxbuf[4+len+1] << 8)
							 |  idle_rxbuf[4+len+0];
				if (calc_crc != wire_crc) // skip packets with a bad CRC
					break;
			}

			// send to PC
			enqueue(idle_rxbuf);
			RXLED_SET;
			--rx_pkts;

			packet_cb(idle_rxbuf);

			break;
		}
	}
}

/**
 * Called when we recieve a packet in connection following mode.
 */
void connection_follow_cb(u8 *packet) {
	int i;

	if (le_connected) {
		// hop (8 * 1.25) = 10 ms after we see a packet on this channel
		if (le_hop_after == 0)
			le_hop_after = le_hop_interval;
	}

	// connect packet
	if (!le_connected && packet[4] == 0x05) {
		le_connected = 1;

		desired_address = 0;
		for (i = 0; i < 4; ++i)
			desired_address |= packet[18+i] << (i*8);

#define CRC_INIT (2+4+6+6+4)
		crc_init = (packet[CRC_INIT+2] << 16)
				 | (packet[CRC_INIT+1] << 8)
				 |  packet[CRC_INIT+0];
		crc_init_reversed = 0;
		for (i = 0; i < 24; ++i)
			crc_init_reversed |= ((crc_init >> i) & 1) << (23 - i);

#define WIN_OFFSET (2+4+6+6+4+3+1)
		// le_hop_after = idle_rxbuf[WIN_OFFSET]-2;
		do_hop = 1;

#define HOP_INTERVAL (2+4+6+6+4+3+1+2)
		le_hop_interval = idle_rxbuf[HOP_INTERVAL];

#define HOP (2+4+6+6+4+3+1+2+2+2+2+5)
		le_hop_amount = packet[HOP] & 0x1f;
		le_channel_idx = le_hop_amount;
	}
}

void bt_follow_le() {
	data_cb = cb_follow_le;
	packet_cb = connection_follow_cb;
	bt_generic_le(MODE_BT_FOLLOW_LE);
}

// divide, rounding to the nearest integer: round up at 0.5.
#define DIVIDE_ROUND(N, D) ((N) + (D)/2) / (D)

void promisc_recover_hop_amount(u8 *packet) {
	static u32 first_ts = 0;
	if (channel == 2404) {
		first_ts = CLK100NS;
		hop_direct_channel = 2406;
		do_hop = 1;
	} else if (channel == 2406) {
		u32 second_ts = CLK100NS;
		// number of channels hopped between previous and current
		u32 channels_hopped = DIVIDE_ROUND(second_ts - first_ts,
										   le_hop_interval * LE_BASECLK);
		if (channels_hopped < 37) {
			// get the hop amount based on the number of channels hopped
			le_hop_amount = hop_interval_lut[channels_hopped];
			le_channel_idx = 1;
			le_hop_after = 0;
			le_connected = 1;

			// move on to regular connection following!
			hop_mode = HOP_BTLE;
			do_hop = 0;
			packet_cb = connection_follow_cb;

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
	u32 obsv_hop_interval; // observed hop interval

	// probably consecutive data packets on the same channel
	if (clk_diff < 2 * LE_BASECLK)
		return;

	if (clk_diff < smallest_interval)
		smallest_interval = clk_diff;

	obsv_hop_interval = DIVIDE_ROUND(smallest_interval, 37 * LE_BASECLK);

	if (le_hop_interval == obsv_hop_interval) {
		// 5 consecutive hop intervals: consider it legit and move on
		++consec;
		if (consec == 5) {
			packet_cb = promisc_recover_hop_amount;
			hop_direct_channel = 2404;
			hop_mode = HOP_DIRECT;
			do_hop = 1;
		}
	} else {
		le_hop_interval = obsv_hop_interval;
		consec = 0;
	}

	prev_clk = cur_clk;
}

void promisc_follow_cb(u8 *packet) {
	int i;

	// get the CRCInit
	if (!crc_verify && packet[4] == 0x01 && packet[5] == 0x00) {
		u32 crc = (packet[8] << 16) | (packet[7] << 8) | packet[6];

		crc_init = btle_reverse_crc(crc, packet + 4, 2);
		crc_init_reversed = 0;
		for (i = 0; i < 24; ++i)
			crc_init_reversed |= ((crc_init >> i) & 1) << (23 - i);

		crc_verify = 1;
		packet_cb = promisc_recover_hop_interval;
	}
}

// LFU cache of recently seen AA's
struct active_aa {
	u32 aa;
	int freq;
} active_aa_list[10] = { { 0, 0, }, };
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
void cb_le_promisc(char *unpacked) {
	int i, j, k;

	// empty data PDU: 01 00
	char desired[] = { 1, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, };
	int idx = whitening_index[btle_channel_index(channel-2402)];

	// whiten the desired data
	for (i = 0; i < (int)sizeof(desired); ++i) {
		desired[i] ^= whitening[idx];
		idx = (idx + 1) % sizeof(whitening);
	}

	// then look for that bitsream in our receive buffer
	for (i = 32; i < (DMA_SIZE*8 + 32); i++) {
		int ok = 1;
		for (j = 0; j < (int)sizeof(desired); ++j) {
			if (unpacked[i+j] != desired[j]) {
				ok = 0;
				break;
			}
		}

		// no match, move along
		if (!ok) continue;

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

		enqueue(idle_rxbuf);

	}

	// once we see an AA 5 times, start following it
	for (i = 0; i < AA_LIST_SIZE; ++i) {
		if (active_aa_list[i].freq > 5) {
			desired_address = active_aa_list[i].aa;
			data_cb = cb_follow_le;
			packet_cb = promisc_follow_cb;
			crc_verify = 0;
		}
	}
}

void bt_promisc_le() {
	data_cb = cb_le_promisc;
	bt_generic_le(MODE_BT_PROMISC_LE);
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
	cc2400_set(MANAND,  0x7fff);
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
				//FIXME ought to use different packet type
				enqueue(buf);
				i = 0;

				handle_usb();
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
	cc2400_set(MANAND,  0x7fff);
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

		USBHwISR();
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
	ubertooth_usb_init();

	while (1) {
		USBHwISR();
		if (rx_pkts
		    && requested_mode == MODE_RX_SYMBOLS
		    && mode != MODE_RX_SYMBOLS )
			bt_stream_rx();
		else if (requested_mode == MODE_BT_FOLLOW && mode != MODE_BT_FOLLOW)
			bt_follow();
		else if (requested_mode == MODE_BT_FOLLOW_LE && mode != MODE_BT_FOLLOW_LE)
			bt_follow_le();
		else if (requested_mode == MODE_BT_PROMISC_LE && mode != MODE_BT_PROMISC_LE)
			bt_promisc_le();
		else if (requested_mode == MODE_TX_TEST && mode != MODE_TX_TEST)
			cc2400_txtest();
		else if (requested_mode == MODE_RANGE_TEST && mode != MODE_RANGE_TEST)
			cc2400_rangetest();
		else if (requested_mode == MODE_REPEATER && mode != MODE_REPEATER)
			cc2400_repeater();
		else if (requested_mode == MODE_SPECAN && mode != MODE_SPECAN)
			specan();
		else if (requested_mode == MODE_LED_SPECAN && mode != MODE_LED_SPECAN)
			led_specan();
		else if (requested_mode == MODE_IDLE && mode != MODE_IDLE)
			cc2400_idle();
		//FIXME do other modes like this
	}
}
