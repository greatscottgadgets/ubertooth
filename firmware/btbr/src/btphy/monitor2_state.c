/* Monitor connection state
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
#include <string.h>
#include <ubtbr/cfg.h>
#include <ubtbr/btctl_intf.h>
#include <ubtbr/btphy.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/bb.h>
#include <ubtbr/rx_task.h>

#define RX_PREPARE_IDX	1  //  We will receive in all timeslots where clkn1_0 = 0

static struct {
	/* Parameters */
	uint64_t master_bdaddr;
} monitor2_state;

static void monitor2_state_schedule_rx(unsigned skip_slots);

static int monitor2_state_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_CONNECTED;
}

static int monitor2_rx_cb(msg_t *msg, void *arg, int time_offset)
{
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;
	btctl_rx_pkt_t *pkt;
	int rc;

	if (monitor2_state_canceled())
	{
		msg_free(msg);
		return 0;
	}
	/* Check if a packet was received, to adjust clock if needed */
	if (h->type != BTCTL_RX_PKT)
		DIE("rx : expect acl rx");
	pkt = (btctl_rx_pkt_t *)h->data;

	if (!BBPKT_HAS_PKT(pkt))
	{
		//console_putc('N');
		goto end;
	}

	if ((pkt->clkn&2) == 0) // Packet from master
	{
		/* Resync to master */
		btphy_adj_clkn_delay(time_offset);
	}

	if (!BBPKT_HAS_HDR(pkt))
	{
		goto end;
	}
#if 1
	if (pkt->bb_hdr.type < 3) // NULL / POLL / FHS
	{
		cprintf("(%c,%x,%x)","NPF"[pkt->bb_hdr.type], pkt->clkn&0xff,pkt->bb_hdr.flags);
	}
#endif
	if (BBPKT_HAS_CRC(pkt))
	{
		if(btctl_tx_enqueue(msg) != 0)
		{
			DIE("mon2: txq full");
		}
		goto end_nofree;
	}
end:
	msg_free(msg);
end_nofree:
	monitor2_state_schedule_rx(0);
	return 0;
}

static void monitor2_state_schedule_rx(unsigned skip_slots)
{
	/* Listen for a packet in next rx slot */
	unsigned delay = 2*skip_slots + (1&(RX_PREPARE_IDX - (btphy.slave_clkn&1)));

	/* Schedule rx: */
	rx_task_schedule(delay,
		monitor2_rx_cb, NULL,	// ID rx callback
		(1<<RX_F_PAYLOAD)	// wait for header 
		|(1<<RX_F_RAW));	// ignore size and crc in payload
}

void monitor2_state_init(uint64_t master_bdaddr)
{
	monitor2_state.master_bdaddr = master_bdaddr;

	/* Initialize basic hopping from master's clock */
	btphy_set_mode(BT_MODE_SLAVE, master_bdaddr&0xffffff, 0xff & (master_bdaddr>>24));

	cprintf("monitor2 started\n");
	btctl_set_state(BTCTL_STATE_CONNECTED, BTCTL_REASON_PAGED);
	monitor2_state_schedule_rx(1);
}
