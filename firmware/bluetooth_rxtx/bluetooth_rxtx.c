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

#include "ubertooth.h"
#include "usbapi.h"
#include "usbhw_lpc.h"

#define IAP_LOCATION 0x1FFF1FF1
typedef void (*IAP)(u32[], u32[]);
IAP iap_entry = (IAP)IAP_LOCATION;

/*
 * CLK100NS is a free-running clock with a period of 100 ns.  It resets every
 * 2^15 * 10^5 cycles (about 5.5 minutes) and is used to compute CLKN.
 *
 * CLKN is the native (local) clock as defined in the Bluetooth specification.
 * It advances 3200 times per second.  Two CLKN periods make a Bluetooth time
 * slot.
 */

#define CLK100NS T0TC
volatile u8 clkn_high;
#define CLKN ((clkn_high << 20) | (CLK100NS / 3125))

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
#define DMA_SIZE 50
u8 rxbuf1[DMA_SIZE];
u8 rxbuf2[DMA_SIZE];

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
u8 *active_rxbuf = &rxbuf1[0];
u8 *idle_rxbuf = &rxbuf2[0];

enum ubertooth_usb_commands {
	UBERTOOTH_PING         = 0,
	UBERTOOTH_RX_SYMBOLS   = 1,
	UBERTOOTH_TX_SYMBOLS   = 2, /* not implemented */
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
	BOOTLOADER_FLASH       = 26, /* do not implement */
	UBERTOOTH_SPECAN       = 27,
	UBERTOOTH_GET_PALEVEL  = 28,
	UBERTOOTH_SET_PALEVEL  = 29,
	UBERTOOTH_REPEATER     = 30,
	UBERTOOTH_RANGE_TEST   = 31,
	UBERTOOTH_RANGE_CHECK  = 32,
	UBERTOOTH_GET_REV_NUM  = 33,
	UBERTOOTH_LED_SPECAN   = 34,
	UBERTOOTH_GET_BOARD_ID = 35
};

enum operating_modes {
	MODE_IDLE       = 0,
	MODE_RX_SYMBOLS = 1,
	MODE_TX_SYMBOLS = 2,
	MODE_TX_TEST    = 3,
	MODE_SPECAN     = 4,
	MODE_RANGE_TEST = 5,
	MODE_REPEATER   = 6,
	MODE_LED_SPECAN = 7
};

enum modulations {
	MOD_BT_BASIC_RATE = 0,
	MOD_BT_LOW_ENERGY = 1,
	MOD_80211_FHSS    = 2
};

typedef struct {
	u8 valid;
	u8 request_pa;
	u8 request_num;
	u8 reply_pa;
	u8 reply_num;
} rangetest_result;

rangetest_result rr;

volatile u32 mode = MODE_IDLE;
volatile u32 requested_mode = MODE_IDLE;
volatile u32 modulation = MOD_BT_BASIC_RATE;
volatile u16 channel = 2441;
volatile u16 low_freq = 2400;
volatile u16 high_freq = 2483;
volatile u8 rssi_threshold = 225;

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

/*
 * USB packet for Bluetooth RX (64 total bytes)
 */
typedef struct {
	u8     pkt_type;
	u8     status;
	u8     channel;
	u8     clkn_high;
	u32    clk100ns;
	char   rssi_max;   // Max RSSI seen while collecting symbols in this packet
	char   rssi_min;   // Min ...
    char   rssi_avg;   // Average ...
	u8     rssi_count; // Number of ... (0 means RSSI stats are invalid)
	u8     reserved[2];
	u8     data[DMA_SIZE];
} usb_pkt_rx;

/*
 * RSSI
 */
int8_t rssi_max;
int8_t rssi_min;
uint8_t rssi_count = 0;
int16_t rssi_sum = 0;

static void rssi_reset(void)
{
	rssi_count = 0;
	rssi_sum = 0;
}

static void rssi_add(int8_t v)
{
	if (rssi_count == 0) {
		rssi_max = v;
		rssi_min = v;
	}
	else {
		rssi_max = (v > rssi_max) ? v : rssi_max;
		rssi_min = (v < rssi_min) ? v : rssi_min;
	}
	rssi_sum += v;
	rssi_count += 1;
}

