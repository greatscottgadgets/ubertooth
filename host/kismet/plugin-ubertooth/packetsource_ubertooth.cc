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

#include <vector>

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
#include <dumpfile.h>
#include <pcap.h>

#include "packetsource_ubertooth.h"
#include "packet_btbb.h"

PacketSource_Ubertooth::PacketSource_Ubertooth(GlobalRegistry *in_globalreg, string in_interface,
									   vector<opt_pair> *in_opts) : 
	KisPacketSource(in_globalreg, in_interface, in_opts) {

	thread_active = 0;
	devh = NULL;

	fake_fd[0] = -1;
	fake_fd[1] = -1;

	pending_packet = 0;

	channel = 39;
	bank = 0;
	empty_buf = NULL;
	full_buf = NULL;
	really_full = false;
	rx_xfer = NULL;

	btbb_packet_id = globalreg->packetchain->RegisterPacketComponent("UBERTOOTH");
}

PacketSource_Ubertooth::~PacketSource_Ubertooth() {
	CloseSource();
}
	

int PacketSource_Ubertooth::ParseOptions(vector<opt_pair> *in_opts) {
	//if (FetchOpt("device", in_opts) != "") {
		//usb_dev = FetchOpt("usbdev", in_opts);
		//_MSG("Ubertooth Bluetooth using USB device '" + usb_dev + "'", MSGFLAG_INFO);
	//} else {
		_MSG("Ubertooth using first USB device that looks like a Ubertooth",
			 MSGFLAG_INFO);
	//}

	return 1;
}

int PacketSource_Ubertooth::AutotypeProbe(string in_device) {
	// Shortcut like we do on airport
	if (in_device == "ubertooth") {
		type = "ubertooth";
		return 1;
	}
}

// Capture thread to fake async io
void *ubertooth_cap_thread(void *arg) {
	PacketSource_Ubertooth *ubertooth = (PacketSource_Ubertooth *) arg;

	while (ubertooth->thread_active);
		//FIXME main ubertooth loop
		//enqueue stuff

	ubertooth->thread_active = -1;
	close(ubertooth->fake_fd[1]);
	ubertooth->fake_fd[1] = -1;
	pthread_exit((void *) 0);
}

int PacketSource_Ubertooth::OpenSource() {
	if ((devh = ubertooth_start()) == NULL) {
		_MSG("Ubertooth '" + name + "' failed to open device '" + usb_dev + "': " +
				string(strerror(errno)), MSGFLAG_ERROR);
		return 0;
	}

	//FIXME 
	/* Initialize the pipe, mutex, and reading thread */
	if (pipe(fake_fd) < 0) {
		_MSG("Ubertooth '" + name + "' failed to make a pipe() (this is really "
			 "weird): " + string(strerror(errno)), MSGFLAG_ERROR);
		//usb_close(devh);
		//devh = NULL;
		return 0;
	}

	if (pthread_mutex_init(&packet_lock, NULL) < 0) {
		_MSG("Ubertooth '" + name + "' failed to initialize pthread mutex: " +
			 string(strerror(errno)), MSGFLAG_ERROR);
		//usb_close(devh);
		//devh = NULL;
		return 0;
	}

	/* Launch a capture thread */
	thread_active = 1;
	pthread_create(&cap_thread, NULL, ubertooth_cap_thread, this);

	return 1;
}

int PacketSource_Ubertooth::CloseSource() {
	void *ret;

	if (thread_active > 0) {
		// Tell the thread to die
		thread_active = 0;

		// Grab it back
		pthread_join(cap_thread, &ret);

		// Kill the mutex
		pthread_mutex_destroy(&packet_lock);
	}

	if (devh) {
		//FIXME make sure xfers are not active
		libusb_free_transfer(rx_xfer);
		ubertooth_stop(devh);
		devh = NULL;
	}

	if (fake_fd[0] >= 0) {
		close(fake_fd[0]);
		fake_fd[0] = -1;
	}

	if (fake_fd[1] >= 0) {
		close(fake_fd[1]);
		fake_fd[1] = -1;
	}

	return 1;
}

int PacketSource_Ubertooth::SetChannel(unsigned int in_ch) {
	if (in_ch < 0 || in_ch > 78)
		return -1;

	//if (thread_active <= 0 || devh == NULL)
	if (thread_active)
		return 0;

	//FIXME actually set the channel

	channel = in_ch;

	return 1;
}

int PacketSource_Ubertooth::FetchDescriptor() {
	// This is as good a place as any to catch a failure
	if (thread_active < 0) {
		_MSG("Ubertooth '" + name + "' capture thread failed: " +
			 thread_error, MSGFLAG_INFO);
		CloseSource();
		return -1;
	}

	return fake_fd[0];
}

int PacketSource_Ubertooth::Poll() {
	char rx;

	// Consume the junk byte we used to raise the FD high
	read(fake_fd[0], &rx, 1);

	pthread_mutex_lock(&packet_lock);

	pending_packet = 0;

	for (unsigned int x = 0; x < packet_queue.size(); x++) {
		kis_packet *newpack = globalreg->packetchain->GeneratePacket();

		newpack->ts.tv_sec = globalreg->timestamp.tv_sec;
		newpack->ts.tv_usec = globalreg->timestamp.tv_usec;

		kis_datachunk *rawchunk = new kis_datachunk;

		rawchunk->length = packet_queue[x]->len;
		rawchunk->data = new uint8_t[rawchunk->length];
		memcpy(rawchunk->data, packet_queue[x]->data, rawchunk->length);
		rawchunk->source_id = source_id;

		rawchunk->dlt = KDLT_BTBB;

		newpack->insert(_PCM(PACK_COMP_LINKFRAME), rawchunk);

		printf("debug - Got packet chan %d len=%d\n", packet_queue[x]->channel, packet_queue[x]->len);

		num_packets++;

		kis_ref_capsource *csrc_ref = new kis_ref_capsource;
		csrc_ref->ref_source = this;
		newpack->insert(_PCM(PACK_COMP_KISCAPSRC), csrc_ref);

		globalreg->packetchain->ProcessPacket(newpack);

		// Delete the temp struct and data
		delete packet_queue[x]->data;
		delete packet_queue[x];
	}

	// Flush the queue
	packet_queue.clear();

	//printf("debug - packet queue cleared %d\n", packet_queue.size());

	pthread_mutex_unlock(&packet_lock);

	return 1;
}
