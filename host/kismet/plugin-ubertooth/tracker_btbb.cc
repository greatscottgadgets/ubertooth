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

#include "config.h"

#include <globalregistry.h>
#include <packetchain.h>

#include "packet_btbb.h"
#include "tracker_btbb.h"

extern int pack_comp_btbb;

enum BTBBDEV_fields {
	BTBBDEV_lap,
	BTBBDEV_bdaddr, BTBBDEV_firsttime,
	BTBBDEV_lasttime, BTBBDEV_packets,
	GPS_COMMON_FIELDS(BTBBDEV),
	BTBBDEV_maxfield
};

const char *BTBBDEV_fields_text[] = {
	"lap", "bdaddr", "firsttime",
	"lasttime", "packets",
	GPS_COMMON_FIELDS_TEXT,
	NULL
};

int Protocol_BTBBDEV(PROTO_PARMS) {
	btbb_network *net = (btbb_network *) data;
	ostringstream osstr;

	cache->Filled(field_vec->size());

	for (unsigned int x = 0; x < field_vec->size(); x++) {
		unsigned int fnum = (*field_vec)[x];

		if (fnum >= BTBBDEV_maxfield) {
			out_string = "Unknown field requested.";
			return -1;
		}

		osstr.str("");

		if (cache->Filled(fnum)) {
			out_string += cache->GetCache(fnum) + " ";
			continue;
		}

		switch (fnum) {
			case BTBBDEV_lap:
				osstr << net->lap;
				break;
			case BTBBDEV_bdaddr:
				// TODO - fix these for endian swaps, output as bytes in a fixed order ?
				osstr << net->bd_addr.Mac2String();
				break;
			case BTBBDEV_firsttime:
				osstr << net->first_time;
				break;
			case BTBBDEV_lasttime:
				osstr << net->last_time;
				break;
			case BTBBDEV_packets:
				osstr << net->num_packets;
				break;
			case BTBBDEV_gpsfixed:
				osstr << net->gpsdata.gps_valid;
				break;
			case BTBBDEV_minlat:
				osstr << net->gpsdata.min_lat;
				break;
			case BTBBDEV_maxlat:
				osstr << net->gpsdata.max_lat;
				break;
			case BTBBDEV_minlon:
				osstr << net->gpsdata.min_lon;
				break;
			case BTBBDEV_maxlon:
				osstr << net->gpsdata.max_lon;
				break;
			case BTBBDEV_minalt:
				osstr << net->gpsdata.min_alt;
				break;
			case BTBBDEV_maxalt:
				osstr << net->gpsdata.max_alt;
				break;
			case BTBBDEV_minspd:
				osstr << net->gpsdata.min_spd;
				break;
			case BTBBDEV_maxspd:
				osstr << net->gpsdata.max_spd;
				break;
			case BTBBDEV_agglat:
				osstr << net->gpsdata.aggregate_lat;
				break;
			case BTBBDEV_agglon:
				osstr << net->gpsdata.aggregate_lon;
				break;
			case BTBBDEV_aggalt:
				osstr << net->gpsdata.aggregate_alt;
				break;
			case BTBBDEV_aggpoints:
				osstr <<net->gpsdata.aggregate_points;
				break;
		}

		out_string += osstr.str() + " ";
		cache->Cache(fnum, osstr.str());
	}

	return 1;
}

void Protocol_BTBBDEV_enable(PROTO_ENABLE_PARMS) {
	((Tracker_BTBB *) data)->BlitDevices(in_fd);
}

int bttracktimer(TIMEEVENT_PARMS) {
#ifndef KIS_NEW_TIMER_PARM
	((Tracker_BTBB *) parm)->BlitDevices(-1);
#else
	((Tracker_BTBB *) auxptr)->BlitDevices(-1);
#endif
	return 1;
}

int btbb_chain_hook(CHAINCALL_PARMS) {
	return ((Tracker_BTBB *) auxdata)->chain_handler(in_pack);
}

Tracker_BTBB::Tracker_BTBB(GlobalRegistry *in_globalreg) {
	globalreg = in_globalreg;

	globalreg->packetchain->RegisterHandler(&btbb_chain_hook, this,
											CHAINPOS_CLASSIFIER, 0);

	BTBBDEV_ref = 
		globalreg->kisnetserver->RegisterProtocol("BTBBDEV", 0, 1,
												  BTBBDEV_fields_text,
												  &Protocol_BTBBDEV,
												  &Protocol_BTBBDEV_enable,
												  this);

	timer_ref =
		globalreg->timetracker->RegisterTimer(SERVER_TIMESLICES_SEC, NULL, 1,
											  &bttracktimer, this);
}

int Tracker_BTBB::chain_handler(kis_packet *in_pack) {
	btbb_packinfo *pi = (btbb_packinfo *) in_pack->fetch(pack_comp_btbb);

	if (pi == NULL)
		return 0;

	uint32_t lap = pi->lap;
	btbb_network *net = NULL;

	/*
	 * Due to poor error correction, there is a high likelihood that LAPs seen
	 * only once don't really exist.
	 */
	if (!first_nets[lap]) {
		printf("first sighting %06x\n", lap);
		net = new btbb_network();
		net->first_time = globalreg->timestamp.tv_sec;
		net->lap = lap;
		first_nets[lap] = net;

	} else if (!tracked_nets[lap]) {
		/* track this LAP now that we've seen it twice */
		printf("new network %06x\n", lap);
		tracked_nets[lap] = first_nets[lap];
		net = tracked_nets[lap];
	} else {
		net = tracked_nets[lap];
	}
	net->bd_addr.longmac =
			((uint64_t)pi->nap << 32) |
			((uint32_t)pi->uap << 24) |
			lap;

	kis_gps_packinfo *gpsinfo = (kis_gps_packinfo *)
			in_pack->fetch(_PCM(PACK_COMP_GPS));

	if (gpsinfo != NULL && gpsinfo->gps_fix) {
		net->gpsdata += gpsinfo;
	}

	net->dirty = 1;

	net->last_time = globalreg->timestamp.tv_sec;
	net->num_packets++;

	return 1;
}

void Tracker_BTBB::BlitDevices(int in_fd) {
	map<uint32_t, btbb_network *>::iterator x;

	for (x = tracked_nets.begin(); x != tracked_nets.end(); x++) {
		kis_protocol_cache cache;

		if (in_fd == -1) {
			if (x->second->dirty == 0)
				continue;

			x->second->dirty = 0;

			if (globalreg->kisnetserver->SendToAll(BTBBDEV_ref,
												   (void *) x->second) < 0)
				break;

		} else {
			if (globalreg->kisnetserver->SendToClient(in_fd, BTBBDEV_ref,
													  (void *) x->second,
													  &cache) < 0)
				break;
		}
	}
}
