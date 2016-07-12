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

#ifndef __UBERTOOTH_CLOCK_H
#define __UBERTOOTH_CLOCK_H value

#include "inttypes.h"

/*
 * CLK100NS is a free-running clock with a period of 100 ns.  It resets every
 * 2^15 * 10^5 cycles (about 5.5 minutes) - computed from clkn and timer0 (T0TC)
 *
 * clkn is the native (local) clock as defined in the Bluetooth specification.
 * It advances 3200 times per second.  Two clkn periods make a Bluetooth time
 * slot.
 */
volatile uint32_t clkn;
volatile uint32_t last_hop;

volatile uint32_t clkn_offset;
volatile uint16_t clk100ns_offset;

// linear clock drift
volatile int16_t clk_drift_ppm;
volatile uint16_t clk_drift_correction;

volatile uint32_t clkn_last_drift_fix;
volatile uint32_t clkn_next_drift_fix;

#define CLK100NS (3125*(clkn & 0xfffff) + T0TC)
#define LE_BASECLK (12500)                    // 1.25 ms in units of 100ns

void clkn_stop();
void clkn_start();
void clkn_init();

#endif
