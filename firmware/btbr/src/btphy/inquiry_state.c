/* Inquiry state
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
#include <ubtbr/tx_task.h>

/* Max duration : 60 seconds */
#define INQ_MAX_TICKS (CLKN_RATE*60)

static struct {
	uint32_t clkn_start;
} inquiry_state;

static int inquiry_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_INQUIRY;
}

#define TX_PREPARE_IDX	3  // We will transmit at clkn1_0 = 0

static void inquiry_schedule(unsigned delay);

static int inquiry_rx_cb(msg_t *msg, void *arg, int time_offset)
{
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;
	btctl_rx_pkt_t *pkt;

	if (inquiry_canceled())
	{
		msg_free(msg);
		return 0;
	}

	if (h->type != BTCTL_RX_PKT)
		DIE("paging : expect acl rx");
	pkt = (btctl_rx_pkt_t *)h->data;

	if (!BBPKT_GOOD_CRC(pkt))
	{
		msg_free(msg);
		inquiry_schedule(3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));	// schedule tx in next slot
	}
	else
	{
		if (pkt->bb_hdr.type == BB_TYPE_FHS)
		{
			/* Schedule rx EFHS for next RX slot*/
			rx_task_schedule(2,
				inquiry_rx_cb, NULL,	// efhs rx callback
				1<<RX_F_PAYLOAD		// wait for payload
			);
		}
		else
		{
			/* Schedule normal inquiry */
			inquiry_schedule(3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));
		}
		if (btctl_tx_enqueue(msg))
		{
			DIE("txq full in inquiry\n");
		}
	}
	return 0;
}

static void inquiry_schedule(unsigned delay)
{
	/* FIXME: wrap */
	if (btphy.master_clkn >= inquiry_state.clkn_start + INQ_MAX_TICKS)
	{
		cprintf("inquiry timeout\n");
		btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_TIMEOUT);
		return;
	}

	/* Schedule TX */
	tx_task_schedule(delay,
		NULL, NULL,
		NULL, NULL); // No packet / no payload

	/* Schedule rx fhs: */
	rx_task_schedule(delay+2,
		inquiry_rx_cb, NULL,	// fhs rx callback
		1<<RX_F_PAYLOAD		// wait for payload
		);
}

/* dummy tick handler to start TX'ing with a proper clock */
static int inquiry_start_sync(uint8_t p1, uint8_t p2, uint16_t p3)
{
	unsigned delay;

	/* Use the master clkn */
	btphy_cancel_clkn_delay();
	delay = 4+(3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));
	inquiry_state.clkn_start = btphy.master_clkn+delay;
	inquiry_schedule(delay);
	return 0;
}

/*
 * Inquiry master
 */
void inquiry_state_setup(void)
{
	/* Init only with the lap, DCI(0) is used as UAP */
	btphy_set_mode(BT_MODE_INQUIRY, GIAC, 0);

	tdma_schedule(2, inquiry_start_sync, 0,0,0,-3);
	btctl_set_state(BTCTL_STATE_INQUIRY, BTCTL_REASON_SUCCESS);
}
