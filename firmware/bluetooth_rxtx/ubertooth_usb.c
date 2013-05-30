/*
 * Copyright 2010 Michael Ossmann
 * Copyright 2013 Dominic Spill
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

#include "usbapi.h"
#include "usbhw_lpc.h"
#include "ubertooth_usb.h"

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

#define BULK_IN_EP		0x82
#define BULK_OUT_EP		0x05

#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

/*
 * This is supposed to be a lock-free ring buffer, but I haven't verified
 * atomicity of the operations on head and tail.
 */

const u8 abDescriptors[] = {

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


u8 abVendorReqData[258];

/* Unused functions
void usb_bulk_in_handler(u8 bEP, u8 bEPStatus)
{
	if (!(bEPStatus & EP_STATUS_DATA))
		dequeue_send();
}

void usb_bulk_out_handler(u8 bEP, u8 bEPStatus)
{
}
*/

VendorRequestHandler *v_req_handler;

BOOL usb_vendor_request_handler(TSetupPacket *pSetup, int *piLen, u8 **ppbData)
{
	int rv;
	u16 params[2] = {pSetup->wValue, pSetup->wIndex};
	rv = v_req_handler(pSetup->bRequest, params, *ppbData, piLen);
	return (BOOL) (rv==1);
}

int ubertooth_usb_init(VendorRequestHandler *vendor_req_handler)
{
	// initialise stack
	USBInit();
	
	// register device descriptors
	USBRegisterDescriptors(abDescriptors);

	// Request handler 
	v_req_handler = vendor_req_handler;

	// override standard request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_VENDOR, usb_vendor_request_handler, abVendorReqData);

	// register endpoints
	//USBHwRegisterEPIntHandler(BULK_IN_EP, usb_bulk_in_handler);
	//USBHwRegisterEPIntHandler(BULK_OUT_EP, usb_bulk_out_handler);

	// enable USB interrupts
	//ISER0 = ISER0_ISE_USB;

	// connect to bus
	USBHwConnect(TRUE);

	return 0;
}

usb_pkt_rx fifo[128];

volatile u32 head = 0;
volatile u32 tail = 0;

void queue_init()
{
	head = 0;
	tail = 0;
}

usb_pkt_rx *usb_enqueue()
{
	u8 h = head & 0x7F;
	u8 t = tail & 0x7F;
	u8 n = (t + 1) & 0x7F;

	/* fail if queue is full */
	if (h == n) {
		return NULL;
	}

	++tail;
	return &fifo[t];
	
}

usb_pkt_rx *dequeue()
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
u32 last_usb_pkt = 0;  // for keep alive packets

int dequeue_send(u32 clkn)
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

void handle_usb(u32 clkn)
{
	u8 epstat;

	/* write queued packets to USB if possible */
	epstat = USBHwEPGetStatus(BULK_IN_EP);
	if (!(epstat & EPSTAT_B1FULL)) {
		dequeue_send(clkn);
	}
	if (!(epstat & EPSTAT_B2FULL)) {
		dequeue_send(clkn);
	}

	/* polled "interrupt" */
	USBHwISR();
}
