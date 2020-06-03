/* Bluetooth physical layer
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
#include <ubtbr/bb.h>
#include <ubtbr/btbb.h>
#include <ubtbr/debug.h>
#include <ubtbr/mem_pool.h>
#include <ubtbr/hop.h>
#include <ubtbr/rf.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/tx_task.h>
#include <ubtbr/rx_task.h>
#include <ubtbr/btphy.h>
#include <ubtbr/system.h>

#define DEFAULT_BDADDR 0x112233445566

typedef void (*btphy_timer_fn_t)(void *arg);

#define BTPHY_TIMER_COUNT 10
typedef struct {
	uint32_t instant;
	btphy_timer_fn_t cb;
	void *cb_arg;
	uint8_t anyway;	// do it anyway if instant as already passed
} btphy_timer_t;

struct {
	unsigned count;
	btphy_timer_t timers[BTPHY_TIMER_COUNT];
} btphy_timers;

btphy_t btphy = {0};

static void btphy_timers_reset(void)
{
	unsigned i;

	btphy_timers.count = 0;
	for(i=0;i<BTPHY_TIMER_COUNT;i++)
		btphy_timers.timers[i].cb = NULL;
}

static void btphy_timers_execute(void)
{
	unsigned i;	// FIXME: unnecessary
	unsigned j;
	uint32_t clkn = btphy_cur_clkn();
	btphy_timer_t *t;

	for (i=0,j=0,t=btphy_timers.timers
		; j<btphy_timers.count && i<BTPHY_TIMER_COUNT
		; i++,t++)
	{
		if (t->cb)
		{
			if (t->instant == clkn)
			{
				t->cb(t->cb_arg);
				t->cb = NULL;
				btphy_timers.count--;
			}
			else if (t->instant < clkn)
			{
				if (t->anyway)
				{
					cprintf("(X)");
					t->cb(t->cb_arg);
				}
				else{
					cprintf("(missed timer)");
				}
				t->cb = NULL;
				btphy_timers.count--;
			}
			j++;
		}
	}
}

void btphy_timer_add(uint32_t instant, btphy_timer_fn_t cb, void *cb_arg, uint8_t anyway)
{
	unsigned i;
	btphy_timer_t *t;

	if (btphy_timers.count == BTPHY_TIMER_COUNT)
	{
		DIE("too many timers");
	}
	for(i=0, t=btphy_timers.timers; i<BTPHY_TIMER_COUNT; i++,t++)
	{
		if (t->cb == NULL)
		{
			t->instant = instant;
			t->cb = cb;
			t->cb_arg = cb_arg;
			t->anyway = anyway;
			btphy_timers.count++;
			return;
		}
	}
	DIE("timers overflow");
}

static void btphy_handler(void)
{
	btphy.master_clkn++;
	btphy.slave_clkn ++;

	if (btphy.clkn_delayed)
	{
		/* reset T0MR value */
		T0MR0 = (CLKN_OFFSET_RESET_VAL)-1;
		btphy.clkn_delayed = 0;
	}

	/* execute timers */
	btphy_timers_execute();

	/* Execute scheduler */
	tdma_sched_execute();

	/* Advance scheduler */
	tdma_sched_advance();
}

void TIMER0_IRQHandler()
{
	/* Timer0: tdma frame */
	if (T0IR & TIR_MR0_Interrupt)
	{
		/* Clear interrupt */
		T0IR = TIR_MR0_Interrupt;
		btphy_handler();

		/* Clear pending interrupt */
		if (T0IR)
		{
			console_putc('O');
			T0IR = TIR_MR0_Interrupt;
		}
	}
}

void btphy_adj_clkn_delay(int delay)
{
	T0MR0 = (CLKN_OFFSET_RESET_VAL+delay)-1;
	btphy.slave_clkn_delay += delay;
	btphy.clkn_delayed = 1;
}

void btphy_cancel_clkn_delay(void)
{
	unsigned delay = btphy.slave_clkn_delay % CLKN_OFFSET_RESET_VAL;
	unsigned clk_delay = btphy.slave_clkn_delay / CLKN_OFFSET_RESET_VAL;

	btphy.master_clkn += clk_delay;
	btphy_adj_clkn_delay(delay);
}

