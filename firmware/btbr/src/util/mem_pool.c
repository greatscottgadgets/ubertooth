/* Memory pool
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
#include <stddef.h>
#include <ubertooth_interface.h>
#include <ubtbr/debug.h>
#include <ubtbr/system.h>
#include <ubtbr/bb_msg.h>

//#define MEM_DEBUG // this was to fix a small double free

#define BUF_NUM 16

/* shoult be enough for any ACL packet */
#define BUF_SIZE (sizeof(msg_t)+sizeof(btctl_hdr_t)+sizeof(btctl_rx_pkt_t)+344)

typedef struct pool_el_s {
#ifdef MEM_DEBUG
	int allocated;
#endif
	struct pool_el_s *li;
	uint8_t buf[BUF_SIZE];
} pool_el_t;

static struct {
	int init;
	pool_el_t *li;
	pool_el_t el[BUF_NUM];
} pool = {0};

void mem_pool_init(void)
{
	int i;

	for(i=0;i<BUF_NUM-1;i++)
		pool.el[i].li = &pool.el[i+1];
	pool.el[BUF_NUM-1].li = NULL;
	pool.li = pool.el;
	pool.init = 1;
}

void *mem_pool_alloc(unsigned size)
{
	uint32_t flags = irq_save_disable();
	pool_el_t *el;
	void *buf;

	if (!pool.init)
		DIE("pool ny initalized\n");

	if (size > BUF_SIZE)
		DIE("Cannot alloc %d\n", size);

	if ((el = pool.li) == NULL)
		DIE("No more buffers\n");
#ifdef MEM_DEBUG
	if (el->allocated)
		DIE("el already allocated\n");
#endif

	pool.li = el->li;
#ifdef MEM_DEBUG
	el->allocated = 1;
#endif
	irq_restore(flags);

	return &el->buf;
}

void mem_pool_free(void *p)
{
#ifdef MEM_DEBUG
	uint32_t lr = get_lr();
#endif
	pool_el_t * el = (pool_el_t*)((unsigned long)p-offsetof(pool_el_t, buf));
	uint32_t flags = irq_save_disable();

#ifdef MEM_DEBUG
	if (el->allocated != 1)
		DIE("el not allocated %08x|%d\n",lr,el->allocated);
	el->allocated = 0;
#endif

	el->li = pool.li;
	pool.li = el;

	irq_restore(flags);
}