static int8_t rssi_avg(void)
{
	return rssi_sum / rssi_count;
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

int enqueue(u8 *buf)
{
	int i;
	u8 h = head & 0x7F;
	u8 t = tail & 0x7F;
	u8 n = (t + 1) & 0x7F;

	usb_pkt_rx *f = &fifo[t];

	/* fail if queue is full */
	if (h == n)
		return 0;

	f->clkn_high = clkn_high;
	f->clk100ns = CLK100NS;
	f->channel = channel-2402;
	f->rssi_min = rssi_min;
	f->rssi_max = rssi_max;
	f->rssi_avg = rssi_avg();
	f->rssi_count = rssi_count;
	rssi_reset();

	USRLED_SET;
	for (i = 0; i < DMA_SIZE; i++)
		fifo[t].data[i] = buf[i];
	fifo[t].status = status;
	status = 0;
	++tail;

	return 1;
}

int dequeue()
{
	u8 h = head & 0x7F;
	u8 t = tail & 0x7F;

	/* fail if queue is empty */
	if (h == t) {
		USRLED_CLR;
		return 0;
	}

	USBHwEPWrite(BULK_IN_EP, (u8 *)&fifo[h], sizeof(usb_pkt_rx));
	++head;

	return 1;
}

static const u8 abDescriptors[] = {

/* Device descriptor */
	0x12,              		
	DESC_DEVICE,       		
	LE_WORD(0x0200),		// bcdUSB	
	0xFF,              		// bDeviceClass
	0x00,              		// bDeviceSubClass
	0x00,              		// bDeviceProtocol
	MAX_PACKET_SIZE0,  		// bMaxPacketSize
	LE_WORD(0xFFFF),		// idVendor
	LE_WORD(0x0004),		// idProduct
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

static u8 abVendorReqData[17];

static void usb_bulk_in_handler(u8 bEP, u8 bEPStatus)
{
	if (!(bEPStatus & EP_STATUS_DATA))
		dequeue();
}

static void usb_bulk_out_handler(u8 bEP, u8 bEPStatus)
{
}

static BOOL usb_vendor_request_handler(TSetupPacket *pSetup, int *piLen, u8 **ppbData)
{
	u8 *pbData = *ppbData;
	u32 command[5];
	u32 result[5];

	switch (pSetup->bRequest) {

	case UBERTOOTH_PING:
		*piLen = 0;
		break;

	case UBERTOOTH_RX_SYMBOLS:
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
		channel = MAX(channel, MIN_FREQ);
		channel = MIN(channel, MAX_FREQ);
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
		rssi_threshold = pSetup->wValue;
		requested_mode = MODE_LED_SPECAN;
		*piLen = 0;
		break;

	case UBERTOOTH_GET_REV_NUM:
		pbData[0] = SVN_REV_NUM & 0xFF;
		pbData[1] = (SVN_REV_NUM >> 8) & 0xFF;
		*piLen = 2;
		break;

	case UBERTOOTH_GET_BOARD_ID:
		pbData[0] = BOARD_ID;
		*piLen = 1;
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

int ubertooth_usb_init()
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
	clkn_high = 0;

#ifdef TC13BADGE
	/*
	 * The peripheral clock has a period of 33.3ns.  3 pclk periods makes one
	 * CLK100NS period (100 ns).  CLK100NS resets every 2^15 * 10^5
	 * (3276800000) steps, roughly 5.5 minutes.
	 */
	T0PR = 2;
#else
	/*
	 * The peripheral clock has a period of 20ns.  5 pclk periods makes one
	 * CLK100NS period (100 ns).  CLK100NS resets every 2^15 * 10^5
	 * (3276800000) steps, roughly 5.5 minutes.
	 */
	T0PR = 4;
#endif
	T0MR0 = 3276799999;
	T0MCR = TMCR_MR0R | TMCR_MR0I;
	ISER0 |= ISER0_ISE_TIMER0;

	/* start timer */
	T0TCR = TCR_Counter_Enable;
}

/* clkn_high is incremented each time CLK100NS rolls over */
void TIMER0_IRQHandler()
{
	if (T0IR & TIR_MR0_Interrupt) {
		++clkn_high;
		T0IR |= TIR_MR0_Interrupt;
	}
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

	/* reset interrupt counters */
	rx_tc = 0;
	rx_err = 0;
}

void DMA_IRQHandler()
{
	/* interrupt on channel 0 */
	if (DMACIntStat & (1 << 0)) {
		if (DMACIntTCStat & (1 << 0)) {
			DMACIntTCClear |= (1 << 0);
			++rx_tc;
		}
		if (DMACIntErrStat & (1 << 0)) {
			DMACIntErrClr |= (1 << 0);
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

/* start un-buffered rx */
void cc2400_rx()
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
	cc2400_set(RSSI, 0x3f); // RSSI - default CS, 8-symbol averaging
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

/* single channel Bluetooth monitoring */
void bt_stream_rx()
{
	u8 *tmp = NULL;
	u8 epstat;
	int i;

	RXLED_SET;

	queue_init();
	dio_ssp_init();
	dma_init();
	dio_ssp_start();
	cc2400_rx();

	while (rx_pkts) {

		/* wait for DMA transfer */
		while ((rx_tc == 0) && (rx_err == 0)) {
			/* take as many rssi readings as possible while waiting */
			rssi_add(cc2400_get(RSSI) >> 8);
		}
		if (rx_tc % 2) {
			/* swap buffers */
			tmp = active_rxbuf;
			active_rxbuf = idle_rxbuf;
			idle_rxbuf = tmp;
		}
		if (rx_err)
			status |= DMA_ERROR;
		if (rx_tc) {
			if (rx_tc > 1)
				status |= DMA_OVERFLOW;
			if (enqueue(idle_rxbuf))
				--rx_pkts;
			else
				status |= FIFO_OVERFLOW;
		}

		/* send via USB */
		epstat = USBHwEPGetStatus(BULK_IN_EP);
		if (!(epstat & EPSTAT_B1FULL))
			dequeue();
		if (!(epstat & EPSTAT_B2FULL))
			dequeue();
		USBHwISR();

		rx_tc = 0;
		rx_err = 0;
	}
	//FIXME turn off rx
	RXLED_CLR;
}

/*
 * This is like bt_stream_rx except it hops along with a target piconet.  This
 * does not do any packet detection; it just tunes the radio 1600 times per
 * second to hop along with a target and streams as many raw symbols per hop
 * (many of which represent background noise, not actual packets) as possible.
 *
 * FIXME This is mostly copied from elsewhere and is incomplete.
 */
void bt_hop_rx()
{
	u8 *tmp = NULL;
	u8 epstat;
	int i;

	RXLED_SET;

	queue_init();
	dio_ssp_init();
	dma_init();
	dio_ssp_start();
	cc2400_rx();

	// could use T0MR1 interrupts
	//T0MR1 = ;
	//T0MCR = TMCR_MR1R | TMCR_MR1I;
	//
	// probably should just take a 400 symbol chunk instead and tune/wait after
	// that

	while (rx_pkts) {
		/* wait for DMA transfer */
		while ((rx_tc == 0) && (rx_err == 0)) {
			/* take as many rssi readings as possible while waiting */
			rssi_add(cc2400_get(RSSI) >> 8);
		}
		if (rx_tc % 2) {
			/* swap buffers */
			tmp = active_rxbuf;
			active_rxbuf = idle_rxbuf;
			idle_rxbuf = tmp;
		}
		if (rx_err)
			status |= DMA_ERROR;
		if (rx_tc) {
			if (rx_tc > 1)
				status |= DMA_OVERFLOW;
			if (enqueue(idle_rxbuf))
				--rx_pkts;
			else
				status |= FIFO_OVERFLOW;
		}

		/* send via USB */
		epstat = USBHwEPGetStatus(BULK_IN_EP);
		if (!(epstat & EPSTAT_B1FULL))
			dequeue();
		if (!(epstat & EPSTAT_B2FULL))
			dequeue();
		USBHwISR();

		rx_tc = 0;
		rx_err = 0;
	}
	//FIXME turn off rx
	RXLED_CLR;
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

			//u32 j = 100; while (--j); //FIXME crude delay
			buf[3 * i] = (f >> 8) & 0xFF;
			buf[(3 * i) + 1] = f  & 0xFF;
			buf[(3 * i) + 2] = cc2400_get(RSSI) >> 8;
			i++;
			if (i == 16) {
				//FIXME ought to use different packet type
				enqueue(buf);
				i = 0;
				/* send via USB */
				epstat = USBHwEPGetStatus(BULK_IN_EP);
				if (!(epstat & EPSTAT_B1FULL))
					dequeue();
				if (!(epstat & EPSTAT_B2FULL))
					dequeue();
				USBHwISR();
			}

			cc2400_strobe(SRFOFF);
			while ((cc2400_status() & FS_LOCK));
		}
	}
	mode = MODE_IDLE;
	RXLED_CLR;
}

void cc2400_idle()
{
	clock_start();
	mode = MODE_IDLE;
	RXLED_CLR;
	TXLED_CLR;
}

/* LED based spectrum analysis */
void led_specan()
{
	u8 lvl;
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
		if (rx_pkts)
			bt_stream_rx();
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
