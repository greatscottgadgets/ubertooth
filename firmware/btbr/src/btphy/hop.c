/* Hop functions
 *
 * Copyright 2012 Dominic Spill
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
#include <ubtbr/cfg.h>
#include <ubtbr/debug.h>
#include <ubtbr/hop.h>
#include <ubtbr/btphy.h>

hop_state_t hop_state;

/* do all of the one time precalculation */
void hop_init(uint32_t address)
{
	uint8_t i;

	/* populate frequency register bank */
	for (i = 0; i < NUM_BREDR_CHANNELS; i++)
	{
		hop_state.basic_bank[i] = ((i * 2) % NUM_BREDR_CHANNELS);
	}
		/* actual frequency is 2402 + bank[i] MHz */

	/* precalculate some of next_hop()'s variables */
	hop_state.a27_23 = (address >> 23) & 0x1f;
	hop_state.a22_19 = (address >> 19) & 0x0f; 	// Addr22_19
	hop_state.C = ((address >> 4) & 0x10) |
			((address >> 3) & 0x08) |
			((address >> 2) & 0x04) |
			((address >> 1) & 0x02) |
			(address & 0x01);
	hop_state.a18_10 = (address >> 10) & 0x1ff;
	hop_state.E =	((address >> 7) & 0x40) +
			((address >> 6) & 0x20) +
			((address >> 5) & 0x10) +
			((address >> 4) & 0x08) +
			((address >> 3) & 0x04) +
			((address >> 2) & 0x02) +
			((address >> 1) & 0x01);
	hop_state.x = 0;
	hop_state.afh_enabled = 0;
	hop_state.bank = hop_state.basic_bank;
	hop_state.chan_count = NUM_BREDR_CHANNELS;
}

/* BT Core v5.1 | Vol 2, Part B, Section 2.6*/
static uint8_t hop_selection_kernel(uint8_t x, uint8_t y1, uint8_t y2,
			uint8_t a, uint8_t b, uint8_t c,
			uint16_t d, uint8_t e, uint8_t f)
{
	/* Extend y1 on 5 bits */
	y1 = ~(y1-1);
	return (perm5((a+x)^b, y1^c, d) + e + f + y2);
}

uint8_t hop_inquiry(uint32_t clk)
{
	uint8_t clk1, sel;

	clk1 = 1&(clk>>1);

	sel = hop_selection_kernel(
			hop_state.x,		// X = the x
			clk1,			// Y1 = clk1
			clk1<<5,		// Y2 = 32*clk1
			hop_state.a27_23,	// A = A27_23
			hop_state.a22_19,	// B = A22_19
			hop_state.C,		// C = A8,6,4,2,0
			hop_state.a18_10,	// D = A18_10
			hop_state.E,		// E = A13,11,9,7,5,3,1
			0) % NUM_BREDR_CHANNELS;

	return hop_state.basic_bank[sel];
}

static void hop_set_afh_map(uint8_t *afh_map)
{
	uint8_t chan;
	unsigned i,count;

	for (i = 0, count=0; i < NUM_BREDR_CHANNELS; i++) {
		//chan = (i * 2) % NUM_BREDR_CHANNELS;
		chan = hop_state.basic_bank[i];
		if(afh_map[chan/8] & (0x1 << (chan%8)))
			hop_state.afh_bank[count++] = chan;
	}
	hop_state.afh_chan_count = count;
}

void hop_cfg_afh(uint8_t* buf)
{
	uint8_t enabled = buf[0] & 1;
	uint8_t *map = buf+1;

	if (enabled)
	{
		cprintf("(+afh)");
		hop_set_afh_map(map);
		hop_state.bank = hop_state.afh_bank;
		hop_state.chan_count = hop_state.afh_chan_count;
	}
	else
	{
		cprintf("(-afh)");
		hop_state.bank = hop_state.basic_bank;
		hop_state.chan_count = NUM_BREDR_CHANNELS;
	}
	hop_state.afh_enabled = enabled;
}

uint8_t hop_basic(uint32_t clk)
{
	uint8_t clk1, sel;

	// Same-channel mechanism for AFH (FIXME: is this correct ?)
	if (hop_state.afh_enabled)
		clk1 = 0;
	else
		clk1 = 1&(clk>>1);

	sel = hop_selection_kernel(
		0x1f & (clk>>2),			// X = clk6_2
		clk1,					// Y1 = clk1
		clk1<<5,				// Y2 = 32*clk1
		hop_state.a27_23^(0x1f&(clk>>21)),	// A = A27_23 ^ clk25_21
		hop_state.a22_19,			// B = A22_19
		hop_state.C^(0x1f&(clk>>16)),		// C = A8,6,4,2,0 ^ clk20_16
		hop_state.a18_10^(0x1ff&(clk>>7)),	// D = a18_10 ^ clk15_7
		hop_state.E,				// E = A13,11,9,7,5,3,1
		((clk>>(7-4))&(0x1fffff<<4))%hop_state.chan_count	// F = 16*clk27_7 % 79
		);
	return hop_state.bank[sel%hop_state.chan_count];
}

uint8_t hop_channel(uint32_t clk)
{
	switch(btphy.mode)
	{
	case BT_MODE_INQUIRY:
	case BT_MODE_PAGING:
	case BT_MODE_INQUIRY_SCAN:
	case BT_MODE_PAGE_SCAN:
		return hop_inquiry(clk);
	case BT_MODE_MASTER:
	case BT_MODE_SLAVE:
		return hop_basic(clk);
	default:
		DIE("Invalid hop mode %d\n", btphy.mode);
	}
}
