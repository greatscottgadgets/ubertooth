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

#include <stdint.h>

extern "C" {
#include <ubertooth.h>
#include "usbapi.h"
#include "usbhw_lpc.h"
}

#include "dfu.h"

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

static const uint16_t usb_vendor_id = 0xFFFF;
static const uint16_t usb_product_id = 0x0004;

static const u8 dfu_descriptors[] = {

/* Device descriptor */
	0x12,              		
	DESC_DEVICE,       		
	LE_WORD(0x0200),		// bcdUSB	
	0x00,              		// bDeviceClass
	0x00,              		// bDeviceSubClass
	0x00,              		// bDeviceProtocol
	MAX_PACKET_SIZE0,  		// bMaxPacketSize
	LE_WORD(usb_vendor_id),		// idVendor
	LE_WORD(usb_product_id),	// idProduct
	LE_WORD(0x0100),		// bcdDevice
	0x01,              		// iManufacturer
	0x02,              		// iProduct
	0x03,              		// iSerialNumber
	0x01,              		// bNumConfigurations

// configuration
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(0x20),  		// wTotalLength
	0x01,  				// bNumInterfaces
	0x01,  				// bConfigurationValue
	0x00,  				// iConfiguration
	0x80,  				// bmAttributes
	0x32,  				// bMaxPower

// interface
	0x09,   				
	DESC_INTERFACE, 
	0x00,  		 		// bInterfaceNumber
	0x00,   			// bAlternateSetting
	0x00,   			// bNumEndPoints
	0xFE,   			// bInterfaceClass
	0x01,   			// bInterfaceSubClass
	0x02,   			// bInterfaceProtocol
	0x00,   			// iInterface

// DFU Functional Descriptor
	0x09,
	DESC_DFU_FUNCTIONAL,
	DFU::WILL_DETACH | DFU::MANIFESTATION_TOLERANT | DFU::CAN_UPLOAD | DFU::CAN_DNLOAD,		// bmAttributes 
	LE_WORD(DFU::detach_timeout_ms),// wDetachTimeOut 
	LE_WORD(DFU::transfer_size),	// wTransferSize 
	LE_WORD(0x0101),		// bcdDFUVersion

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

static Flash flash;
static DFU dfu(flash);
static uint8_t dfu_buffer[DFU::transfer_size];

BOOL dfu_request_handler(TSetupPacket *pSetup, int *piLen, u8 **ppbData) {
    return dfu.request_handler(pSetup, reinterpret_cast<uint32_t*>(piLen), ppbData) ? TRUE : FALSE;
}

int bootloader_usb_init()
{
	// initialise stack
	USBInit();
	
	// register device descriptors
	USBRegisterDescriptors(dfu_descriptors);

	// override standard request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, dfu_request_handler, dfu_buffer);

	// enable USB interrupts
	//ISER0 |= ISER0_ISE_USB;
	
	// connect to bus
	USBHwConnect(TRUE);

	return 0;
}

void bootloader_usb_close() {
    USBHwConnect(FALSE);
}

static void run_bootloader()
{
	bootloader_usb_init();

	while( dfu.in_dfu_mode() ) {
		USBHwISR();
	}
    
    bootloader_usb_close();
}

static void run_application() {

// disable interrupts, or perhaps do it on entry to the bootloader, since we're not using interrupts?
    
    typedef void (*ApplicationEntry)();
    ApplicationEntry application_entry = reinterpret_cast<ApplicationEntry>(0x4000);
    application_entry();
}

int main(void)
{
    ubertooth_init();
    
	ENTRY_PIN |= ENTRY_SET;
	
	/*if (ENTRY_PIN & ENTRY_SET)
		;
	else*/
		run_bootloader();

    run_application();
    
    return 0;
}

