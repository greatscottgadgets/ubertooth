/* -*- c++ -*- */
/*
 * Copyright 2010, 2011 Michael Ossmann
 * Copyright 2009, 2010 Mike Kershaw
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

#ifndef __PACKET_BTBB_H__
#define __PACKET_BTBB_H__

#include "config.h"

#include <packetchain.h>
#include <packetsource.h>

// we are temporarily encapsulating in ethernet frames
#define KDLT_BTBB 1

int kis_btbb_dissector(CHAINCALL_PARMS);

enum btbb_type {
	btbb_type_null = 0x0,
	btbb_type_poll = 0x1,
	btbb_type_fhs = 0x2,
	btbb_type_dm1 = 0x3,
	btbb_type_dh1 = 0x4,
	btbb_type_hv1 = 0x5,
	btbb_type_hv2 = 0x6,
	btbb_type_hv3 = 0x7,
	btbb_type_dv = 0x8,
	btbb_type_aux1 = 0x9,
	btbb_type_dm3 = 0xa,
	btbb_type_dh3 = 0xb,
	btbb_type_ev4 = 0xc,
	btbb_type_ev5 = 0xd,
	btbb_type_dm5 = 0xe,
	btbb_type_dh5 = 0xf,
	btbb_type_id,
	btbb_type_max
};
extern const char *btbb_type_str[];

class btbb_packinfo : public packet_component {
public:
	btbb_packinfo() {
		self_destruct = 1;

		lap = 0;
		uap = 0;
		nap = 0;
		have_uap = false;
		have_nap = false;
		type = btbb_type_id;

		channel = 0;
	};

	uint32_t lap;
	uint8_t uap;
	uint16_t nap;

	bool have_uap;
	bool have_nap;

	btbb_type type;

	int channel;
};

#endif
