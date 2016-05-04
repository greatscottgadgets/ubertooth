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

#ifndef UBERTOOTH_CS_H
#define UBERTOOTH_CS_H

#include "inttypes.h"

#define CS_HOLD_TIME  2      // min pkts to send on trig (>=1)

uint8_t cs_no_squelch;       // rx all packets if set
int8_t cs_threshold_req;     // requested CS threshold in dBm
int8_t cs_threshold_cur;     // current CS threshold in dBm
volatile uint8_t cs_trigger; // set by intr on P2.2 falling (CS)

void cs_threshold_calc_and_set(uint16_t channel);
void cs_trigger_enable(void);
void cs_trigger_disable(void);
void cs_reset(void);

#endif
