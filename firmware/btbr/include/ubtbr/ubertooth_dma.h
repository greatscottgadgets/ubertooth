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

#include "cfg.h"
#include <stdint.h>
void dma_init_gen(volatile void *buf);

typedef void (*dma_tx_cb_t)(void *arg);

void dma_poweron(void);
void dma_poweroff(void);
void dio_ssp_stop(void);
void dio_ssp_start_rx(void);
void dma_init_rx_single(volatile void *buf, unsigned buf_size);
unsigned long dma_get_rx_offset(void);
void dio_ssp_start_tx(void);
void dma_init_tx_single(volatile void *buf, unsigned buf_size, dma_tx_cb_t cb, void *cb_arg);
unsigned long dma_get_tx_offset(void);
int dma_get_error(void);
#endif
