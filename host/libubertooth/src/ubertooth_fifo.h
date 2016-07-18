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

#ifndef __UBERTOOTH_FIFO_H__
#define __UBERTOOTH_FIFO_H__

#include "ubertooth_control.h"

// set fifo size to 1000000 elements or 64 MByte
#define FIFO_SIZE 1000000

typedef struct {
	usb_pkt_rx packets[FIFO_SIZE];
	size_t read_ptr;
	size_t write_ptr;
} fifo_t;

fifo_t* fifo_init();

void fifo_inc_write_ptr(fifo_t* fifo);

void fifo_push(fifo_t* fifo, const usb_pkt_rx* packet);
usb_pkt_rx fifo_pop(fifo_t* fifo);
usb_pkt_rx* fifo_get_write_element(fifo_t* fifo);

uint8_t fifo_empty(fifo_t* fifo);

#endif /* __UBERTOOTH_FIFO_H__ */
