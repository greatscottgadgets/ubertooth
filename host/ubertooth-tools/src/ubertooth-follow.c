/*
 * Copyright 2010, 2011 Michael Ossmann
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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#include "ubertooth.h"
#include <btbb.h>
#include <getopt.h>

extern int max_ac_errors;
extern btbb_piconet *follow_pn;
extern FILE *dumpfile;

struct libusb_device_handle *devh = NULL;

static void usage()
{
	printf("ubertooth-follow - active(bluez) CLK discovery and follow for a particular UAP/LAP\n");
	printf("Usage:\n");
	printf("\t-h this help\n");
	printf("\t-l<LAP> (in hexadecimal)\n");
	printf("\t-u<UAP> (in hexadecimal)\n");
	printf("\t-U<0-7> set ubertooth device to use\n");
	printf("\t-r<filename> capture packets to PCAPNG file\n");
#ifdef ENABLE_PCAP
	printf("\t-q<filename> capture packets to PCAP file\n");
#endif
	printf("\t-e max_ac_errors\n");
	printf("\t-d filename\n");
	printf("\t-a Enable AFH\n");
	printf("\t-b Bluetooth device (hci0)\n");
	printf("\t-w USB delay in 625us timeslots (default:5)\n");
	printf("\nLAP and UAP are both required, if not given they are read from the local device, in some cases this may give the incorrect address.\n");
//	printf("If an input file is not specified, an Ubertooth device is used for live capture.\n");
}

int main(int argc, char *argv[])
{
	int opt, sock, dev_id, lap = 0, uap = 0, delay = 5;
	int have_lap = 0;
	int have_uap = 0;
	int afh_enabled = 0;
	uint8_t mode, afh_map[10];
	char *end, ubertooth_device = -1;
	char *bt_dev = "hci0";
    char addr[19] = { 0 };
	uint32_t clock;
	uint16_t accuracy, handle, offset;
	bdaddr_t bdaddr;
	btbb_piconet *pn;
	struct hci_dev_info di;
	int cc = 0;


	pn = btbb_piconet_new();

	while ((opt=getopt(argc,argv,"hl:u:U:e:d:ab:w:r:q:")) != EOF) {
		switch(opt) {
		case 'l':
			lap = strtol(optarg, &end, 16);
			if (end != optarg) {
				++have_lap;
			}
			break;
		case 'u':
			uap = strtol(optarg, &end, 16);
			if (end != optarg) {
				++have_uap;
			}
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'r':
			if (!h_pcapng_bredr) {
				if (btbb_pcapng_create_file( optarg, "Ubertooth", &h_pcapng_bredr )) {
					err(1, "create_bredr_capture_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
#ifdef ENABLE_PCAP
		case 'q':
			if (!h_pcap_bredr) {
				if (btbb_pcap_create_file(optarg, &h_pcap_bredr)) {
					err(1, "btbb_pcap_create_file: ");
				}
			}
			else {
				printf("Ignoring extra capture file: %s\n", optarg);
			}
			break;
#endif
		case 'e':
			max_ac_errors = atoi(optarg);
			break;
		case 'd':
			dumpfile = fopen(optarg, "w");
			if (dumpfile == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'a':
			afh_enabled = 1;
			break;
		case 'b':
			bt_dev = optarg;
			if (bt_dev == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 'w': //wait
			delay = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}

	dev_id = hci_devid(bt_dev);
	sock = hci_open_dev(dev_id);
	hci_read_clock(sock, 0, 0, &clock, &accuracy, 0);

	if ((have_lap != 1) || (have_uap != 1)) {
		printf("No address given, reading address from device\n");
		hci_read_bd_addr(sock, &bdaddr, 0);
		lap = bdaddr.b[0] | bdaddr.b[1] << 8 | bdaddr.b[2] << 16;
		btbb_init_piconet(pn, lap);
		uap = bdaddr.b[3];
		btbb_piconet_set_uap(pn, uap);
		printf("LAP=%06x UAP=%02x\n", lap, uap);
	} else if (have_lap && have_uap) {
		btbb_init_piconet(pn, lap);
		btbb_piconet_set_uap(pn, uap);
		printf("Address given, assuming address is remote\n");
		sprintf(addr, "00:00:%02X:%02X:%02X:%02X",
			uap,
			(lap >> 16) & 0xFF,
			(lap >> 8) & 0xFF,
			lap & 0xFF
		);
		str2ba(addr, &bdaddr);
		printf("Address: %s\n", addr);
	
		if (hci_devinfo(dev_id, &di) < 0) {
			perror("Can't get device info");
			return 1;
		}

		if (hci_create_connection(sock, &bdaddr,
					htobs(di.pkt_type & ACL_PTYPE_MASK),
					0, 0x01, &handle, 25000) < 0) {
			perror("Can't create connection");
			return 1;
		}
		sleep(1);
		cc = 1;

		if (hci_read_clock_offset(sock, handle, &offset, 1000) < 0) {
			perror("Reading clock offset failed");
		}
		clock += offset;
	} else {
			usage();
			return 1;
	}
	
	if (h_pcapng_bredr) {
		btbb_pcapng_record_bdaddr(h_pcapng_bredr,
								  (((uint32_t)uap)<<24)|lap,
								  0xff, 0);
	}
	
	//Experimental AFH map reading from remote device
	if(afh_enabled) {
		if(hci_read_afh_map(sock, handle, &mode, afh_map, 1000) < 0) {
			perror("HCI read AFH map request failed");
			//exit(1);
		}
		if(mode == 0x01) {
			btbb_piconet_set_afh_map(pn, afh_map);
			btbb_print_afh_map(pn);
		} else {
			printf("AFH disabled.\n");
			afh_enabled = 0;
		}
	} else {
		printf("Not use AFH\n");
	}
	if (cc) {
		usleep(10000);
		hci_disconnect(sock, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
	}
	
	/* Clean up on exit. */
	register_cleanup_handler(devh);
	
	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}
	cmd_set_bdaddr(devh, btbb_piconet_get_bdaddr(pn));
	if(afh_enabled)
		cmd_set_afh_map(devh, afh_map);
	btbb_piconet_set_clk_offset(pn, clock+delay);
	btbb_piconet_set_flag(pn, BTBB_FOLLOWING, 1);
	btbb_piconet_set_flag(pn, BTBB_CLK27_VALID, 1);
	follow_pn = pn;
	rx_live(devh, pn, 0);
	ubertooth_stop(devh);

	return 0;
}
