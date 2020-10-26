/* Master connection state
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
#include <ubtbr/ll.h>

#define TX_PREPARE_IDX	3  // We will transmit at clkn1_0 = 0
#define RX_PREPARE_IDX	1  //  We will receive at clkn1_0 = 2

static struct {
	link_layer_t ll;
} master_state;

static void master_state_schedule_tx(unsigned skip_slots);
static void master_state_schedule_rx(unsigned skip_slots);

static int master_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_CONNECTED;
}

static int master_rx_cb(msg_t *msg, void *arg, int time_offset)
{
	int rc;

	if (master_canceled())
	{
		msg_free(msg);
		ll_reset(&master_state.ll);
		return 0;
	}
	rc = ll_process_rx(&master_state.ll, msg);

	if (rc < 0)
	{
		ll_reset(&master_state.ll);
		btctl_set_state(BTCTL_STATE_STANDBY, -rc);
	}
	else if (rc == 1)
	{
		master_state_schedule_tx(0);
	}
	else
	{
		master_state_schedule_rx(0);
	}
	return 0;
}

static void master_state_schedule_rx(unsigned skip_slots)
{
	/* Wait for next rx slot */
	unsigned delay = 4*skip_slots+(3&(RX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));

	/* Schedule rx: */
	rx_task_schedule(delay,
		master_rx_cb, NULL,	// ID rx callback
		1<<RX_F_PAYLOAD		// wait for header
	);
}

static void tx_done_cb(void* arg)
{
	master_state_schedule_rx(0);
}

static void master_state_schedule_tx(unsigned skip_slots)
{
	/* Wait for next tx slot */
	unsigned delay = skip_slots * 4 + (3&(TX_PREPARE_IDX-CUR_MASTER_SLOT_IDX()));
	bbhdr_t *tx_hdr;
	uint8_t *tx_data;

	ll_prepare_tx(&master_state.ll, &tx_hdr, &tx_data);

	tx_task_schedule(delay,
		tx_done_cb, NULL,
		tx_hdr, tx_data);
}

/* Called by paging task once the slave acknowledged our FHS */
void master_state_init(void)
{
	/* Initialize basic hopping from master's clock */
	btphy_set_mode(BT_MODE_MASTER, btphy.my_lap, btphy.my_uap);

	ll_init(&master_state.ll, 1, 1); // FIXME: fixed slave lt_addr = 1

	/* We are already in sync, just schedule the first tx (in a while)*/
	cprintf("master started\n");
	master_state_schedule_tx(1);
	btctl_set_state(BTCTL_STATE_CONNECTED, BTCTL_REASON_PAGED);
}
