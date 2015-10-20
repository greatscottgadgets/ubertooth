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

#include "ubertooth_ringbuffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void unpack_symbols(const uint8_t* buf, char* unpacked)
{
	int i, j;


	for (i = 0; i < SYM_LEN; i++) {
		/* output one byte for each received symbol (0x00 or 0x01) */
		for (j = 0; j < 8; j++) {
			unpacked[i * 8 + j] = ((buf[i] << j) & 0x80) >> 7;
		}
	}
}

ringbuffer_t* ringbuffer_init()
{
	ringbuffer_t* rb = (ringbuffer_t*)malloc(sizeof(ringbuffer_t));

	rb->current_bank = 0;

	return rb;
}

int ringbuffer_add(ringbuffer_t* rb, const usb_pkt_rx* rx)
{
	rb->current_bank = (rb->current_bank + 1) % NUM_BANKS;

	/* Copy packet (for dump) */
	memcpy(ringbuffer_top_usb(rb), rx, sizeof(usb_pkt_rx));

	unpack_symbols(ringbuffer_top_usb(rb)->data, ringbuffer_top_bt(rb));

	return 0;
}


usb_pkt_rx* ringbuffer_get_usb(ringbuffer_t* rb, uint8_t index)
{
	return &(rb->usb[(rb->current_bank+1+index) % NUM_BANKS]);
}
usb_pkt_rx* ringbuffer_top_usb(ringbuffer_t* rb)
{
	return ringbuffer_get_usb(rb, NUM_BANKS-1);
}

usb_pkt_rx* ringbuffer_bottom_usb(ringbuffer_t* rb)
{
	return ringbuffer_get_usb(rb, 0);
}

char* ringbuffer_get_bt(ringbuffer_t* rb, uint8_t index)
{
	return rb->bt[(rb->current_bank+1+index) % NUM_BANKS];
}

char* ringbuffer_top_bt(ringbuffer_t* rb)
{
	return ringbuffer_get_bt(rb, NUM_BANKS-1);
}

char* ringbuffer_bottom_bt(ringbuffer_t* rb)
{
	return ringbuffer_get_bt(rb, 0);
}
