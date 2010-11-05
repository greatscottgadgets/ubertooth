/*
 * Copyright 2010 Michael Ossmann
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
 *
 * adapted from TestThroughput.c from micropendous project:
 * Created: 2008-08-15 by Opendous Inc. - basic Custom-Class USB communication example
 * Edit: 2009-08-28 by www.ilmarin.info - all SpeedTest-related code
 * Last Edit: 2009-10-04 by Opendous Inc. - minor alterations
 */
#include <stdio.h>
#include <usb.h>

#define VENDORID     0xffff
#define PRODUCTID    0x0005
#define IN_EP        0x82
#define IN_EP_SIZE   64
#define OUT_EP       0x05
#define OUT_EP_SIZE  64
#define CONFIGNUM    1
#define INTERFACENUM 1
#define TIMEOUT      2000
#define BUFFER_SIZE  102400

/* tell ubertooth to stream rx */
void send_command_rx(usb_dev_handle *udev)
{
	const unsigned char command = 'r';
	int ret;

	ret = usb_bulk_write(udev, OUT_EP, &command, 1, TIMEOUT);
	if (ret != 1) {
		printf("[Write] returned: %d , failed to write\n", ret);
		exit(1);
	}
}

/* stream ubertooth output to stdout */
void stream_rx(usb_dev_handle *udev, int block_size)
{
	unsigned char buffer[BUFFER_SIZE];
	int ret;
	int i, j;

	if (block_size > BUFFER_SIZE)
		block_size = BUFFER_SIZE;

	while (1) {
		ret = usb_bulk_read(udev, IN_EP, buffer, block_size, TIMEOUT);
		if (ret < 0) {
			printf("[Read] returned: %d , failed to read\n", ret);
			continue;
		}
		for (i = 0; i < ret; i++)
			for (j = 0; j < 8; j++) {
				printf("%c", (buffer[i] & 0x80) >> 7 );
				buffer[i] <<= 1;
			}
		fflush(stderr);
	}
}

int main(void)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *udev;
	int ret;

	/* let libusb print all debug messages */
	usb_set_debug(255);

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev=bus->devices;dev;dev=dev->next) {

			if ((dev->descriptor.idVendor==VENDORID) && (dev->descriptor.idProduct==PRODUCTID)) {

				udev = usb_open(dev);

				/* tell libusb to use the CONFIGNUM configuration of the device */
				usb_set_configuration(udev, CONFIGNUM);

				#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
				/* detach the automatically assigned kernel driver from the current interface */
				usb_detach_kernel_driver_np(udev, INTERFACENUM);
				#endif

				/* claim the interface for use by this program */
				ret = usb_claim_interface(udev, INTERFACENUM);
				if (ret != 0) {
					printf("failed to claim interface\n");
					return ret;
				}

				send_command_rx(udev);
				stream_rx(udev,512);

				/* make sure other programs can still access this device */
				/* release the interface and close the device */
				usb_release_interface(udev, INTERFACENUM);
				usb_close(udev);
			}
		}
	}
    return 0;
}
