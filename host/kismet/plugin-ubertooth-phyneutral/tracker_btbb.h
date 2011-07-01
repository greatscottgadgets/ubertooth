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

#ifndef __TRACKER_BTBB_H__
#define __TRACKER_BTBB_H__

#include "config.h"

#include "globalregistry.h"
#include "packet_btbb.h"

class btbb_network {
public:
	btbb_network() {
		first_time = 0;
		last_time = 0;
		num_packets = 0;
		dirty = 0;
	}

	uint32_t lap;

	mac_addr bd_addr;

	int num_packets;

	time_t first_time, last_time;

	kis_gps_data gpsdata;

	int dirty;
};

class Tracker_BTBB {
public:
	Tracker_BTBB() { fprintf(stderr, "FATAL OOPS: tracker_btbb()\n"); exit(1); }
	Tracker_BTBB(GlobalRegistry *in_globalreg);

	int chain_handler(kis_packet *in_pack);

	void BlitDevices(int in_fd);

protected:
	GlobalRegistry *globalreg;

	map<uint32_t, btbb_network *> first_nets;
	
	map<uint32_t, btbb_network *> tracked_nets;

	int BTBBDEV_ref;
	int timer_ref;
};

#endif
