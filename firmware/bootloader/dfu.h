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

#ifndef _DFU_H_
#define _DFU_H_

/* Status */
#define STATUS_OK				0x00
#define STATUS_ERRTARGET		0x01
#define STATUS_ERRFILE			0x02
#define STATUS_ERRWRITE			0x03
#define STATUS_ERRERASE			0x04
#define STATUS_ERRCHECK_ERASED	0x05
#define STATUS_ERRPROG			0x06
#define STATUS_ERRVERIFY		0x07
#define STATUS_ERRADDRESS		0x08
#define STATUS_ERRNOTDONE		0x09
#define STATUS_ERRFIRMWARE		0x0A
#define STATUS_ERRVENDOR		0x0B
#define STATUS_ERRUSBR			0x0C
#define STATUS_ERRPOR			0x0D
#define STATUS_ERRUNKNOWN		0x0E
#define STATUS_ERRSTALLEDPKT	0x0F

/* State */
#define STATE_APPIDLE					0x00
#define STATE_APPDETACH					0x01
#define STATE_DFUIDLE					0x02
#define STATE_DFUDNLOAD_SYNC			0x03
#define STATE_DFUDNBUSY					0x04
#define STATE_DFUDNLOAD_IDLE			0x05
#define STATE_DFUMANIFEST_SYNC			0x06
#define STATE_DFUMANIFEST				0x07
#define STATE_DFUMANIFEST_WAIT_RESET	0x08
#define STATE_DFUUPLOAD_IDLE			0x09
#define STATE_DFUERROR					0x0A

/* bmAttributes */
#define WILL_DETACH            (0x1 << 3)
#define MANIFESTATION_TOLERANT (0x1 << 2)
#define CAN_UPLOAD             (0x1 << 1)
#define CAN_DNLOAD             (0x1 << 0)

/* bmAttributes */
#define DETACH     0
#define DNLOAD     1
#define UPLOAD     2
#define GETSTATUS  3
#define CLRSTATUS  4
#define GETSTATE   5
#define ABORT      6

#endif /* _DFU_H_ */
