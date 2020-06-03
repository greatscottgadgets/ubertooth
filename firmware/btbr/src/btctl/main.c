/* Ubtbr main
 *
 * Copyright 2020 Etienne Helluy-Lafont, Univ. Lille, CNRS.
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
#include <ubertooth.h>
#include <ubtbr/cfg.h>
#include <ubtbr/btctl_intf.h>
#include <ubtbr/vendor_request_handler.h>
#include <ubtbr/btphy.h>

int main()
{
	unsigned clk, clk_prev = 0;

	// enable all fault handlers (see fault.c)
	SCB_SHCSR = (1 << 18) | (1 << 17) | (1 << 16);

	ubertooth_init();
	ubertooth_usb_init(vendor_request_handler);

	/* Initialize memory */
	mem_pool_init();

	/* Initialize control interface */
	btctl_init();

	/* Initialize sync & phy */
	btphy_init();

	while (1) {
		/* Flush console once per second */
		if ((clk = MASTER_CLKN) >= clk_prev+3200)
		{
			console_flush();
			clk_prev = clk;
		}
		/* Handle usb async work */
		usb_work();
		/* Handle host messages */
		btctl_work();
	}
}
