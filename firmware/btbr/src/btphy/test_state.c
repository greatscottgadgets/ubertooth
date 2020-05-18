/* Test state
 *
 * Copyright 2020 Etienne Helluy-Lafont
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
#include <ubtbr/debug.h>
#include <ubtbr/btphy.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/bb.h>
#include <ubtbr/tx_task.h>

#define TEST_UAP 0x7e
#define TEST_LAP 0xb2c866

#define TEST_MAX_TICKS	(CLKN_RATE*15)

#define TX_TEST_DELAY	4

#define MIN_CHAN	55
#define MAX_CHAN	55

#define MIN_BUF		36
#define MAX_BUF		150

static struct {
	uint8_t chan;
	uint32_t clkn_start;
	uint8_t fhs_data[20];
	bbhdr_t fhs_hdr;
} tx_test_state;

static int tx_test_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_TEST;
}

static void prepare_fhs_packet(void)
{
	bbhdr_t *hdr= &tx_test_state.fhs_hdr;

	memset(tx_test_state.fhs_data, 0, sizeof(tx_test_state.fhs_data));
	
	/* Write syncword based on slave's lap */
	//bbpkt_write_access_code(tx_test_state.fhs_air_data, btphy.chan_sw);

	/* A proper FHS header */ 
	hdr->lt_addr = 0;
	hdr->type = BB_TYPE_FHS;
	hdr->flags = 0;

	/* Prepare constant part of FHS payload */
	bbpkt_fhs_prepare_payload(tx_test_state.fhs_data,
		btphy.my_sw&0xffffffff, 	// parity: low32 bits of master's sw
		btphy.my_lap,		// master's bdaddr
		btphy.my_uap,		// 
		btphy.my_nap,		// 
		0x5A020C,		// class: FIXME I have no idea what this is
		1,
		0
		);

	/* Write clk27_2 in fhs payload */
	bbpkt_fhs_finalize_payload(tx_test_state.fhs_data, 12345678);
}

static void tx_test_cb(void *arg)
{
	if (tx_test_canceled())
	{
		return;
	}
	/* FIXME: wrap */
	if (btphy.master_clkn >= tx_test_state.clkn_start + TEST_MAX_TICKS)
	{
		cprintf("test timeout\n");
		btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_TIMEOUT);
		return;
	}
	prepare_fhs_packet();
	/* tx skipping 4 bytes of sw */
	tx_task_schedule(3&(3-CUR_MASTER_SLOT_IDX()),
		tx_test_cb, NULL,
		&tx_test_state.fhs_hdr, tx_test_state.fhs_data);
}


/* dummy tick handler to start TX'ing with a proper clock */
static int tx_test_start_sync(uint8_t p1, uint8_t p2, uint16_t p3)
{
	unsigned delay = 4+(3&(3-CUR_MASTER_SLOT_IDX()));
	prepare_fhs_packet();
	/* tx skipping 4 bytes of sw */
	tx_task_schedule(delay,
		tx_test_cb, NULL,
		&tx_test_state.fhs_hdr, tx_test_state.fhs_data);
	return 0;
}

/*
 * TX Test task
 */
void tx_test_state_setup(void)
{
	uint8_t delay=4;

	tx_test_state.chan = MIN_CHAN;

	/* Simulate FHS directed to the inquiry channel: */
	btphy_set_mode(BT_MODE_MASTER, TEST_LAP, TEST_UAP);

	/* FIXME: debug */
	prepare_fhs_packet();
	unsigned i;
	for (i=0;i<sizeof(tx_test_state.fhs_data); i++)
		cprintf("%2d:%02x ", i, tx_test_state.fhs_data[i]);

	tx_test_state.clkn_start = btphy.master_clkn;
	tdma_schedule(2, tx_test_start_sync, 0,0,0,-3);
	btctl_set_state(BTCTL_STATE_TEST, BTCTL_REASON_SUCCESS);
}
