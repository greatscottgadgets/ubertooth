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

#define DFU_TIMEOUT 1572864

#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

/*
 * Ubertooth Zero:
 *
 * pin expansion connector (J1) pin 13 which is GPIO P2.7, configure
 * pull-up and check if it has been jumpered to ground (pin 1).
 *
 * On Ubertooth One and ToorCon 13 Badge:
 *
 * expansion connector (P4) pin 3 which is GPIO P0.22, same
 * configuration, ground is at pin 1.
 */

#ifdef UBERTOOTH_ZERO
#define ENTRY_PIN (!(FIO2PIN & (1 << 7)))
#endif
#if defined UBERTOOTH_ONE || defined TC13BADGE
#define ENTRY_PIN (!(FIO0PIN & (1 << 22)))
#endif

#ifdef UBERTOOTH_ZERO
#define ID_VENDOR 0x1D50
#define ID_PRODUCT 0x6001
#elif defined UBERTOOTH_ONE
#define ID_VENDOR 0x1D50
#define ID_PRODUCT 0x6003
#elif defined TC13BADGE
#define ID_VENDOR 0xFFFF
#define ID_PRODUCT 0x0004
#else
#define ID_VENDOR 0xFFFF
#define ID_PRODUCT 0x0004
#endif

static uint8_t dfu_descriptors[] = {

/* Device descriptor */
	0x12,
	DESC_DEVICE,
	LE_WORD(0x0200),		// bcdUSB
	0x00,              		// bDeviceClass
	0x00,              		// bDeviceSubClass
	0x00,              		// bDeviceProtocol
	MAX_PACKET_SIZE0,  		// bMaxPacketSize
	LE_WORD(ID_VENDOR),		// idVendor
	LE_WORD(ID_PRODUCT),	// idProduct
	LE_WORD(0x0100),		// bcdDevice
	0x01,              		// iManufacturer
	0x02,              		// iProduct
	0x00,              		// iSerialNumber
	0x01,              		// bNumConfigurations

// configuration
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(0x1B),  		// wTotalLength
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
	0x5C,
	DESC_STRING,
	'h', 0, 't', 0, 't', 0, 'p', 0, ':', 0, '/', 0, '/', 0, 'g', 0,
	'i', 0, 't', 0, 'h', 0, 'u', 0, 'b', 0, '.', 0, 'c', 0, 'o', 0,
	'm', 0, '/', 0, 'g', 0, 'r', 0, 'e', 0, 'a', 0, 't', 0, 's', 0,
	'c', 0, 'o', 0, 't', 0, 't', 0, 'g', 0, 'a', 0, 'd', 0, 'g', 0,
	'e', 0, 't', 0, 's', 0, '/', 0, 'u', 0, 'b', 0, 'e', 0, 'r', 0,
	't', 0, 'o', 0, 'o', 0, 't', 0, 'h', 0,

	// product string
	0x1E,
	DESC_STRING,
	'u', 0, 's', 0, 'b', 0, '_', 0, 'b', 0, 'o', 0, 'o', 0, 't', 0, 'l', 0, 'o', 0,
	'a', 0, 'd', 0, 'e', 0, 'r', 0,

	// terminator
	0
};

static Flash flash;
static DFU dfu(flash);
static uint8_t dfu_buffer[DFU::transfer_size];
static uint32_t count = 0;
static bool use_timeout = false;

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

void leds_on() {
		TXLED_SET;
		RXLED_SET;
		USRLED_SET;
}

void leds_off() {
		TXLED_CLR;
		RXLED_CLR;
		USRLED_CLR;
}

/* chasing pattern indicates bootloader activity */
void update_leds() {
	count += 1;
	const uint32_t led_state = (count >> 16) % 6;
	switch (led_state) {
	case 0:
		TXLED_SET;
		RXLED_CLR;
		USRLED_CLR;
		break;
	case 1:
		TXLED_SET;
		RXLED_SET;
		USRLED_CLR;
		break;
	case 2:
		TXLED_SET;
		RXLED_SET;
		USRLED_SET;
		break;
	case 3:
		TXLED_SET;
		RXLED_SET;
		USRLED_CLR;
		break;
	case 4:
		TXLED_SET;
		RXLED_CLR;
		USRLED_CLR;
		break;
	case 5:
		TXLED_CLR;
		RXLED_CLR;
		USRLED_CLR;
		break;
	}
}

static void run_bootloader()
{
	leds_on();
	bootloader_usb_init();

	while( dfu.in_dfu_mode() ) {
		USBHwISR();
		update_leds();
		if (use_timeout && dfu.dfu_virgin() && (count > DFU_TIMEOUT))
			break;
	}

	bootloader_usb_close();
	leds_off();
}

static void run_application() {

// disable interrupts, or perhaps do it on entry to the bootloader, since we're not using interrupts?
    asm(
        "movw    r3, #16384\n\t"
        "movt    r3, #0\n\t"
        "ldr     sp, [r3, #0]\n\t"  // initial stack pointer
        "ldr     pc, [r3, #4]\n\t"  // initial program counter
    );
}

int main(void)
{
	gpio_init();

	if (VBUS) {
		if (bootloader_ctrl == DFU_MODE) {
			ubertooth_init();
			run_bootloader();
			bootloader_ctrl = 0;
			wait(1);
			reset();
		} else if (ENTRY_PIN) {
			use_timeout = true;
			ubertooth_init();
			run_bootloader();
			wait(1);
			reset();
		}
	}

	run_application();

	return 0;
}

