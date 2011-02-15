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


/**
	Definitions of structures of standard USB packets
*/

#ifndef _USBSTRUCT_H_
#define _USBSTRUCT_H_


#include "type.h"


/** setup packet definitions */
typedef struct {
	U8	bmRequestType;			/**< characteristics of the specific request */
	U8	bRequest;				/**< specific request */
	U16	wValue;					/**< request specific parameter */
	U16	wIndex;					/**< request specific parameter */
	U16	wLength;				/**< length of data transfered in data phase */
} TSetupPacket;


#define REQTYPE_GET_DIR(x)		(((x)>>7)&0x01)
#define REQTYPE_GET_TYPE(x)		(((x)>>5)&0x03)
#define REQTYPE_GET_RECIP(x)	((x)&0x1F)

#define REQTYPE_DIR_TO_DEVICE	0
#define REQTYPE_DIR_TO_HOST		1

#define REQTYPE_TYPE_STANDARD	0
#define REQTYPE_TYPE_CLASS		1
#define REQTYPE_TYPE_VENDOR		2
#define REQTYPE_TYPE_RESERVED	3

#define REQTYPE_RECIP_DEVICE	0
#define REQTYPE_RECIP_INTERFACE	1
#define REQTYPE_RECIP_ENDPOINT	2
#define REQTYPE_RECIP_OTHER		3

/* standard requests */
#define	REQ_GET_STATUS			0x00
#define REQ_CLEAR_FEATURE		0x01
#define REQ_SET_FEATURE			0x03
#define REQ_SET_ADDRESS			0x05
#define REQ_GET_DESCRIPTOR		0x06
#define REQ_SET_DESCRIPTOR		0x07
#define REQ_GET_CONFIGURATION	0x08
#define REQ_SET_CONFIGURATION	0x09
#define REQ_GET_INTERFACE		0x0A
#define REQ_SET_INTERFACE		0x0B
#define REQ_SYNCH_FRAME			0x0C

/* class requests HID */
#define HID_GET_REPORT			0x01
#define HID_GET_IDLE			0x02
#define HID_GET_PROTOCOL	 	0x03
#define HID_SET_REPORT			0x09
#define HID_SET_IDLE			0x0A
#define HID_SET_PROTOCOL		0x0B

/* feature selectors */
#define FEA_ENDPOINT_HALT		0x00
#define FEA_REMOTE_WAKEUP		0x01
#define FEA_TEST_MODE			0x02

/* DFU Attributes */
#define DFU_WILL_DETACH            (0x1 << 3)
#define DFU_MANIFESTATION_TOLERANT (0x1 << 2)
#define DFU_CAN_UPLOAD             (0x1 << 1)
#define DFU_CAN_DNLOAD             (0x1 << 0)

/*
	USB descriptors
*/

/** USB descriptor header */
typedef struct {
	U8	bLength;			/**< descriptor length */
	U8	bDescriptorType;	/**< descriptor type */
} TUSBDescHeader;

#define DESC_DEVICE				1
#define DESC_CONFIGURATION		2
#define DESC_STRING				3
#define DESC_INTERFACE			4
#define DESC_ENDPOINT			5
#define DESC_DEVICE_QUALIFIER	6
#define DESC_OTHER_SPEED		7
#define DESC_INTERFACE_POWER	8

#define DESC_DFU_FUNCTIONAL		0x21

#define DESC_HID_HID			0x21
#define DESC_HID_REPORT			0x22
#define DESC_HID_PHYSICAL		0x23

#define GET_DESC_TYPE(x)		(((x)>>8)&0xFF)
#define GET_DESC_INDEX(x)		((x)&0xFF)

#define DFU_STATUS_OK				0x00
#define DFU_STATUS_ERRTARGET		0x01
#define DFU_STATUS_ERRFILE			0x02
#define DFU_STATUS_ERRWRITE			0x03
#define DFU_STATUS_ERRERASE			0x04
#define DFU_STATUS_ERRCHECK_ERASED	0x05
#define DFU_STATUS_ERRPROG			0x06
#define DFU_STATUS_ERRVERIFY		0x07
#define DFU_STATUS_ERRADDRESS		0x08
#define DFU_STATUS_ERRNOTDONE		0x09
#define DFU_STATUS_ERRFIRMWARE		0x0A
#define DFU_STATUS_ERRVENDOR		0x0B
#define DFU_STATUS_ERRUSBR			0x0C
#define DFU_STATUS_ERRPOR			0x0D
#define DFU_STATUS_ERRUNKNOWN		0x0E
#define DFU_STATUS_ERRSTALLEDPKT	0x0F


#define DFU_STATE_appIDLE					0x00
#define DFU_STATE_appDETACH					0x01
#define DFU_STATE_dfuIDLE					0x02
#define DFU_STATE_dfuDNLOAD-SYNC			0x03
#define DFU_STATE_dfuDNBUSY					0x04
#define DFU_STATE_dfuDNLOAD-IDLE			0x05
#define DFU_STATE_dfuMANIFEST-SYNC			0x06
#define DFU_STATE_dfuMANIFEST				0x07
#define DFU_STATE_dfuMANIFEST-WAIT-RESET	0x08
#define DFU_STATE_dfuUPLOAD-IDLE			0x09
#define DFU_STATE_dfuERROR					0x0A

#endif /* _USBSTRUCT_H_ */