void clkn_start(void)
{
	unsigned prescale;

	/* Disable tdma interrupt */
	ICER0 = ICER0_ICE_TIMER0;
	ICER0 = ICER0_ICE_TIMER1;
	
	// stop timers
	T0TCR = 0;
	T1TCR = 0;
	btphy.master_clkn = 0;

	/* Timer 0: 100ns adjustable clkn interrupt */
	T0PR = CLKN_OFFSET_PRESCALE_VAL-1;
	/* Match register 0: CLKN_RATE */
	T0MR0 = CLKN_OFFSET_RESET_VAL-1;
	/* Match control register: Reset TC, trigger interrupt. */
	T0MCR = TMCR_MR0R | TMCR_MR0I;

	/* Timer 1: clkn value */
	/* Prescale from peripheral clock -> clkn clock */
	T1PR = (PERIPH_CLK_RATE/CLKN_RATE)-1;
	/* Match register 0: nothing */
	T1MR0 = 0xffffffff-1;
	/* Match control register: Reset TC, no interrupt . */
	T1MCR = TMCR_MR0R;

	/* Enable timer0 interrupt */
	ISER0 = ISER0_ISE_TIMER0;

	/* Start the timers */
	T1TCR = 1;
	T0TCR = 1;
}

/* TODO: move this ?*/
uint8_t btphy_whiten_seed(uint32_t clk)
{
	/* Initialize whiten seed */
	switch(btphy.mode)
	{
	case BT_MODE_INQUIRY:
	case BT_MODE_PAGING:
	case BT_MODE_INQUIRY_SCAN:
	case BT_MODE_PAGE_SCAN:
	/* BT 5.1|Vol 2, Part B, 7.2 :
	 * Instead of the master clock, the X-input used in the inquiry or page response (depending on current state)
	 * routine shall be used The 5-bit value shall be extended with two MSBs of value 1 */
		return hop_state.x|0x60;
	case BT_MODE_MASTER:
	case BT_MODE_SLAVE:
		return (clk>>1)|0x40;
	default:
		DIE("Invalid rx mode %d\n", btphy.mode);
	}
}


void btphy_set_mode(btphy_mode_t mode, uint32_t lap, uint8_t uap)
{
	uint64_t sw = btbb_gen_syncword(lap);

	btphy.chan_lap = lap;
	btphy.chan_uap = uap;
	btphy.chan_sw = sw;

	btphy.chan_sw_lo[0] = 0xff&(sw>>0);
	btphy.chan_sw_lo[1] = 0xff&(sw>>8);
	btphy.chan_sw_lo[2] = 0xff&(sw>>16);
	btphy.chan_sw_lo[3] = 0xff&(sw>>24);

	btphy.chan_sw_hi[0] = 0xff&(sw>>32);
	btphy.chan_sw_hi[1] = 0xff&(sw>>40);
	btphy.chan_sw_hi[2] = 0xff&(sw>>48);
	btphy.chan_sw_hi[3] = 0xff&(sw>>56);

	btphy.chan_trailer = (btphy.chan_sw >> 63) ? 0x0a : 0x05;
	btphy_rf_cfg_sync(reverse32(btphy.chan_sw & 0xffffffff));
	/* BT Core Spec v5.2|Vol 2, Part B, table 2.3:
	 * A23_0=lap, and A27_24=uap3_0 */
	hop_init(lap|(uap<<24));
	btphy.mode = mode;
}

void btphy_set_bdaddr(uint64_t bdaddr)
{
	btphy.my_lap = 0xffffff & bdaddr;
	btphy.my_uap = 0xff & (bdaddr>>24);
	btphy.my_nap = 0xffff & (bdaddr>>32);
	btphy.my_sw = btbb_gen_syncword(btphy.my_lap);
	cprintf("Set bdaddr %llx -> sw %llx\n", bdaddr, btphy.my_sw);
}

void btphy_init(void)
{
	tdma_sched_reset();

	/* Start timers */
	clkn_start();

	/* Initialize RF */
	btphy_rf_init();

	tx_task_reset();
	rx_task_reset();

	btphy.chan_sw = 0;
	btphy_timers_reset();

	btphy_set_bdaddr(DEFAULT_BDADDR);
}
