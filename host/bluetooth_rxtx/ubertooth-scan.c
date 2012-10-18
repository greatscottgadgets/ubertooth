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

/*
 * PLAN
 *
 * Scan with hci device
 * Find LAP/UAP combinations not in the list (using ubertooth)
 * inquiry scan everything in the combined list
 * 
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "ubertooth.h"
#include <bluetooth_packet.h>
#include <bluetooth_piconet.h>
#include <getopt.h>

extern int max_ac_errors;
extern FILE *dumpfile;

static void usage()
{
	printf("ubertooth-scan - active(bluez) device scan and inquiry supported by Ubertooth\n");
	printf("Usage:\n");
	printf("\t-h this Help\n");
	printf("\t-s hci Scan - perform HCI scan\n");
	printf("\t-t scan Time (seconds) - length of time to sniff packets\n");
	printf("\t-x eXtended scan - retrieve additional information about target devices\n");
	printf("\t-b Bluetooth device (hci0)\n");
}


void extra_info(int dd, int dev_id, bdaddr_t* bdaddr)
{
	uint16_t handle;
	uint8_t features[8], max_page = 0;
	char name[249], *tmp;
	char addr[19] = { 0 };
	struct hci_version version;
	struct hci_dev_info di;
	struct hci_conn_info_req *cr;
	int i, cc = 0;

	//if (dev_id < 0)
	//	dev_id = hci_for_each_dev(HCI_UP, find_conn, (long) &bdaddr);
	//
	//if (dev_id < 0)
	//	dev_id = hci_get_route(&bdaddr);
	//
	//if (dev_id < 0) {
	//	fprintf(stderr, "Device is not available or not connected.\n");
	//	exit(1);
	//}
	
	if (hci_devinfo(dev_id, &di) < 0) {
		perror("Can't get device info");
		exit(1);
	}

	printf("Requesting information ...\n");

	cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	if (!cr) {
		perror("Can't get connection info");
		exit(1);
	}

	bacpy(&cr->bdaddr, bdaddr);
	cr->type = ACL_LINK;
	if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
		if (hci_create_connection(dd, bdaddr,
					htobs(di.pkt_type & ACL_PTYPE_MASK),
					0, 0x01, &handle, 25000) < 0) {
			perror("Can't create connection");
			return;
		}
		sleep(1);
		cc = 1;
	} else
		handle = htobs(cr->conn_info->handle);

	ba2str(bdaddr, addr);
	printf("\tBD Address:  %s\n", addr);

	if (hci_read_remote_name(dd, bdaddr, sizeof(name), name, 25000) == 0)
		printf("\tDevice Name: %s\n", name);

	if (hci_read_remote_version(dd, handle, &version, 20000) == 0) {
		char *ver = lmp_vertostr(version.lmp_ver);
		printf("\tLMP Version: %s (0x%x) LMP Subversion: 0x%x\n"
			"\tManufacturer: %s (%d)\n",
			ver ? ver : "n/a",
			version.lmp_ver,
			version.lmp_subver,
			bt_compidtostr(version.manufacturer),
			version.manufacturer);
		if (ver)
			bt_free(ver);
	}

	memset(features, 0, sizeof(features));
	hci_read_remote_features(dd, handle, features, 20000);

	if ((di.features[7] & LMP_EXT_FEAT) && (features[7] & LMP_EXT_FEAT))
		hci_read_remote_ext_features(dd, handle, 0, &max_page,
							features, 20000);

	printf("\tFeatures%s: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
				"0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
		(max_page > 0) ? " page 0" : "",
		features[0], features[1], features[2], features[3],
		features[4], features[5], features[6], features[7]);

	tmp = lmp_featurestostr(features, "\t\t", 63);
	printf("%s\n", tmp);
	bt_free(tmp);

	for (i = 1; i <= max_page; i++) {
		if (hci_read_remote_ext_features(dd, handle, i, NULL,
							features, 20000) < 0)
			continue;

		printf("\tFeatures page %d: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
					"0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n", i,
			features[0], features[1], features[2], features[3],
			features[4], features[5], features[6], features[7]);
	}

	if (cc) {
		usleep(10000);
		hci_disconnect(dd, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
	}
}

int main(int argc, char *argv[])
{
    inquiry_info *ii = NULL;
	int i, opt, dev_id, sock, len, flags, max_rsp, num_rsp, timeout = 20;
	uint8_t extended = 0;
	uint8_t scan = 0;
	char ubertooth_device = -1;
	char *bt_dev = "hci0";
    char addr[19] = { 0 };
    char name[248] = { 0 };
	struct libusb_device_handle *devh = NULL;
	pnet_list_item* pnet_list;
	bdaddr_t bdaddr;

	while ((opt=getopt(argc,argv,"ht:xsb:")) != EOF) {
		switch(opt) {
		case 'b':
			bt_dev = optarg;
			if (bt_dev == NULL) {
				perror(optarg);
				return 1;
			}
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'x':
			extended = 1;
			break;
		case 's':
			scan = 1;
			break;
		case 'h':
		default:
			usage();
			return 1;
		}
	}
	
    dev_id = hci_devid(bt_dev);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        return 1;
	}

	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}

	if (scan) {
		len  = 8;
		max_rsp = 255;
		flags = IREQ_CACHE_FLUSH;
		ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
		
		num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
		if( num_rsp < 0 )
			perror("hci_inquiry");
	
		/* Equivalent to "hcitool scan" */
		printf("HCI scan\n");
		for (i = 0; i < num_rsp; i++) {
			ba2str(&(ii+i)->bdaddr, addr);
			memset(name, 0, sizeof(name));
			if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
			name, 0) < 0)
				strcpy(name, "[unknown]");
			printf("%s  %s\n", addr, name);
		}
		free(ii);
	}

	/* Now find hidden piconets with Ubertooth */
	printf("\nUbertooth scan\n");
	pnet_list = ubertooth_scan(devh, timeout);
	ubertooth_stop(devh);

	while(pnet_list != NULL) {
		if (pnet_list->pnet->have_UAP) {
			//printf("00:00:%02x:%02x:%02x:%02x\n",
			//	pnet_list->pnet->UAP,
			//	(pnet_list->pnet->LAP >> 16) & 0xFF,
			//	(pnet_list->pnet->LAP >> 8) & 0xFF,
			//	pnet_list->pnet->LAP & 0xFF);
			sprintf(addr, "00:00:%02x:%02x:%02x:%02x",
				pnet_list->pnet->UAP,
				(pnet_list->pnet->LAP >> 16) & 0xFF,
				(pnet_list->pnet->LAP >> 8) & 0xFF,
				pnet_list->pnet->LAP & 0xFF
			);
			str2ba(addr, &bdaddr);
			memset(name, 0, sizeof(name));
			if (hci_read_remote_name(sock, &bdaddr, sizeof(name), name, 0) < 0)
				strcpy(name, "[unknown]");
			printf("%s  %s\n", addr, name);
			if (extended)
				extra_info(sock, dev_id, &bdaddr);
		}
		pnet_list = pnet_list->next;
	}

    close( sock );
    return 0;
}
