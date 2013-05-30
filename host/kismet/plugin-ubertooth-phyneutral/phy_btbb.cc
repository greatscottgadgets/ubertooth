/*
    Copyright 2012, 2013 Dominic Spill
    Copyright 2010, 2011 Michael Ossmann
    Copyright 2009 - 2011 Mike Kershaw
 
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
*/

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
#include <alertracker.h>
#include <configfile.h>
#include <devicetracker.h>

#include "phy_btbb.h"
#include "packet_btbb.h"

enum BTBB_fields {
	BTBB_bdaddr, BTBB_lap, 
	BTBB_maxfield
};

const char *BTBB_fields_text[] = {
	"bdaddr", "lap",
	NULL
};

int Protocol_BTBB(PROTO_PARMS) {
	btbb_device_component *btbb = (btbb_device_component *) data;
	string scratch;

	cache->Filled(field_vec->size());

	for (unsigned int x = 0; x < field_vec->size(); x++) {
		unsigned int fnum = (*field_vec)[x];

		if (fnum > BTBB_maxfield) {
			out_string = "\001Unknown field\001";
			return -1;
		}

		if (cache->Filled(fnum)) {
			out_string += cache->GetCache(fnum) + " ";
			continue;
		}

		scratch = "";

		switch (fnum) {
			case BTBB_bdaddr:
				scratch = btbb->bdaddr.Mac2String();
				break;
			case BTBB_lap:
				scratch = UIntToString(btbb->lap);
				break;
		}

		cache->Cache(fnum, scratch);
		out_string += scratch + " ";
	}

	return 1;
}

void Protocol_BTBB_enable(PROTO_ENABLE_PARMS) {
	((Btbb_Phy *) data)->BlitDevices(in_fd, NULL);
}

int phybtbb_packethook_btbbdissect(CHAINCALL_PARMS) {
	return ((Btbb_Phy *) auxdata)->DissectorBtbb(in_pack);
}

int phybtbb_packethook_btbbclassify(CHAINCALL_PARMS) {
	return ((Btbb_Phy *) auxdata)->ClassifierBtbb(in_pack);
}

int phybtbb_packethook_btbbtracker(CHAINCALL_PARMS) {
	return ((Btbb_Phy *) auxdata)->TrackerBtbb(in_pack);
}

Btbb_Phy::Btbb_Phy(GlobalRegistry *in_globalreg, Devicetracker *in_tracker,
				   int in_phyid) : Kis_Phy_Handler(in_globalreg, in_tracker,
												   in_phyid) {
	globalreg->InsertGlobal("PHY_BTBB_TRACKER", this);
	phyname = "BTBB";

	globalreg->packetchain->RegisterHandler(&phybtbb_packethook_btbbdissect,
											this, CHAINPOS_LLCDISSECT, 0);
	globalreg->packetchain->RegisterHandler(&phybtbb_packethook_btbbclassify,
											this, CHAINPOS_CLASSIFIER, 0);
	globalreg->packetchain->RegisterHandler(&phybtbb_packethook_btbbtracker,
											this, CHAINPOS_TRACKER, 100);

	dev_comp_btbbdev = devicetracker->RegisterDeviceComponent("BTBB_DEV");
	dev_comp_common = devicetracker->RegisterDeviceComponent("COMMON");

	pack_comp_btbb = globalreg->packetchain->RegisterPacketComponent("UBERTOOTH");
	pack_comp_common = globalreg->packetchain->RegisterPacketComponent("COMMON");
	pack_comp_device = globalreg->packetchain->RegisterPacketComponent("DEVICE");

	proto_ref_btbbdev =
		globalreg->kisnetserver->RegisterProtocol("BTBB", 0, 1,
												  BTBB_fields_text,
												  &Protocol_BTBB,
												  &Protocol_BTBB_enable,
												  this);
}

Btbb_Phy::~Btbb_Phy() {
	globalreg->packetchain->RemoveHandler(&phybtbb_packethook_btbbdissect,
										  CHAINPOS_LLCDISSECT);
	globalreg->packetchain->RemoveHandler(&phybtbb_packethook_btbbclassify,
										  CHAINPOS_CLASSIFIER);
	globalreg->packetchain->RemoveHandler(&phybtbb_packethook_btbbtracker,
										  CHAINPOS_TRACKER);

	globalreg->kisnetserver->RemoveProtocol(proto_ref_btbbdev);
}

int Btbb_Phy::DissectorBtbb(kis_packet *in_pack) {
	// printf("debug - btbb dissector\n");

	btbb_packinfo *pi = NULL;

	if (in_pack->error)
		return 0;

	kis_datachunk *chunk =
		(kis_datachunk *) in_pack->fetch(_PCM(PACK_COMP_LINKFRAME));

	if (chunk == NULL)
		return 0;

	if (chunk->dlt != KDLT_BTBB) {
		return 0;
	}

	if (chunk->length < 14) {
		in_pack->error = 1;
		return 0;
	}

	pi = new btbb_packinfo();

	pi->type = btbb_type_id;
	pi->nap = (chunk->data[6] << 8)
			| chunk->data[7];
	pi->uap = chunk->data[8];
	pi->lap = (chunk->data[9] << 16)
			| (chunk->data[10] << 8)
			| chunk->data[11];

	//printf("Bluetooth Baseband Packet %d\n", debugno);

	// printf("debug - btbb - adding pi\n");
	in_pack->insert(pack_comp_btbb, pi);

	return 1;
}

