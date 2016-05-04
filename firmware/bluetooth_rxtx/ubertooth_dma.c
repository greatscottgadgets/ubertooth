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

#include "ubertooth_dma.h"

/* DMA linked list items */
typedef struct {
	uint32_t src;
	uint32_t dest;
	uint32_t next_lli;
	uint32_t control;
} dma_lli;

dma_lli rx_dma_lli1;
dma_lli rx_dma_lli2;

dma_lli le_dma_lli[11]; // 11 x 4 bytes


static void dma_enable(void)
{
	/* enable DMA */
	DMACC0Config |= DMACCxConfig_E;
	ISER0 = ISER0_ISE_DMA;
}

static void dma_disable(void)
{
	// disable DMA engine:
	// refer to UM10360 LPC17xx User Manual Ch 31 Sec 31.6.1, PDF page 607

	// disable DMA interrupts
	ICER0 = ICER0_ICE_DMA;

	// disable active channels
	DMACC0Config = 0;
	DMACC1Config = 0;
	DMACC2Config = 0;
	DMACC3Config = 0;
	DMACC4Config = 0;
	DMACC5Config = 0;
	DMACC6Config = 0;
	DMACC7Config = 0;
	DMACIntTCClear = 0xFF;
	DMACIntErrClr = 0xFF;

	// Disable the DMA controller by writing 0 to the DMA Enable bit in the DMACConfig
	// register.
	DMACConfig &= ~DMACConfig_E;
	while (DMACConfig & DMACConfig_E);

	/* reset interrupt counters */
	rx_tc = 0;
	rx_err = 0;

	active_rxbuf = &rxbuf1[0];
	idle_rxbuf = &rxbuf2[0];
}

void dma_init()
{
	/* power up GPDMA controller */
	PCONP |= PCONP_PCGPDMA;

	dma_disable();

	/* DMA linked lists */
	rx_dma_lli1.src = (uint32_t)&(DIO_SSP_DR);
	rx_dma_lli1.dest = (uint32_t)&rxbuf1[0];
	rx_dma_lli1.next_lli = (uint32_t)&rx_dma_lli2;
	rx_dma_lli1.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	rx_dma_lli2.src = (uint32_t)&(DIO_SSP_DR);
	rx_dma_lli2.dest = (uint32_t)&rxbuf2[0];
	rx_dma_lli2.next_lli = (uint32_t)&rx_dma_lli1;
	rx_dma_lli2.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	/* configure DMA channel 1 */
	DMACC0SrcAddr = rx_dma_lli1.src;
	DMACC0DestAddr = rx_dma_lli1.dest;
	DMACC0LLI = rx_dma_lli1.next_lli;
	DMACC0Control = rx_dma_lli1.control;
	DMACC0Config = DIO_SSP_SRC
	               | (0x2 << 11)       /* peripheral to memory */
	               | DMACCxConfig_IE   /* allow error interrupts */
	               | DMACCxConfig_ITC; /* allow terminal count interrupts */
}

void dma_init_le()
{
	int i;

	/* power up GPDMA controller */
	PCONP |= PCONP_PCGPDMA;

	dma_disable();

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	for (i = 0; i < 11; ++i) {
		le_dma_lli[i].src = (uint32_t)&(DIO_SSP_DR);
		le_dma_lli[i].dest = (uint32_t)&rxbuf1[4 * i];
		le_dma_lli[i].next_lli = i < 10 ? (uint32_t)&le_dma_lli[i+1] : 0;
		le_dma_lli[i].control = 4 |
				(1 << 12) |        /* source burst size = 4 */
				(0 << 15) |        /* destination burst size = 1 */
				(0 << 18) |        /* source width 8 bits */
				(0 << 21) |        /* destination width 8 bits */
				DMACCxControl_DI | /* destination increment */
				DMACCxControl_I;   /* terminal count interrupt enable */
	}

	/* configure DMA channel 0 */
	DMACC0SrcAddr = le_dma_lli[0].src;
	DMACC0DestAddr = le_dma_lli[0].dest;
	DMACC0LLI = le_dma_lli[0].next_lli;
	DMACC0Control = le_dma_lli[0].control;
	DMACC0Config =
			DIO_SSP_SRC |
			(0x2 << 11) |     /* peripheral to memory */
			DMACCxConfig_IE | /* allow error interrupts */
			DMACCxConfig_ITC; /* allow terminal count interrupts */
}


void dio_ssp_start()
{
	/* make sure the (active low) slave select signal is not active */
	DIO_SSEL_SET;

	/* enable rx DMA on DIO_SSP */
	DIO_SSP_DMACR |= SSPDMACR_RXDMAE;
	DIO_SSP_CR1 |= SSPCR1_SSE;

	dma_enable();

	/* activate slave select pin */
	DIO_SSEL_CLR;
}

void dio_ssp_stop()
{
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable DMA on SSP; disable SSP
	DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;
	DIO_SSP_CR1 &= ~SSPCR1_SSE;

	dma_disable();
}
