/* Monitor paging state
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
#include <ubtbr/btphy.h>
#include <ubtbr/bb.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/scan_task.h>
#include <ubtbr/rx_task.h>
#include <ubtbr/tx_task.h>
#include <ubtbr/monitor_state.h>

#define PAGE_SCAN_MAX_TICKS (CLKN_RATE*60)
#define TX_PREPARE_IDX	1  // We will transmit at clkn1_0 = 2

struct {
	fhs_info_t fhs_info;
	uint32_t clkn_start;
} monitor1_state;

static void monitor1_schedule(unsigned delay);

static int monitor1_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_PAGE_SCAN;
}

static int start_monitor2(uint8_t p1, uint8_t p2, uint16_t p3)
{
	cprintf("fhs: ba=%llx clk27_2=%x lta=%d\n",
		monitor1_state.fhs_info.bdaddr,
		monitor1_state.fhs_info.clk27_2,
		monitor1_state.fhs_info.lt_addr);

	monitor2_state_init(monitor1_state.fhs_info.bdaddr);
	return 0;
}

/* RX FHS cb */
static int monitor1_rx_fhs_cb(msg_t *msg, void *arg, int time_offset)
{
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;
	btctl_rx_pkt_t *pkt;

	if (monitor1_canceled())
	{
		cprintf("monitor1 canceled\n");
		goto end;
	}
	pkt = (btctl_rx_pkt_t *)h->data;
	if (BBPKT_GOOD_CRC(pkt))
	{
		if (pkt->bb_hdr.type != BB_TYPE_FHS)
		{
			cprintf("(bad type %d)", pkt->bb_hdr.type);
			goto end;
		}
		// Parse fhs to get bdaddr/clkn/ltaddr
		bbpkt_decode_fhs(pkt->bt_data, &monitor1_state.fhs_info);


		// Set slave clkn
		btphy.slave_clkn = (monitor1_state.fhs_info.clk27_2<<2)
				 + (btphy.slave_clkn-pkt->clkn);
		
		// Start hopping in next timeslot
		tdma_schedule(1, start_monitor2, 0, 0, 0, -3);

		// Send FHS to host
		btctl_tx_enqueue(msg);
		goto end_nofree;
	}
	else{
		if (BBPKT_HAS_HDR(pkt) && pkt->bb_hdr.type)
			console_putc('0'+pkt->bb_hdr.type);
		monitor1_schedule(0);
	}
end:
	msg_free(msg);
end_nofree:
	return 0;
}

/* RX ID(1) cb */
static void monitor1_rx_id_cb(int sw_detected, void *arg)
{
	unsigned delay;
	if(monitor1_canceled())
	{
		cprintf("monitor1 canceled\n");
		return;
	}
	if (sw_detected)
	{
		/* we should be at slave_clk = 0, prepare in one slot*/
		console_putc('!');
		/* Schedule TX ID(2) */
		delay = 3&(TX_PREPARE_IDX-CUR_SLAVE_SLOT_IDX());
		tx_task_schedule(delay,
			NULL, NULL,	// no tx callback
			NULL, NULL); 	// no header / no payload

		/* Schedule rx FHS */
		rx_task_schedule(delay+2,
			monitor1_rx_fhs_cb, NULL,	// ID rx callback
			1<<RX_F_PAYLOAD			// FHS packet has payload
			);
	}
	else
	{
		monitor1_schedule(0);
	}
}

static void monitor1_schedule(unsigned delay)
{
	if (MASTER_CLKN >= monitor1_state.clkn_start + PAGE_SCAN_MAX_TICKS)
	{
		cprintf("monitor1 timeout\n");
		btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_TIMEOUT);
		return;
	}
	scan_task_schedule(delay, monitor1_rx_id_cb, NULL);
}

/*
 * Monitor paging in a nutshell:
 * 1 - Wait for ID(1) with target's lap
 * 2 - Reply with ID(2)
 * 3 - Receive FHS, do not send ID(3) (thats the trick!!)
 * 4 - Start hopping on piconet and wait for real connection to establish
 * 5 - Sniff all timeslots
 */
void monitor1_state_setup(uint64_t slave_bdaddr)
{
	btphy_set_bdaddr(slave_bdaddr);
	btphy_set_mode(BT_MODE_PAGE_SCAN, btphy.my_lap, btphy.my_uap);
	btctl_set_state(BTCTL_STATE_PAGE_SCAN, BTCTL_REASON_SUCCESS);
	monitor1_state.clkn_start = MASTER_CLKN+1;
	monitor1_schedule(1);
}
