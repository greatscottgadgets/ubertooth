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

#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

/* Ubertooth Zero
 * pin expansion connector (J1) pin 13 which is GPIO P2.7, configure
 * pull-up and check if it has been jumpered to ground (pin 1)

 * On Ubertooth One:

* expansion connector (P4) pin 3 which is GPIO P0.22, same
* configuration, ground is at pin 1.
*/

#ifdef UBERTOOTH_ZERO
#define ENTRY_PIN PINSEL4
#define ENTRY_SET PINSEL4_P2_7
#endif
#ifdef UBERTOOTH_ONE
#define ENTRY_PIN PINSEL1
#define ENTRY_SET PINSEL1_P0_22
#endif

enum dfu_usb_commands {
	DFU_DETACH     = 0,
	DFU_DNLOAD     = 1,
	DFU_UPLOAD     = 2,
	DFU_GETSTATUS  = 3,
	DFU_CLRSTATUS  = 4,
	DFU_GETSTATE   = 5,
	DFU_ABORT      = 6
};

static const u8 abDescriptors[] = {

/* Device descriptor */
	0x12,              		
	DESC_DEVICE,       		
	LE_WORD(0x0200),		// bcdUSB	
	0x00,              		// bDeviceClass
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
	0x00,   				// bNumEndPoints
	0xFE,   				// bInterfaceClass
	0x01,   				// bInterfaceSubClass
	0x02,   				// bInterfaceProtocol
	0x00,   				// iInterface

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
	'u', 0, 's', 0, 'b', 0, '_', 0, 'b', 0, 'o', 0, 'o', 0, 't', 0, 'l', 0, 'o', 0,
	'a', 0, 'd', 0, 'e', 0, 'r', 0,

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
		;
}

static BOOL usb_vendor_request_handler(TSetupPacket *pSetup, int *piLen, u8 **ppbData)
{
	u8 *pbData = *ppbData;
	u32 command[5];
	u32 result[5];

	switch (pSetup->bRequest) {


	case DFU_DETACH:
		break;

	case DFU_DNLOAD:
		break;

	case DFU_UPLOAD:
		break;

	case DFU_GETSTATUS:
		break;

	case DFU_CLRSTATUS:
		break;

	case DFU_GETSTATE:
		break;

	case DFU_ABORT:
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

int bootloader_usb_init()
{
	// initialise stack
	USBInit();
	
	// register device descriptors
	USBRegisterDescriptors(abDescriptors);

	// override standard request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_VENDOR, usb_vendor_request_handler, abVendorReqData);

	// enable USB interrupts
	//ISER0 |= ISER0_ISE_USB;
	
	// connect to bus
	USBHwConnect(TRUE);

	return 0;
}

// For testing
void blink()
{
	USRLED_SET;
	TXLED_SET;
	RXLED_SET;
	wait(1);
	USRLED_CLR;
	TXLED_CLR;
	RXLED_CLR;
	wait(1);
}

static void run_bootloader()
{
	bootloader_usb_init();

	while (1) {
		USBHwISR();
	}
	
}

void main(void)
{
	ENTRY_PIN |= ENTRY_SET;
	
	if (ENTRY_PIN & ENTRY_SET)
		;
	else
		run_bootloader();

}
