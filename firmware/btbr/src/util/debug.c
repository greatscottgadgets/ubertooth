/* Debug
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
#include <stdarg.h>
#include <string.h>
#include <ubertooth.h>
#include "tinyprintf.h"
#include <ubtbr/ubertooth_usb.h>
#include <ubtbr/console.h>
#include <ubtbr/system.h>

static void a_putc(void *arg, char c)
{
	console_putc(c);
}

void cprintf(char *fmt, ...)
{
	va_list ap;
	void *ret;

	va_start(ap, fmt);
	tfp_format(NULL, a_putc, fmt, ap);
	va_end(ap);
}

void early_printf(char *fmt, ...)
{
	uint32_t flags = irq_save_disable();
	char buf[64] = {0};
	int len;
	va_list ap;
	void *ret;
	
	buf[0] = BTUSB_EARLY_PRINT;
	va_start(ap, fmt);
	len = tfp_vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	usb_send_sync((void*)buf, 1+len);

	irq_restore(flags);
}

void print_hex(uint8_t *pkt, unsigned size)
{
	unsigned i;
	for (i=0;i<size;i++)
		cprintf("%02x ", pkt[i]);
}

void active_msleep(unsigned msec)
{
	int i;
	for (i=msec/2;i;i--);
}

// flash LEDs a certain number of times
void flash_leds(int count, int delay) {
	int i;

	for (i = 0; i < count; ++i) {
		USRLED_SET;
		RXLED_SET;
		TXLED_SET;
		active_msleep(delay);
		USRLED_CLR;
		RXLED_CLR;
		TXLED_CLR;
		active_msleep(delay);
	}
}

void xUSB_ERROR(void)
{
	while (1)
	{
		flash_leds(32, 300);
		flash_leds(16, 600);
		usb_work();
	}
}

void xTDMA_ERROR(void)
{
	while (1)
	{
		flash_leds(16, 300);
		flash_leds(4, 600);
		usb_work();
	}
}

void xDMA_ERROR(void)
{
	while (1)
	{
		flash_leds(8, 300);
		flash_leds(4, 600);
		usb_work();
	}
}

void xOTHER_ERROR(void)
{
	while (1)
	{
		flash_leds(2,300);
		flash_leds(1,600);
		usb_work();
	}
}

void xWIN(void)
{
	while (1)
	{
		flash_leds(1,150);
		usb_work();
	}
}
