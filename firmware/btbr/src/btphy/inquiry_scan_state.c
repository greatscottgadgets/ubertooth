/* Inquiry scan state
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
#include <ubtbr/tx_task.h>

#define INQ_SCAN_MAX_TICKS (CLKN_RATE*60)
#define TX_PREPARE_IDX	1  // We will transmit at clkn1_0 = 2


static struct {
	uint32_t clkn_start;
	bbhdr_t fhs_hdr;
	uint8_t fhs_data[20];
} inquiry_scan_state;

static void inquiry_scan_schedule(unsigned delay);

static int inquiry_scan_canceled(void)
{
	return btctl_get_state() != BTCTL_STATE_INQUIRY_SCAN;
}

static void tx_eir_cb(void *arg)
{
	inquiry_scan_schedule(1);
}

static void tx_fhs_cb(void *arg)
{
	btctl_tx_pkt_t *eir_pkt = btctl_get_eir();
	if (eir_pkt)
	{
		tx_task_schedule(3&(TX_PREPARE_IDX-CUR_SLAVE_SLOT_IDX()),
			tx_eir_cb, NULL,
			&eir_pkt->bb_hdr, eir_pkt->bt_data);
	}
	else
	{
		inquiry_scan_schedule(1);
	}
}

static void inquiry_scan_rx_cb(int sw_detected, void *arg)
{

	if (inquiry_scan_canceled())
	{
		cprintf("inq scan canceled\n");
		return;
	}

	if (sw_detected)
	{
		console_putc('!');
		/* Schedule TX FHS
		 * We're in the scan_wait, in same tick as rx.
		 * we must prepare in next tick to start tx at rx+2 */
			
		/* Write clk27_2 in fhs payload
		 * Will tx at (clkn+2) */
		bbpkt_fhs_finalize_payload(inquiry_scan_state.fhs_data, (btphy.master_clkn+2)>>2);

		/* Schedule tx FHS */
		tx_task_schedule(3&(TX_PREPARE_IDX-CUR_SLAVE_SLOT_IDX()),
			tx_fhs_cb, NULL,
			&inquiry_scan_state.fhs_hdr, inquiry_scan_state.fhs_data);
	}
	else
	{
		inquiry_scan_schedule(0); // do it again
	}
}

static void inquiry_scan_schedule(unsigned delay)
{
	if (MASTER_CLKN >= inquiry_scan_state.clkn_start + INQ_SCAN_MAX_TICKS)
	{
		cprintf("inquiry scan timeout\n");
		btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_TIMEOUT);
		return;
	}
	scan_task_schedule(delay, inquiry_scan_rx_cb, NULL);
}

/*
 * Inquiry slave
 */
void inquiry_scan_state_setup(void)
{
	/* Init only with the lap, DCI(0) is used as UAP */
	btphy_set_mode(BT_MODE_INQUIRY_SCAN, GIAC, 0);

	btctl_set_state(BTCTL_STATE_INQUIRY_SCAN, BTCTL_REASON_SUCCESS);

	/* A simple Header for our fhs */ 
	inquiry_scan_state.fhs_hdr.lt_addr = 0;
	inquiry_scan_state.fhs_hdr.type = BB_TYPE_FHS;
	inquiry_scan_state.fhs_hdr.flags = 0; 		// Depends on devices, some sends BTHDR_FLOW|BTHDR_SEQN, others send 0, both seems right

	/* Prepare constant part of FHS payload */
	bbpkt_fhs_prepare_payload(inquiry_scan_state.fhs_data,
		btphy.my_sw&0xffffffff,	// parity: low32 bits of master's sw
		btphy.my_lap,		// master's bdaddr
		btphy.my_uap,		// 
		btphy.my_nap,		// 
		0x5A020C,		// class: FIXME: make this configurable
		0,			// lt_addr
		btctl_get_eir() != NULL
		);

	inquiry_scan_state.clkn_start = MASTER_CLKN+1;
	inquiry_scan_schedule(1);
}
