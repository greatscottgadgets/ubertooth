/* Scan task
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
#include <ubtbr/rf.h>
#include <ubtbr/btctl_intf.h>
#include <ubtbr/btphy.h>
#include <ubtbr/hop.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/scan_task.h>

#define SCAN_MAX_SLOTS	8
#define	SCAN_WAIT_SYNCWORD	250	// Wait 250usec max

static struct {
	/* Arguments */
	scan_task_cb_t cb;
	void * cb_arg;

	/* Variables */
	uint16_t pkt_time;
	uint8_t pkt_received;
	int num_slots;
} scan_task; 

static int scan_wait_more(uint8_t p1, uint8_t p2, uint16_t p3);

static void scan_finalize(void)
{
	btphy_rf_idle();

	if (scan_task.cb == NULL)
		DIE("no cb in scan task\n");

	/* Call user callback */
	scan_task.cb(scan_task.pkt_received, scan_task.cb_arg);
}

static void scan_wait(void)
{
	int clkn_delay;

	/* Wait for packet.. */
	while (CLKN_OFFSET < SCAN_WAIT_SYNCWORD)
	{
		/* PKT strobe goes low if a syncword is detected */
		if (GIO6 == 0)
		{
			scan_task.pkt_time = CLKN_OFFSET;
			scan_task.pkt_received = 1;
			break;
		}
	}
	scan_task.num_slots ++;
	if (scan_task.pkt_received)
	{
		/* Adjust timer*/
		clkn_delay = scan_task.pkt_time - RF_EXPECTED_RX_CLKN_OFFSET;
		btphy.slave_clkn = 0;
		btphy_adj_clkn_delay(clkn_delay);

		// TODO: synchronization train decoding? 

		/* finalize */
		scan_finalize();
	}
	else
	{
		if (scan_task.num_slots == SCAN_MAX_SLOTS)
		{
			scan_finalize();
		}
		else
		{
			/* Wait one more slot */
			tdma_schedule(1, scan_wait_more, 0, 0, 0, -3);
		}
	}
}

static int scan_wait_more(uint8_t p1, uint8_t p2, uint16_t p3)
{
	scan_wait();
	return 0;
}

static int scan_start(uint8_t p1, uint8_t p2, uint16_t p3)
{
	uint8_t chan;

	chan = hop_channel(0);	// Assume clk1 = 0 = master tx

	/* Tune to chan & start RX */
	btphy_rf_tune_chan(2402+chan, 0);

	scan_task.pkt_received = 0;
	scan_task.pkt_time = 0;
	scan_task.num_slots = 0;

	/* Here i must strobe SRX before & after GRMDM cfg. why ? */
	cc2400_strobe(SFSON);
	btphy_rf_cfg_rx();
	btphy_rf_rx();
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_RX);

	scan_wait();

	return 0; 
}

void scan_task_schedule(unsigned delay, scan_task_cb_t cb, void*cbarg)
{
	scan_task.cb = cb;
	scan_task.cb_arg = cbarg;

	/* FIXME: We RX only ~250usec in a 312.5usec timeslot.
	 * Therefore we might get unlucky and never receive the master ID. */

	/* In inquiry/paging scan , hop.x must be incremented before
	 * master TX */
	hop_increment();

	tdma_schedule(1, scan_start, 0, 0, 0, -3);
}
