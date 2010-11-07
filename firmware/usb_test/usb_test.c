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
 */

#include "ubertooth.h"
#include "usb_serial.h"

int main()
{
	int c;

	ubertooth_init();
	usb_serial_init();

	/*
	 * for each character received over USB serial connection, echo the
	 * character back over USB serial and toggle USRLED
	 */
	while (1) {
		c = VCOM_getchar();
		if (c != EOF) {
			/* toggle USRLED */
			if (USRLED)
				USRLED_CLR;
			else
				USRLED_SET;
			VCOM_putchar(c);
		}
	}
}