int Btbb_Phy::ClassifierBtbb(kis_packet *in_pack) {
	btbb_packinfo *pi = (btbb_packinfo *) in_pack->fetch(pack_comp_btbb);

	if (pi == NULL) {
		// printf("debug -btbb no pi\n");
		return 0;
	}

	uint32_t lap = pi->lap;

	kis_common_info *common = new kis_common_info;

	common->phyid = phyid;
	common->type = packet_basic_phy;

	uint8_t bdaddr[6];
	bdaddr[0] = (pi->nap >> 8) & 0xFF;
	bdaddr[1] = pi->nap & 0xFF;
	bdaddr[2] = pi->uap & 0xFF;
	bdaddr[3] = (pi->lap >> 16) & 0xFF;
	bdaddr[4] = (pi->lap >> 8) & 0xFF;
	bdaddr[5] = pi->lap & 0xFF;

	common->device = mac_addr(bdaddr, 6);
	common->device.SetPhy(phyid);

	common->source = common->device;

	in_pack->insert(pack_comp_common, common);

	/*
	 * Due to poor error correction, there is a high likelihood that LAPs seen
	 * only once don't really exist.
	 */
	if (first_nets.find(lap) == first_nets.end()) {
		first_nets[lap] = 1;

		// Flag the packet as filtered
		in_pack->filtered = 1;
	}  


	return 1;
}

int Btbb_Phy::TrackerBtbb(kis_packet *in_pack) {
	btbb_packinfo *pi = (btbb_packinfo *) in_pack->fetch(pack_comp_btbb);
	kis_tracked_device_info *devinfo = 
		(kis_tracked_device_info *) in_pack->fetch(pack_comp_device);
	btbb_device_component *btbb = NULL;
	kis_device_common *commondev = NULL;

	// If we dont' have btbb, or we're filtered (ie first time seen),
	// ignore it
	if (pi == NULL || in_pack->filtered || devinfo == NULL)
		return 0;

	if (devinfo->devref == NULL)
		return 0;

	// Get the common
	commondev = (kis_device_common *) devinfo->devref->fetch(dev_comp_common);

	if (commondev == NULL)
		return 0;

	/* track this LAP now that we've seen it twice */
	btbb = (btbb_device_component *) devinfo->devref->fetch(dev_comp_btbbdev);

	// we only have to track stuff on new BTBB records, the rest is common
	if (btbb == NULL) {
		btbb = new btbb_device_component;
		devinfo->devref->insert(dev_comp_btbbdev, btbb);

		uint8_t bdaddr[6];
		bdaddr[0] = (pi->nap >> 8) & 0xFF;
		bdaddr[1] = pi->nap & 0xFF;
		bdaddr[2] = pi->uap & 0xFF;
		bdaddr[3] = (pi->lap >> 16) & 0xFF;
		bdaddr[4] = (pi->lap >> 8) & 0xFF;
		bdaddr[5] = pi->lap & 0xFF;

		btbb->lap = pi->lap;
		btbb->bdaddr = mac_addr(bdaddr, 6);

		// Bluetooth devices are all networks, so they're all APs.  I guess.
		commondev->basic_type_set |= 
			(KIS_DEVICE_BASICTYPE_AP | KIS_DEVICE_BASICTYPE_CLIENT |
			 KIS_DEVICE_BASICTYPE_PEER);
		commondev->type_string = "Bluetooth";
		commondev->name = btbb->bdaddr.Mac2String();

		_MSG("Detected new Bluetooth baseband device " + 
			 btbb->bdaddr.Mac2String(), MSGFLAG_INFO);
	}

	return 1;
}

int Btbb_Phy::TimerKick() {
	// We dont' have to kick dirty devices b/c the devicetracker handles all that 
	// for us
	return 1;
}

void Btbb_Phy::BlitDevices(int in_fd, vector<kis_tracked_device *> *devlist) {
	if (devlist == NULL)
		devlist = devicetracker->FetchDevices(phyid);

	for (unsigned int x = 0; x < devlist->size(); x++) {
		kis_protocol_cache cache;

		btbb_device_component *btbb;

		if ((btbb = (btbb_device_component *) (*devlist)[x]->fetch(dev_comp_btbbdev)) != NULL) {
			if (in_fd == -1)
				globalreg->kisnetserver->SendToAll(proto_ref_btbbdev,
												   (void *) btbb);
			else
				globalreg->kisnetserver->SendToClient(in_fd, proto_ref_btbbdev,
													  (void *) btbb, &cache);
		}
	}

}

void Btbb_Phy::ExportLogRecord(kis_tracked_device *in_device, string in_logtype, 
							   FILE *in_logfile, int in_lineindent) {
	// Nothing special to log yet
	return;
}


