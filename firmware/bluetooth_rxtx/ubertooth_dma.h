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

#ifndef __UBERTOOTH_DMA_H
#define __UBERTOOTH_DMA_H value

#include "inttypes.h"
#include "ubertooth.h"

volatile uint8_t rxbuf1[DMA_SIZE];
volatile uint8_t rxbuf2[DMA_SIZE];

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
volatile uint8_t* volatile active_rxbuf;
volatile uint8_t* volatile idle_rxbuf;

/* rx terminal count and error interrupt counters */
volatile uint32_t rx_tc;
volatile uint32_t rx_err;

void dma_init();
void dma_init_le();
void dio_ssp_start();
void dio_ssp_stop();

#endif
