/* Page state
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
#include <ubtbr/debug.h>
#include <ubtbr/btphy.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/bb.h>
#include <ubtbr/rx_task.h>
#include <ubtbr/tx_task.h>
#include <ubtbr/master_state.h>
#include <ubtbr/btctl_intf.h>

/* Max duration : 45 seconds */
#define PAGING_MAX_TICKS (CLKN_RATE*45)
#define TX_PREPARE_IDX	3  // We will transmit at clkn1_0 = 0

static struct {
	uint32_t clkn_start;
	bbhdr_t fhs_hdr;
	uint8_t fhs_data[20];
} paging_state;

static int paging_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_PAGE;
}

static void paging_schedule(unsigned delay);

/* Timeframe:
 * usec                 0             625                   1250              1875
 * clk          -1      0        1     2          3            4         5    6
 * master:      prep tx | tx id  |  .. |       	  | rx/prep tx | tx fhs  | .. |
 * slave:               |        |  .. | tx id(2) |            |         | .. |tx id(3)
 */

/* We received ID(3) from paged device */
static int paging_rx_ack_cb(msg_t *msg, void *arg, int time_offset)
{
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;
	btctl_rx_pkt_t *pkt;

	if (paging_canceled())
	{
		msg_free(msg);
		return 0;
	}
	pkt = (btctl_rx_pkt_t *)h->data;

	if (BBPKT_HAS_PKT(pkt))
	{
		cprintf("ID(3)\n");
		master_state_init();
	}
	else{
		cprintf("no ID(3)\n");
		paging_schedule(3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));
	}
	msg_free(msg);
	return 0;
}

/* We received the ID(2) from paged device */
static int paging_rx_cb(msg_t *msg, void *arg, int time_offset)
{
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;
	btctl_rx_pkt_t *pkt;
	unsigned delay; 

	if (paging_canceled())
		goto end;
	pkt = (btctl_rx_pkt_t *)h->data;

	if (BBPKT_HAS_PKT(pkt))
	{
		/* We received the slave's ID(2) at t'(k).
		 * Send FHS in next slot and start basic hopping
		 * The FHS shall be sent on chan f(k+1), following the std paging hop
		 * */
		delay = 3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX());

		/* Write clk27_2 in fhs payload
		 * Will tx at (clkn+1) */
		bbpkt_fhs_finalize_payload(paging_state.fhs_data, (btphy.master_clkn+delay+1)>>2);

		/* Schedule tx masters's page response (FHS): */
		tx_task_schedule(delay, 	// must schedule tx_prepare now to start tx in next clkn
			NULL, NULL,
			/* skip 4 bytes of sw */
			&paging_state.fhs_hdr, paging_state.fhs_data);

		/* Schedule rx slave's ID(3) page response: */
		rx_task_schedule(delay+2,
			paging_rx_ack_cb, NULL,	// ID rx callback
			0			// no payload
			);
	}
	else
	{
		// schedule tx in next slot
		paging_schedule(3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));
	}
end:
	msg_free(msg);
	return 0;
}

static void paging_schedule(unsigned delay)
{
	/* FIXME: wrap */
	if (btphy.master_clkn >= paging_state.clkn_start + PAGING_MAX_TICKS)
	{
		cprintf("paging timeout\n");
		btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_TIMEOUT);
		return;
	}

	/* Schedule TX ID(1) */
	tx_task_schedule(delay,
		NULL, NULL,	// no tx callback
		NULL, NULL); 	// no header / no payload

	/* Schedule rx ID(2): */
	rx_task_schedule(delay+2,
		paging_rx_cb, NULL,	// ID rx callback
		0			// no payload for an ID packet
		);
}

/* dummy tick handler to start TX'ing with a proper clock */
static int paging_start_sync(uint8_t p1, uint8_t p2, uint16_t p3)
{
	unsigned delay;

	/* Use the master clkn */
	btphy_cancel_clkn_delay();
	delay = 4+(3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));
	paging_state.clkn_start = btphy.master_clkn+delay;
	paging_schedule(delay);
	return 0;
}

/*
 * Paging master
 * Refs: timings at BT52|V2PB|Table 8.3
 * In a nutshell, we must:
 * k 0: Tx ID(1) packets with sw of paged device on f(k)
 * k 2: clk1(1): Receive ID(2) packet in response of the paged device 	at clk1(1) 
 * k 4: Tx FHS packet with sw of paged device
 *	if contains master's (low_sw, lap, nap, uap, etc.), and clk27_2 sampled at start of tx.
 * k 6: Rx ID(3) from the slave, and start master task.
 */
void paging_state_setup(uint32_t lap, uint8_t uap)
{
	// Reset paging state
	btphy_set_mode(BT_MODE_PAGING, lap, uap);

	/* A simple Header for our fhs */ 
	paging_state.fhs_hdr.lt_addr = 0;		//  This seems right
	paging_state.fhs_hdr.type = BB_TYPE_FHS;
	paging_state.fhs_hdr.flags = 0; 		// Depends on devices, some sends BTHDR_FLOW|BTHDR_SEQN, others send 0, both seems right

	/* Prepare constant part of FHS payload */
	bbpkt_fhs_prepare_payload(paging_state.fhs_data,
		btphy.my_sw&0xffffffff, 	// parity: low32 bits of master's sw
		btphy.my_lap,		// master's bdaddr
		btphy.my_uap,		// 
		btphy.my_nap,		// 
		0x5A020C,		// class: FIXME make this configurable
		1,			// lt_addr: FIXME: fixed ltaddr = 1
		0 // No eir
		);

	tdma_schedule(2, paging_start_sync, 0,0,0,-3);
	btctl_set_state(BTCTL_STATE_PAGE, BTCTL_REASON_SUCCESS);
}
