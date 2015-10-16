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

#ifndef __UBERTOOTH_RINGBUFFER_H__
#define __UBERTOOTH_RINGBUFFER_H__

#include "ubertooth_control.h"

typedef struct {
	uint8_t current_bank;
	usb_pkt_rx usb[NUM_BANKS];
	char bt[NUM_BANKS][BANK_LEN];
} ringbuffer_t;

ringbuffer_t* ringbuffer_init();

int ringbuffer_add(ringbuffer_t* rb, const usb_pkt_rx* rx);

usb_pkt_rx* ringbuffer_get_usb(ringbuffer_t* rb, uint8_t index);
usb_pkt_rx* ringbuffer_top_usb(ringbuffer_t* rb);
usb_pkt_rx* ringbuffer_bottom_usb(ringbuffer_t* rb);
char* ringbuffer_get_bt(ringbuffer_t* rb, uint8_t index);
char* ringbuffer_top_bt(ringbuffer_t* rb);
char* ringbuffer_bottom_bt(ringbuffer_t* rb);

#endif /* __UBERTOOTH_RINGBUFFER_H__ */
