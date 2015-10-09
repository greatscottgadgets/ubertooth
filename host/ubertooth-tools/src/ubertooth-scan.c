/*
 * Copyright 2012 - 2013 Dominic Spill
 * Extra info scan:
 * Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "ubertooth.h"
#include <btbb.h>
#include <getopt.h>

extern int max_ac_errors;

static void usage()
{
	printf("ubertooth-scan - active(bluez) device scan and inquiry supported by Ubertooth\n");
	printf("Usage:\n");
	printf("\t-h this Help\n");
	printf("\t-U<0-7> set Ubertooth device to use\n");
	printf("\t-t scan Time (seconds) - length of time to sniff packets. [Default: 20s]\n");
	printf("\t-e max_ac_errors (default: %d, range: 0-4)\n", max_ac_errors);
	printf("\t-s hci Scan - perform the equivalent of 'hcitool scan'\n");
	printf("\t-x eXtended scan - retrieve additional information about target devices\n");
	printf("\t-b Bluetooth device (hci0)\n");
}


void extra_info(int dd, int dev_id, bdaddr_t* bdaddr)
{
	uint16_t handle, offset;
	uint8_t features[8], max_page = 0;
	char name[249], *tmp;
	char addr[19] = { 0 };
	uint8_t mode, afh_map[10];
	struct hci_version version;
	struct hci_dev_info di;
	struct hci_conn_info_req *cr;
	int i, cc = 0;
	
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

	if (hci_read_clock_offset(dd, handle, &offset, 1000) < 0) {
		perror("Reading clock offset failed");
		exit(1);
	}

	printf("\tClock offset: 0x%4.4x\n", btohs(offset));

	if(hci_read_afh_map(dd, handle, &mode, afh_map, 1000) < 0) {
	perror("HCI read AFH map request failed");
	}
	if(mode == 0x01) {
		// DGS: Replace with call to btbb_print_afh_map - need a piconet
		printf("\tAFH Map: 0x");
		for(i=0; i<10; i++)
			printf("%02x", afh_map[i]);
		printf("\n");
	} else {
		printf("AFH disabled.\n");
	}
	free(cr);
	
	if (cc) {
		usleep(10000);
		hci_disconnect(dd, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
	}
}

/* For a given BD_ADDR, print address, name and class */
void print_name_and_class(int dev_handle, int dev_id, bdaddr_t *bdaddr,
						  char* printable_addr, uint8_t extended)
{
    char name[248] = { 0 };

	if (hci_read_remote_name(dev_handle, bdaddr, sizeof(name), name, 0) < 0)
			strcpy(name, "[unknown]");

	printf("%s\t%s\n", printable_addr, name);
	if (extended)
		extra_info(dev_handle, dev_id, bdaddr);
}


int main(int argc, char *argv[])
{
    inquiry_info *ii = NULL;
	int i, opt, dev_id, dev_handle, len, flags, max_rsp, num_rsp, lap, timeout = 20;
	uint8_t uap, extended = 0;
	uint8_t scan = 0;
	char ubertooth_device = -1;
	char *bt_dev = "hci0";
    char addr[19] = { 0 };
	struct libusb_device_handle *devh = NULL;
	btbb_piconet *pn;
	bdaddr_t bdaddr;

	while ((opt=getopt(argc,argv,"hU:t:e:xsb:")) != EOF) {
		switch(opt) {
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
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
		case 'e':
			max_ac_errors = atoi(optarg);
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
	if (dev_id < 0) {
		printf("error: Unable to find %s (%d)\n", bt_dev, dev_id);
		return 1;
	}

	dev_handle = hci_open_dev( dev_id );
	if (dev_handle < 0) {
		perror("HCI device open failed");
		return 1;
	}

	devh = ubertooth_start(ubertooth_device);
	if (devh == NULL) {
		usage();
		return 1;
	}
	/* Set sweep mode - otherwise AFH map is useless */
	cmd_set_channel(devh, 9999);

	if (scan) {
		/* Equivalent to "hcitool scan" */
		printf("HCI scan\n");
		len  = 8;
		max_rsp = 255;
		flags = IREQ_CACHE_FLUSH;
		ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
		
		num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
		if( num_rsp < 0 )
			perror("hci_inquiry");
	
		for (i = 0; i < num_rsp; i++) {
			ba2str(&(ii+i)->bdaddr, addr);
			print_name_and_class(dev_handle, dev_id, &(ii+i)->bdaddr, addr,
								 extended);
		}
		free(ii);
	}

	/* Now find hidden piconets with Ubertooth */
	printf("\nUbertooth scan\n");
	btbb_init_survey();
	rx_live(devh, NULL, timeout);
	ubertooth_stop(devh);

	while((pn=btbb_next_survey_result()) != NULL) {
		lap = btbb_piconet_get_lap(pn);
		if (btbb_piconet_get_flag(pn, BTBB_UAP_VALID)) {
			lap = btbb_piconet_get_lap(pn);
			uap = btbb_piconet_get_uap(pn);
			sprintf(addr, "00:00:%02X:%02X:%02X:%02X", uap,
					(lap >> 16) & 0xFF, (lap >> 8) & 0xFF, lap & 0xFF);
			str2ba(addr, &bdaddr);
			/* Printable version showing that the NAP is unknown */
			sprintf(addr, "??:??:%02X:%02X:%02X:%02X", uap,
					(lap >> 16) & 0xFF, (lap >> 8) & 0xFF, lap & 0xFF);
			print_name_and_class(dev_handle, dev_id, &bdaddr, addr, extended);
		} else
			printf("??:??:??:%02X:%02X:%02X\n", (lap >> 16) & 0xFF,
				   (lap >> 8) & 0xFF, lap & 0xFF);
		btbb_print_afh_map(pn);
	}

    close(dev_handle);
    return 0;
}
