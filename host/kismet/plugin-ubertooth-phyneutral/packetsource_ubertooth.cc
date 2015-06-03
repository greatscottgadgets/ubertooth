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

	ParseOptions(in_opts);
}

PacketSource_Ubertooth::~PacketSource_Ubertooth() {
	CloseSource();
}
	

int PacketSource_Ubertooth::ParseOptions(vector<opt_pair> *in_opts) {
	// Fixme = allow multiple ubertooth devices, etc
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

	return 0;
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

void enqueue(PacketSource_Ubertooth *ubertooth, btbb_packet *pkt)
{
	int write_size;
	
	if (btbb_packet_get_ac_errors(pkt) <= 2)
		printf("GOT PACKET ch=%2d LAP=%06x err=%u clk100ns=%u\n",
			   btbb_packet_get_channel(pkt),
			   btbb_packet_get_lap(pkt),
			   btbb_packet_get_ac_errors(pkt),
			   btbb_packet_get_clkn(pkt));
	// Lock the packet queue, throw away when there are more than 20 in the queue
	// that haven't been handled, raise the file descriptor hot if we need to
	pthread_mutex_lock(&(ubertooth->packet_lock));

	if (ubertooth->packet_queue.size() > 20) {
		// printf("debug - thread packet queue too big\n");
	} else {
		ubertooth->packet_queue.push_back(pkt);
		if (ubertooth->pending_packet == 0) {
			// printf("debug - writing to fakefd\n");
			ubertooth->pending_packet = 1;
			write_size = write(ubertooth->fake_fd[1], "bogus", 1);
			if (write_size <= 0)
				printf("Error writing to fakefd\n");
		}
	}
	pthread_mutex_unlock(&(ubertooth->packet_lock));
}

// Capture thread to fake async io
void *ubertooth_cap_thread(void *arg)
{
	PacketSource_Ubertooth *ubertooth = (PacketSource_Ubertooth *) arg;
	int i, j, k, m, r, offset;
	int xfer_size = 512;
	int xfer_blocks;
	uint32_t clkn; /* native (local) clock in 625 us */
	char syms[BANK_LEN * NUM_BANKS];
	usb_pkt_rx *rx;
	
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
 
	cmd_rx_syms(ubertooth->devh);
 
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
			rx = (usb_pkt_rx *)&(ubertooth->full_buf[64 * i]);
			//fprintf(stderr, "rx block timestamp %u * 100 nanoseconds\n", time);
			for (j = 0; j < 50; j++) {
				/* output one byte for each received symbol (0 or 1) */
				for (k = 0; k < 8; k++) {
					//printf("%c", (full_buf[j] & 0x80) >> 7 );
					ubertooth->symbols[ubertooth->bank][j * 8 + k] =
						(rx->data[j] & 0x80) >> 7;
					rx->data[j] <<= 1;
				}
			}
 
			/*
			* Populate syms with enough symbols to run sniff_ac across one
			* bank (BANK_LEN + AC_LEN).
			*/
			m = 0;
			for (j = 0, k = 0; k < BANK_LEN; k++)
					syms[m++] = ubertooth->symbols[(j + 1 + ubertooth->bank)
									% NUM_BANKS][k];
			for (j = 1, k = 0; k < AC_LEN; k++)
					syms[m++] = ubertooth->symbols[(j + 1 + ubertooth->bank)
									% NUM_BANKS][k];
 
			btbb_packet *pkt;
			offset = btbb_find_ac(syms, BANK_LEN, LAP_ANY, 1, &pkt);
			if (offset >= 0) {
				/*
				* Populate syms with the remaining banks.  We don't know how
				* long the packet is, so we assume the maximum length.
				*/
				for (j = 1, k = AC_LEN; k < BANK_LEN; k++)
					syms[m++] = ubertooth->symbols[(j + 1 + ubertooth->bank)
								% NUM_BANKS][k];
				for (j = 2; j < NUM_BANKS; j++)
					for (k = 0; k < BANK_LEN; k++)
						syms[m++] = ubertooth->symbols[(j + 1 + ubertooth->bank)
								% NUM_BANKS][k];
				
				clkn = (rx->clkn_high << 20) + (le32toh(rx->clk100ns) + offset + 1562) / 3125;
				btbb_packet_set_data(pkt, syms + offset,
									 NUM_BANKS * BANK_LEN - offset,
									 rx->channel, clkn);
				enqueue(ubertooth, pkt);
			}
			ubertooth->bank = (ubertooth->bank + 1) % NUM_BANKS;
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
	if ((devh = ubertooth_start(-1)) == NULL) {
		_MSG("Ubertooth '" + name + "' failed to open device '" + usb_dev +
			 "': " + string(strerror(errno)), MSGFLAG_ERROR);
		return 0;
	}

	/* Set sweep mode on startup */
	cmd_set_channel(devh, 9999);

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
	int ret;
	if (in_ch < 0 || in_ch > 78)
		return -1;

	if (thread_active <= 0 || devh == NULL)
		return 0;

	//FIXME actually set the channel

	printf("Switching channel to %d\n", in_ch);
	ret = 0; //cmd_set_channel(devh, (u16) in_ch);
	printf("cmd_set_channel got return value: %d\n", ret);
	if (ret) {
		_MSG("Packet source '" + name + "' failed to set channel "
			 + IntToString(in_ch), MSGFLAG_PRINTERROR);
		return -1;
	}
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

void PacketSource_Ubertooth::build_pcap_header(uint8_t* data, uint32_t lap) {
	data[0] = data[1] = data[2] = data[3] = data[4] = data[5] = 0x00;
	data[6] = data[7] = data[8] = 0x00;
	data[9] = (lap >> 16) & 0xff;
	data[10] = (lap >> 8) & 0xff;
	data[11] = lap & 0xff;
	data[12] = 0xff;
	data[13] = 0xf0;
}

void PacketSource_Ubertooth::build_pcap_payload(uint8_t* data, btbb_packet* pkt) {
	uint32_t clkn;
	uint16_t nap;

	if (btbb_packet_get_flag(pkt, BTBB_NAP_VALID)) {
		nap = btbb_packet_get_nap(pkt);
		data[6] = (nap >> 8) & 0xff;
		data[7] = nap & 0xff;
	}

	data[8] = btbb_packet_get_uap(pkt);

	/* meta data */
	clkn = btbb_packet_get_clkn(pkt);
	data[14] = clkn & 0xff;
	data[15] = (clkn >> 8) & 0xff;
	data[16] = (clkn >> 16) & 0xff;
	data[17] = (clkn >> 24) & 0xff;
	data[18] = btbb_packet_get_channel(pkt);
	data[19] = btbb_packet_get_flag(pkt, BTBB_CLK27_VALID) | (btbb_packet_get_flag(pkt, BTBB_NAP_VALID) << 1);

	/* packet header modified to fit byte boundaries */
	/* lt_addr and type */
	data[20] = (char) (btbb_packet_get_type(pkt) << 3) | btbb_packet_get_lt_addr(pkt);
	/* flags */
	data[21] = (char) btbb_packet_get_header_flags(pkt);
	/* HEC */
	data[22] = (char) btbb_packet_get_hec(pkt);

	btbb_get_payload_packed(pkt, (char *) &data[23]);

}

int PacketSource_Ubertooth::handle_header(btbb_packet* pkt) {
	btbb_piconet *pn;
	uint32_t lap;
	/* Only create new piconets for LAPs we've seen with fewer errors */
	lap = btbb_packet_get_lap(pkt);
	if (piconets.find(lap) == piconets.end()) {
		if (btbb_packet_get_ac_errors(pkt) <= 1) {
			pn = btbb_piconet_new();
			piconets[lap] = pn;
			btbb_init_piconet(pn, lap);
		} else
			return 0;
	}
	pn = piconets[lap];
	if (btbb_piconet_get_flag(pn, BTBB_CLK6_VALID) && btbb_piconet_get_flag(pn, BTBB_UAP_VALID))
		decode_pkt(pkt, pn);
	else
		if (btbb_uap_from_header(pkt, pn))
			decode_pkt(pkt, pn);
	/*
	 * If this is an inquiry response, saving the piconet state will only
	 * cause problems later.
	 */
	if (lap == GIAC || lap == LIAC)
		piconets.erase(lap);

	return 1;
}

/* decode packet with header */
void PacketSource_Ubertooth::decode_pkt(btbb_packet* pkt, btbb_piconet* pn) {
	btbb_process_packet(pkt, pn);
}

int PacketSource_Ubertooth::Poll() {
	char rx;
	int read_size;
	int process_packet;

	// Consume the junk byte we used to raise the FD high
	read_size = read(fake_fd[0], &rx, 1);
	if (read_size <= 0)
		printf("Error reading from fakefd\n");

	pthread_mutex_lock(&packet_lock);

	pending_packet = 0;

	for (unsigned int x = 0; x < packet_queue.size(); x++) {
		kis_packet *newpack = globalreg->packetchain->GeneratePacket();
		btbb_packet *pkt = packet_queue[x];

		newpack->ts.tv_sec = globalreg->timestamp.tv_sec;
		newpack->ts.tv_usec = globalreg->timestamp.tv_usec;

		kis_datachunk *rawchunk = new kis_datachunk;

		process_packet = 1;
		if (btbb_header_present(pkt))
			process_packet = handle_header(pkt);
		else if (btbb_packet_get_ac_errors(pkt) > 1)
			process_packet = 0;

		/*
		 * Only continue processing packet if it has a header and piconet or if
		 * it is an ID packet (without header).
		 */
		if (process_packet) {
			rawchunk->length = 14;
			if (btbb_packet_get_flag(pkt, BTBB_HAS_PAYLOAD))
				rawchunk->length += 9 + btbb_packet_get_payload_length(pkt);
			rawchunk->data = new uint8_t[rawchunk->length];
			build_pcap_header(rawchunk->data, btbb_packet_get_lap(pkt));
			if (btbb_packet_get_flag(pkt, BTBB_HAS_PAYLOAD))
				build_pcap_payload(rawchunk->data, pkt);
	
			rawchunk->source_id = source_id;
	
			rawchunk->dlt = KDLT_BTBB;
	
			newpack->insert(_PCM(PACK_COMP_LINKFRAME), rawchunk);
	
			//printf("debug - Got packet lap %06x chan %d len=%d\n",
			//	   btbb_packet_get_lap(pkt), btbb_packet_get_channel(pkt),
			//	   btbb_packet_get_payload_length(pkt));
	
			num_packets++;
	
			kis_ref_capsource *csrc_ref = new kis_ref_capsource;
			csrc_ref->ref_source = this;
			newpack->insert(_PCM(PACK_COMP_KISCAPSRC), csrc_ref);
	
			globalreg->packetchain->ProcessPacket(newpack);
		}

		// Delete the temp struct
		btbb_packet_unref(pkt);
	}

	// Flush the queue
	packet_queue.clear();

	//printf("debug - packet queue cleared %d\n", packet_queue.size());

	pthread_mutex_unlock(&packet_lock);

	return 1;
}
