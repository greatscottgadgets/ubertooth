/* Fault handlers
 *
 * Copyright 2019 Mike Ryan
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
#include <ubertooth.h>
#include <ubtbr/debug.h>
#include <ubtbr/ubertooth_usb.h>
#include <ubtbr/system.h>
#include "tinyprintf.h"

void __attribute__((noreturn)) die (char *fmt, ...)
{
	int i;
	char buf[64] = {0};
	int len;
	va_list ap;
	void *ret;
	irq_save_disable();

	// This looks a bit like early_printf
	buf[0] = BTUSB_EARLY_PRINT;
	va_start(ap, fmt);
	len = tfp_vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	usb_send_sync((void*)buf, 1+len);

	while (1)
	{
		flash_leds(5, 100);
	}
}

static void dump_debug_registers(void)
{
	int i;
	extern uint32_t _StackTop;
	volatile uint32_t _stk[1] = {0xdeadbeef}, *stk;
	early_printf("CFSR=%08x\n", SCB_CFSR);
	early_printf("HFSR=%08x\n", SCB_HFSR);
	early_printf("MMSR=%08x\n", SCB_MMSR);
	early_printf("BFSR=%08x\n", SCB_BFSR);
	early_printf("UFSR=%08x\n", SCB_UFSR);
	early_printf("BFAR=%08x\n", SCB_BFAR);

	early_printf("\nStack dump (~%p):\n", _stk);
	for (stk=(uint32_t*)((uint32_t)&_stk&~0xf);stk<&_StackTop;stk+=4)
	{
		early_printf("%p: %08x %08x %08x %08x\n",
			stk, stk[0], stk[1], stk[2], stk[3]);
	}
}

void HardFault_Handler(void)
{
	early_printf("HardFault at 0x%x\n", get_lr());
	dump_debug_registers();
	while (1) {
		flash_leds(2,500);
		flash_leds(25,100);
	}
}

void MemManagement_Handler(void)
{
	early_printf("MMFault at 0x%x\n", get_lr());
	dump_debug_registers();
	while (1) {
		flash_leds(4,500);
		flash_leds(12,100);
	}
}

void BusFault_Handler(void)
{
	early_printf("BusFault at %x\n", get_lr());
	dump_debug_registers();
	while (1) {
		flash_leds(8,500);
		flash_leds(6,100);
	}
}

void UsageFault_Handler(void)
{
	early_printf("UsageFault at %x\n", get_lr());
	dump_debug_registers();
	while (1) {
		flash_leds(8,500);
		flash_leds(6,100);
	}
}
