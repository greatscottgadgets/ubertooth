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

#include "bluetooth_le.h"

u16 btle_next_hop()
{
	u16 channel, i;
	btle_channel = (btle_channel + hop_increment) % BTLE_CHANNELS;
	channel = btle_channel;
	for(i=0; i<ADVERTISING_CHANNELS; i++)
		if (channel == advertising_channels[i]) {
			channel = (channel + 1) % BTLE_CHANNELS;
			break;
		}
	return 2042 + 2*channel;
}

u32 received_data = 0;

int btle_find_access_address(u8 *idle_rxbuf)
{
	/* Looks for an AA in the stream */
	u16 count;
	u8 curr_buf;
	int i = 0;

	if (received_data == 0) {
		for (; i<8; i++) {
			received_data <<= 8;
			received_data |= idle_rxbuf[i];
		}
	}
	curr_buf = idle_rxbuf[i];

	// Search until we're 32 symbols from the end of the buffer
	for(count = 0; count < ((8 * DMA_SIZE) - 32); count++)
	{
		if (received_data == access_address)
			return count;

		if (count%8 == 0)
			curr_buf = idle_rxbuf[++i];

		received_data <<= 1;
		curr_buf <<= 1;
	}
	return -1;
}

u8 btle_channel_index(u8 channel) {
	u8 idx;
	channel /= 2;
	if (channel == 0)
		idx = 37;
	else if (channel < 12)
		idx = channel - 1;
	else if (channel == 12)
		idx = 38;
	else if (channel < 39)
		idx = channel - 2;
	else
		idx = 39;
	return idx;
}
