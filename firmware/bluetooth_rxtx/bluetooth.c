/*
 * Copyright 2012 Dominic Spill
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

#include "bluetooth.h"

/* these values for hop() can be precalculated (at leastin part) */
u8 a1, b, c1, e;
u16 d1;
/* frequency register bank */
u8 bank[CHANNELS];

/* do all of the one time precalculation */
void precalc()
{
	u8 i;
	u32 address;
	address = target.address & 0xffffffff;
	syncword = 0;

	/* populate frequency register bank*/
	for (i = 0; i < CHANNELS; i++)
		bank[i] = ((i * 2) % CHANNELS);
		/* actual frequency is 2402 + bank[i] MHz */


	/* precalculate some of next_hop()'s variables */
	a1 = (address >> 23) & 0x1f;
	b = (address >> 19) & 0x0f;
	c1 = ((address >> 4) & 0x10) +
		((address >> 3) & 0x08) +
		((address >> 2) & 0x04) +
		((address >> 1) & 0x02) +
		(address & 0x01);
	d1 = (address >> 10) & 0x1ff;
	e = ((address >> 7) & 0x40) +
		((address >> 6) & 0x20) +
		((address >> 5) & 0x10) +
		((address >> 4) & 0x08) +
		((address >> 3) & 0x04) +
		((address >> 2) & 0x02) +
		((address >> 1) & 0x01);
}

/* 5 bit permutation */
u8 perm5(u8 z, u8 p_high, u16 p_low)
{
	/* z is constrained to 5 bits, p_high to 5 bits, p_low to 9 bits */
	z &= 0x1f;
	p_high &= 0x1f;
	p_low &= 0x1ff;

	int i;
	u8 tmp, output, z_bit[5], p[14];
	u8 index1[] = {0, 2, 1, 3, 0, 1, 0, 3, 1, 0, 2, 1, 0, 1};
	u8 index2[] = {1, 3, 2, 4, 4, 3, 2, 4, 4, 3, 4, 3, 3, 2};

	/* bits of p_low and p_high are control signals */
	for (i = 0; i < 9; i++)
		p[i] = (p_low >> i) & 0x01;
	for (i = 0; i < 5; i++)
		p[i+9] = (p_high >> i) & 0x01;

	/* bit swapping will be easier with an array of bits */
	for (i = 0; i < 5; i++)
		z_bit[i] = (z >> i) & 0x01;

	/* butterfly operations */
	for (i = 13; i >= 0; i--) {
		/* swap bits according to index arrays if control signal tells us to */
		if (p[i]) {
			tmp = z_bit[index1[i]];
			z_bit[index1[i]] = z_bit[index2[i]];
			z_bit[index2[i]] = tmp;
		}
	}

	/* reconstruct output from rearranged bits */
	output = 0;
	for (i = 0; i < 5; i++)
		output += z_bit[i] << i;

	return output;
}

u16 next_hop(u32 clock)
{
	u8 a, c, x, y1, perm;
	u16 d, y2;
	u32 f;

	/* following variable names used in section 2.6 of the spec */
	x = (clock >> 2) & 0x1f;
	y1 = (clock >> 1) & 0x01;
	y2 = y1 << 5;
	a = (a1 ^ (clock >> 21)) & 0x1f;
	/* b is already defined */
	c = (c1 ^ (clock >> 16)) & 0x1f;
	d = (d1 ^ (clock >> 7)) & 0x1ff;
	/* e is already defined */
	f = (clock >> 3) & 0x1fffff0;

	perm = perm5(
		((x + a) % 32) ^ b,
		(y1 * 0x1f) ^ c,
		d);
	/* hop selection */
	return(2402 + bank[(perm + e + f + y2) % CHANNELS]);

}

/* count the number of 1 bits in a uint64_t */
uint8_t count_bits(uint64_t n)
{
	uint8_t i = 0;
	for (i = 0; n != 0; i++)
		n &= n - 1;
	return i;
}

int find_access_code(u8 *idle_rxbuf)
{
	/* Looks for an AC in the stream */
	u8 bit_errors, curr_buf;
	int i = 0, count = 0;

	if (syncword == 0) {
		for (; i<8; i++) {
			syncword <<= 8;
			syncword = (syncword & 0xffffffffffffff00) | idle_rxbuf[i];
		}
		count = 64;
	}
	curr_buf = idle_rxbuf[i];

	// Search until we're 64 symbols from the end of the buffer
	for(; count < ((8 * DMA_SIZE) - 64); count++)
	{
		bit_errors = count_bits(syncword ^ target.access_code);

		if (bit_errors < MAX_SYNCWORD_ERRS)
			return count;

		if (count%8 == 0)
			curr_buf = idle_rxbuf[++i];

		syncword <<= 1;
		syncword = (syncword & 0xfffffffffffffffe) | ((curr_buf & 0x80) >> 7);
		curr_buf <<= 1;
	}
	return -1;
}
