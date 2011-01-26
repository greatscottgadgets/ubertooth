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
 * CLK100NS is a free-running clock with a period of 100ns.  It resets every
 * 2^15 * 10^5 cycles (about 5.5 minutes) and is used to compute CLKN.
 *
 * CLKN is the native (local) clock as defined in the Bluetooth specification.
 * It advances 3200 times per second.  Two CLKN periods make a Bluetooth time
 * slot.
 */

#define CLK100NS T0TC
volatile u8 clkn_high;
#define CLKN ((clkn_high << 20) | (CLK100NS / 3125))

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
	UBERTOOTH_PING        = 0,
	UBERTOOTH_RX_SYMBOLS  = 1,
	UBERTOOTH_TX_SYMBOLS  = 2, /* not implemented */
	UBERTOOTH_GET_USRLED  = 3,
	UBERTOOTH_SET_USRLED  = 4,
	UBERTOOTH_GET_RXLED   = 5,
	UBERTOOTH_SET_RXLED   = 6,
	UBERTOOTH_GET_TXLED   = 7,
	UBERTOOTH_SET_TXLED   = 8,
	UBERTOOTH_GET_1V8     = 9,
	UBERTOOTH_SET_1V8     = 10,
	UBERTOOTH_GET_CHANNEL = 11, /* not implemented */
	UBERTOOTH_SET_CHANNEL = 12, /* not implemented */
	UBERTOOTH_RESET       = 13, /* not implemented */
	UBERTOOTH_GET_SERIAL  = 14,
	UBERTOOTH_GET_PARTNUM = 15
};

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
	u8     reserved[6];
	u8     data[DMA_SIZE];
} usb_pkt_rx;

/*
 * This is supposed to be a lock-free ring buffer, but I haven't verified
 * atomicity of the operations on head and tail.
 */

usb_pkt_rx fifo[256];

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
	u8 h = head & 0xFF;
	u8 t = tail & 0xFF;
	u8 n = (t + 1) & 0xFF;

	/* fail if queue is full */
	if (h == n)
		return 0;

	fifo[t].clkn_high = clkn_high;
	fifo[t].clk100ns = CLK100NS;

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
	u8 h = head & 0xFF;
	u8 t = tail & 0xFF;

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
	 * and in timer mode.  The TIMER0 peripheral clock should have been set to
	 * cclk/2 (50 MHz) by clock_start().
	 */

	/* stop and reset the timer to zero */
	T0TCR = TCR_Counter_Reset;
	clkn_high = 0;

	/*
	 * The peripheral clock has a period of 20ns.  5 pclk periods makes one
	 * CLK100NS period (100ns).  CLK100NS resets every 2^15 * 10^5 (3276800000)
	 * steps, roughly 5.5 minutes.
	 */
	T0PR = 4;
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

/* start un-buffered bluetooth rx */
void cc2400_rx()
{
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
	cc2400_set(FSDIV,   0x0988); // 2440 MHz + 1 MHz IF = 2441 MHz
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
}

void cc2400_txtest()
{
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x334b); // with PRNG
	cc2400_set(FSDIV,   0x0989); // 2441 MHz
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(STX);
}

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
	TXLED_SET;

	while (rx_pkts) {
		/* wait for DMA transfer */
		while ((rx_tc == 0) && (rx_err == 0));
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

int main()
{
	ubertooth_init();
	clkn_init();
	ubertooth_usb_init();

	while (1) {
		USBHwISR();
		if (rx_pkts)
			bt_stream_rx();
	}
}
