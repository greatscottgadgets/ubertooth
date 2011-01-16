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

/* bulk transfer callback for libusb */
void cb_xfer(struct libusb_transfer *xfer)
{
	PacketSource_Ubertooth *ubertooth =
			(PacketSource_Ubertooth *) xfer->user_data;
	int r;
	u8 *tmp;

	if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
		fprintf(stderr, "rx_xfer status: %d\n", xfer->status);
		libusb_free_transfer(xfer);
		ubertooth->rx_xfer = NULL;
		return;
	}

	while (ubertooth->really_full)
		fprintf(stderr, "uh oh, full_buf not emptied\n");

	tmp = ubertooth->full_buf;
	ubertooth->full_buf = ubertooth->empty_buf;
	ubertooth->empty_buf = tmp;
	ubertooth->really_full = 1;

	ubertooth->rx_xfer->buffer = ubertooth->empty_buf;

	while (1) {
		r = libusb_submit_transfer(ubertooth->rx_xfer);
		if (r < 0)
			fprintf(stderr, "rx_xfer submission from callback: %d\n", r);
		else
			break;
	}
}

void enqueue(PacketSource_Ubertooth *ubertooth, packet *pkt, int channel)
{
	//FIXME should use tun_format() or similar
	char *data = new char[14];
	int len = 14;
	uint32_t lap = pkt->LAP;
	data[0] = data[1] = data[2] = data[3] = data[4] = data[5] = 0x00;
	data[6] = data[7] = data[8] = 0x00;
	data[9] = (lap >> 16) & 0xff;
	data[10] = (lap >> 8) & 0xff;
	data[11] = lap & 0xff;
	data[12] = 0xff;
	data[13] = 0xf0;

	// Lock the packet queue, throw away when there are more than 20 in the queue
	// that haven't been handled, raise the file descriptor hot if we need to
	pthread_mutex_lock(&(ubertooth->packet_lock));

	if (ubertooth->packet_queue.size() > 20) {
		// printf("debug - thread packet queue to big\n");
	} else {
		struct PacketSource_Ubertooth::ubertooth_bt_pkt *rpkt =
				new PacketSource_Ubertooth::ubertooth_bt_pkt;
		rpkt->data = data;
		rpkt->len = len;
		rpkt->channel = channel;

		ubertooth->packet_queue.push_back(rpkt);
		if (ubertooth->pending_packet == 0) {
			// printf("debug - writing to fakefd\n");
			ubertooth->pending_packet = 1;
			write(ubertooth->fake_fd[1], data, 1);
		}

	}
	pthread_mutex_unlock(&(ubertooth->packet_lock));

}

// Capture thread to fake async io
void *ubertooth_cap_thread(void *arg)
{
	PacketSource_Ubertooth *ubertooth = (PacketSource_Ubertooth *) arg;
	int r;
	int i, j, k, m;
	int xfer_size = 512;
	int xfer_blocks;
	uint32_t time; /* in 100 nanosecond units */
	uint32_t clkn; /* native (local) clock in 625 us */
	uint8_t clkn_high;
	char syms[BANK_LEN * NUM_BANKS];
	packet pkt;

	/*
	 * A block is 64 bytes transferred over USB (includes 50 bytes of rx symbol
	 * payload).  A transfer consists of one or more blocks.  Consecutive
	 * blocks should be approximately 400 microseconds apart (timestamps about
	 * 4000 apart in units of 100 nanoseconds).
	 */

	if (xfer_size > BUFFER_SIZE)
		xfer_size = BUFFER_SIZE;
	xfer_blocks = xfer_size / 64;
	xfer_size = xfer_blocks * 64;

	fprintf(stderr, "rx blocks of 64 bytes in %d byte transfers\n", xfer_size);

	ubertooth->empty_buf = &(ubertooth->rx_buf1[0]);
	ubertooth->full_buf = &(ubertooth->rx_buf2[0]);
	ubertooth->really_full = 0;
	ubertooth->rx_xfer = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(ubertooth->rx_xfer, ubertooth->devh, DATA_IN,
			ubertooth->empty_buf, xfer_size, cb_xfer, ubertooth, TIMEOUT);

	cmd_rx_syms(ubertooth->devh, 0);

	r = libusb_submit_transfer(ubertooth->rx_xfer);
	if (r < 0) {
		fprintf(stderr, "rx_xfer submission: %d\n", r);
		goto out;
	}

	while (ubertooth->thread_active) {
		while (!ubertooth->really_full) {
			r = libusb_handle_events(NULL);
			if (r < 0) {
				fprintf(stderr, "libusb_handle_events: %d\n", r);
				goto out;
			}
		}

		/* process each received block */
		for (i = 0; i < xfer_blocks; i++) {
			time = ubertooth->full_buf[4 + 64 * i]
					| (ubertooth->full_buf[5 + 64 * i] << 8)
					| (ubertooth->full_buf[6 + 64 * i] << 16)
					| (ubertooth->full_buf[7 + 64 * i] << 24);
			clkn_high = ubertooth->full_buf[3 + 64 * i];
			//fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			for (j = 0; j < 50; j++) {
				/* output one byte for each received symbol (0 or 1) */
				for (k = 0; k < 8; k++) {
					//printf("%c", (full_buf[j] & 0x80) >> 7 );
					ubertooth->symbols[ubertooth->bank][j * 8 + k] =
							(ubertooth->full_buf[j + 14 + i * 64] & 0x80) >> 7;
					ubertooth->full_buf[j + 14 + i * 64] <<= 1;
				}
			}

			/* awfully repetitious */
			m = 0;
			for (j = 0; j < NUM_BANKS; j++)
				for (k = 0; k < BANK_LEN; k++)
					syms[m++] = ubertooth->symbols[(j + 1 + ubertooth->bank)
							% NUM_BANKS][k];
			ubertooth->bank = (ubertooth->bank + 1) % NUM_BANKS;

			r = sniff_ac(syms, BANK_LEN);
			if  (r > -1) {

				clkn = (clkn_high << 19) | ((time + r * 10) / 6250);

				init_packet(&pkt, &syms[r], BANK_LEN * NUM_BANKS - r);

				printf("GOT PACKET on channel %d, LAP = %06x at time stamp %u, clkn %u\n",
							ubertooth->channel, pkt.LAP, time + r * 10, clkn);
				enqueue(ubertooth, &pkt, ubertooth->channel);
			}
		}
		ubertooth->really_full = 0;
		fflush(stderr);
	}

out:
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

	/* Initialize the pipe, mutex, and reading thread */
	if (pipe(fake_fd) < 0) {
		_MSG("Ubertooth '" + name + "' failed to make a pipe() (this is really "
			 "weird): " + string(strerror(errno)), MSGFLAG_ERROR);
		ubertooth_stop(devh);
		devh = NULL;
		return 0;
	}

	if (pthread_mutex_init(&packet_lock, NULL) < 0) {
		_MSG("Ubertooth '" + name + "' failed to initialize pthread mutex: " +
			 string(strerror(errno)), MSGFLAG_ERROR);
		ubertooth_stop(devh);
		devh = NULL;
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
