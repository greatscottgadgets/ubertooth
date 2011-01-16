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

#ifndef __PACKETSOURCE_UBERTOOTH_H__
#define __PACKETSOURCE_UBERTOOTH_H__

#include "config.h"

#include <packetsource.h>

extern "C" {
	#include <bluetooth_packet.h>
	#include "ubertooth.h"
}

#define USE_PACKETSOURCE_UBERTOOTH

#define NUM_BANKS 2
#define BANK_LEN 400

class PacketSource_Ubertooth : public KisPacketSource {
public:
	PacketSource_Ubertooth() {
		fprintf(stderr, "FATAL OOPS: Packetsource_Ubertooth()\n");
		exit(1);
	}

	PacketSource_Ubertooth(GlobalRegistry *in_globalreg) :
		KisPacketSource(in_globalreg) {

	}

	virtual KisPacketSource *CreateSource(GlobalRegistry *in_globalreg,
										  string in_interface,
										  vector<opt_pair> *in_opts) {
		return new PacketSource_Ubertooth(in_globalreg, in_interface, in_opts);
	}

	virtual int AutotypeProbe(string in_device);

	virtual int RegisterSources(Packetsourcetracker *tracker) {
		tracker->RegisterPacketProto("ubertooth", this, "UBERTOOTH", 0);
		return 1;
	}

	PacketSource_Ubertooth(GlobalRegistry *in_globalreg, string in_interface,
					   vector<opt_pair> *in_opts);

	virtual ~PacketSource_Ubertooth();

	virtual int ParseOptions(vector<opt_pair> *in_opts);

	virtual int OpenSource();
	virtual int CloseSource();

	virtual int FetchChannelCapable() { return 1; }
	virtual int EnableMonitor() { return 1; }
	virtual int DisableMonitor() { return 1; }

	virtual int SetChannel(unsigned int in_ch);

	virtual int FetchDescriptor();
	virtual int Poll();

	struct ubertooth_bt_pkt {
		char *data;
		int len;
		int channel;
	};

protected:
	virtual void FetchRadioData(kis_packet *in_packet) { };

	int btbb_packet_id;

	int thread_active;

	pthread_t cap_thread;

	// Named USB interface
	string usb_dev;

	struct libusb_device_handle* devh;

	// FD pipes
	int fake_fd[2];

	// Packet storage, locked with packet_lock
	vector<struct ubertooth_bt_pkt *> packet_queue;
    
	// Pending packet, locked with packet_lock
	int pending_packet;

	// Error from thread
	string thread_error;

	char symbols[NUM_BANKS][BANK_LEN];
	int bank;
	uint8_t rx_buf1[BUFFER_SIZE];
	uint8_t rx_buf2[BUFFER_SIZE];

	unsigned int channel;

	uint8_t *empty_buf;
	uint8_t *full_buf;
	bool really_full;
	struct libusb_transfer *rx_xfer;

	pthread_mutex_t packet_lock;

	friend void enqueue(PacketSource_Ubertooth *, packet *, int);
	friend void cb_xfer(struct libusb_transfer *);
	friend void *ubertooth_cap_thread(void *);

};

#endif
