/*
    Copyright 2010, 2011 Michael Ossmann
    Copyright 2009, 2010, 2011 Mike Kershaw
 
	This file is part of Project Ubertooth.

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	CODE IN BOTH phy_80211.cc AND phy_80211_dissectors.cc
*/

#ifndef __PHY_BTBB_H__
#define __PHY_BTBB_H__

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <globalregistry.h>
#include <packetchain.h>
#include <kis_netframe.h>
#include <timetracker.h>
#include <filtercore.h>
#include <gpscore.h>
#include <packet.h>
#include <uuid.h>
#include <configfile.h>

#include <devicetracker.h>

class btbb_device_component : public tracker_component {
public:
	btbb_device_component() {
		lap = 0;
	}

	uint32_t lap;
	mac_addr bdaddr;
};

class Btbb_Phy : public Kis_Phy_Handler {
public:
	Btbb_Phy() { }
	~Btbb_Phy();

	// Weak constructor
	Btbb_Phy(GlobalRegistry *in_globalreg) :
		Kis_Phy_Handler(in_globalreg) { };

	// Builder
	virtual Kis_Phy_Handler *CreatePhyHandler(GlobalRegistry *in_globalreg,
											  Devicetracker *in_tracker,
											  int in_phyid) {
		return new Btbb_Phy(in_globalreg, in_tracker, in_phyid);
	}

	// Strong constructor
	Btbb_Phy(GlobalRegistry *in_globalreg, Devicetracker *in_tracker,
			 int in_phyid);

	// BTBB dissector
	int DissectorBtbb(kis_packet *in_pack);

	// BTBB classifier
	int ClassifierBtbb(kis_packet *in_pack);

	// BTBB tracker
	int TrackerBtbb(kis_packet *in_pack);

	// Timer called from devicetracker
	virtual int TimerKick();

	virtual void BlitDevices(int in_fd, vector<kis_tracked_device *> *devlist);

	virtual void ExportLogRecord(kis_tracked_device *in_device, string in_logtype, 
								 FILE *in_logfile, int in_lineindent);

protected:
	// Protocol references
	int proto_ref_btbbdev;

	// Device components
	int dev_comp_btbbdev, dev_comp_common;

	// Packet components
	int pack_comp_btbb, pack_comp_common, pack_comp_device;

	// We have to keep a local copy of tracked networks because we need
	// to filter single-appearance networks
	map<uint32_t, int> first_nets;
};

#endif

