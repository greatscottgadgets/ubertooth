/* Console
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
#include <stdlib.h>
#include <ubtbr/cfg.h>
#include <ubtbr/btctl_intf.h>
#include <ubtbr/queue.h>
#include <ubtbr/system.h>

static msg_t *cur_msg = NULL;

void console_flush(void)
{
#ifdef USE_CONSOLE
	uint32_t flags = irq_save_disable();
	char *p;
	if (!cur_msg)
		goto end;
	p = (char*)msg_put(cur_msg, 1);
	*p = 0;
	if (btctl_tx_enqueue(cur_msg))
	{
		DIE("txq full in console\n");
	}
	cur_msg = NULL;
end:
	irq_restore(flags);
#endif
}

void console_putc(char c)
{
#ifdef USE_CONSOLE
	uint32_t flags = irq_save_disable();
	char *p;

	if (cur_msg == NULL)
	{
		cur_msg = btctl_msg_alloc(BTCTL_DEBUG);
	}
	p = (char*)msg_put(cur_msg, 1);
	*p = c;

	if (c == '\n' || msg_write_avail(cur_msg) == 1)
	{
		console_flush();
	}
	irq_restore(flags);
#endif
}
