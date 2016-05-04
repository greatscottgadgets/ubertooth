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

#include "ubertooth_cs.h"
#include "ubertooth.h"
#include "ubertooth_rssi.h"

typedef enum {
	CS_SAMPLES_1 = 1,
	CS_SAMPLES_2 = 2,
	CS_SAMPLES_4 = 3,
	CS_SAMPLES_8 = 4,
} cs_samples_t;

#define CS_THRESHOLD_DEFAULT (int8_t)(-120)


/* Set CC2400 carrier sense threshold and store value to
 * global. CC2400 RSSI is determined by 54dBm + level. CS threshold is
 * in 4dBm steps, so the provided level is rounded to the nearest
 * multiple of 4 by adding 56. Useful range is -100 to -20. */
static void cs_threshold_set(int8_t level, cs_samples_t samples)
{
	level = level < -120 ? -120 : level;
	level = level > -20 ? -20 : level;
	cc2400_set(RSSI, (uint8_t)((level + 56) & (0x3f << 2)) | ((uint8_t)samples&3));
	cs_threshold_cur = level;
	cs_no_squelch = (level <= -120);
}

void cs_threshold_calc_and_set(uint16_t channel)
{
	int8_t level;

	/* If threshold is max/avg based (>0), reset here while rx is
	 * off.  TODO - max-to-iir only works in SWEEP mode, where the
	 * channel is known to be in the BT band, i.e., rssi_iir has a
	 * value for it. */
	if (cs_threshold_req > 0) {
		int8_t rssi = rssi_get_avg(channel);
		level = rssi - 54 + cs_threshold_req;
	} else {
		level = cs_threshold_req;
	}
	cs_threshold_set(level, CS_SAMPLES_4);
}

/* CS comes from CC2400 GIO6, which is LPC P2.2, active low. GPIO
 * triggers EINT3, which could be used for other things (but is not
 * currently). TODO - EINT3 should be managed globally, not turned on
 * and off here. */
void cs_trigger_enable(void)
{
	cs_trigger = 0;
	ISER0 = ISER0_ISE_EINT3;
	IO2IntClr = PIN_GIO6;      // Clear pending
	IO2IntEnF |= PIN_GIO6;     // Enable port 2.2 falling (CS active low)
}

void cs_trigger_disable(void)
{
	IO2IntEnF &= ~PIN_GIO6;    // Disable port 2.2 falling (CS active low)
	IO2IntClr = PIN_GIO6;      // Clear pending
	ICER0 = ICER0_ICE_EINT3;
	cs_trigger = 0;
}

void cs_reset(void)
{
	cs_trigger_disable();

	cs_no_squelch = 0;
	cs_threshold_req=CS_THRESHOLD_DEFAULT;
	cs_threshold_cur=CS_THRESHOLD_DEFAULT;
}
