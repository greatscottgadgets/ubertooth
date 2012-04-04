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

#include <config.h>
#include <string>
#include <errno.h>
#include <time.h>

#include <pthread.h>

#include <sstream>
#include <iomanip>

#include <util.h>
#include <messagebus.h>
#include <packet.h>
#include <packetchain.h>
#include <packetsource.h>
#include <packetsourcetracker.h>
#include <timetracker.h>
#include <configfile.h>
#include <plugintracker.h>
#include <globalregistry.h>
#include <netracker.h>
#include <alertracker.h>
#include <dumpfile_pcap.h>
#include <version.h>

#include "packetsource_ubertooth.h"
#include "packet_btbb.h"
#include "phy_btbb.h"

GlobalRegistry *globalreg = NULL;

int pack_comp_btbb;

int ubertooth_unregister(GlobalRegistry *in_globalreg) {
	return 0;
}

int ubertooth_register(GlobalRegistry *in_globalreg) {
	globalreg = in_globalreg;

	globalreg->sourcetracker->AddChannelList("ubertooth:"
			"0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,"
			"20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,"
			"40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,"
			"60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78");

	if (globalreg->sourcetracker->RegisterPacketSource(new PacketSource_Ubertooth(globalreg)) < 0 || globalreg->fatal_condition)
		return -1;

	if (globalreg->kismet_instance == KISMET_INSTANCE_SERVER) {

		pack_comp_btbb =
			globalreg->packetchain->RegisterPacketComponent("BTBBFRAME");

		// dumpfile that inherits from the global one
		Dumpfile_Pcap *btbbdump;
		btbbdump = 
			new Dumpfile_Pcap(globalreg, "pcapbtbb", KDLT_BTBB,
							  globalreg->pcapdump, NULL, NULL);
		btbbdump->SetVolatile(1);

		// Phyneutral btbb phy
		if (globalreg->devicetracker->RegisterPhyHandler(new Btbb_Phy(globalreg)) < 0) {
			_MSG("Failed to load BTBB PHY handler", MSGFLAG_ERROR);
			return -1;
		}
	}

	return 1;
}

extern "C" {
	int kis_plugin_info(plugin_usrdata *data) {
		data->pl_name = "UBERTOOTH";
		data->pl_version = string(VERSION_MAJOR) + "-" + string(VERSION_MINOR) + "-" +
			string(VERSION_TINY);
		data->pl_description = "Ubertooth plugin for Bluetooth baseband protocol";
		data->pl_unloadable = 0; // We can't be unloaded because we defined a source
		data->plugin_register = ubertooth_register;
		data->plugin_unregister = ubertooth_unregister;

		return 1;
	}

	void kis_revision_info(plugin_revision *prev) {
		if (prev->version_api_revision >= 1) {
			prev->version_api_revision = 1;
			prev->major = string(VERSION_MAJOR);
			prev->minor = string(VERSION_MINOR);
			prev->tiny = string(VERSION_TINY);
		}
	}
}
