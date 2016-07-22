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

#include "ubertooth_fifo.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

fifo_t* fifo_init()
{
	fifo_t* fifo = (fifo_t*)malloc(sizeof(fifo_t));

	fifo->read_ptr = 0;
	fifo->write_ptr = 0;

	return fifo;
}

void fifo_inc_write_ptr(fifo_t* fifo)
{
	if (fifo->read_ptr != (fifo->write_ptr+1) % FIFO_SIZE) {
		fifo->write_ptr = (fifo->write_ptr+1) % FIFO_SIZE;
	} else {
		fprintf(stderr, "FIFO overflow, packet discarded\n");
	}
}

uint8_t fifo_empty(fifo_t* fifo)
{
	return (fifo->read_ptr == fifo->write_ptr);
}

void fifo_push(fifo_t* fifo, const usb_pkt_rx* packet)
{
	memcpy(&(fifo->packets[fifo->write_ptr]), packet, sizeof(usb_pkt_rx));

	fifo_inc_write_ptr(fifo);
}

usb_pkt_rx fifo_pop(fifo_t* fifo)
{
	size_t selected = fifo->read_ptr;

	fifo->read_ptr = (fifo->read_ptr+1) % FIFO_SIZE;

	return fifo->packets[selected];
}

usb_pkt_rx* fifo_get_write_element(fifo_t* fifo)
{
	return &(fifo->packets[fifo->write_ptr]);
}
