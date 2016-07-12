/*
 * Copyright 2015 Hannes Ellinger
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

#include "ubertooth_clock.h"
#include "ubertooth.h"

void clkn_stop()
{
	/* stop and reset the timer to zero */
	T0TCR = TCR_Counter_Reset;

	clkn = 0;
	last_hop = 0;

	clkn_offset = 0;
	clk100ns_offset = 0;

	clk_drift_ppm = 0;
	clk_drift_correction = 0;

	clkn_last_drift_fix = 0;
	clkn_next_drift_fix = 0;
}

void clkn_start()
{
	/* start timer */
	T0TCR = TCR_Counter_Enable;
}

void clkn_init()
{
	/*
	 * Because these are reset defaults, we're assuming TIMER0 is powered on
	 * and in timer mode.  The TIMER0 peripheral clock should have been set by
	 * clock_start().
	 */

	clkn_stop();

#ifdef TC13BADGE
	/*
	 * The peripheral clock has a period of 33.3ns.  3 pclk periods makes one
	 * CLK100NS period (100 ns).
	 */
	T0PR = 2;
#else
	/*
	 * The peripheral clock has a period of 20ns.  5 pclk periods
	 * makes one CLK100NS period (100 ns).
	 */
	T0PR = 4;
#endif
	/* 3125 * 100 ns = 312.5 us, the Bluetooth clock (CLKN). */
	T0MR0 = 3124;
	T0MCR = TMCR_MR0R | TMCR_MR0I;
	ISER0 = ISER0_ISE_TIMER0;
}
