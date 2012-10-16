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
	printf("\t-t scan Time (seconds) - length of time to sniff packets\n");
	printf("\t-b Bluetooth device (hci0)\n");
}

int main(int argc, char *argv[])
{
    inquiry_info *ii = NULL;
	int i, opt, dev_id, sock, len, flags, max_rsp, num_rsp, timeout = 20;
	char ubertooth_device = -1;
	char *bt_dev = "hci0";
    char addr[19] = { 0 };
    char name[248] = { 0 };
	struct libusb_device_handle *devh = NULL;
	pnet_list_item* pnet_list;
	bdaddr_t bdaddr;

	while ((opt=getopt(argc,argv,"ht:b:")) != EOF) {
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
    free( ii );

	/* Now find hidden piconets with Ubertooth */
	printf("Ubertooth scan\n");
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
		}
		pnet_list = pnet_list->next;
	}

    close( sock );
    return 0;
}
