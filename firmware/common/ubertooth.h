/*
 * Copyright 2010 Michael Ossmann
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

#ifndef __UBERTOOTH_H
#define __UBERTOOTH_H

#include "lpc17.h"
#include "types.h"
#include "cc2400.h"

/* GPIO pins */
#define PIN_USRLED (1 << 11) /* P0.11 */
#define PIN_RXLED  (1 << 28) /* P4.28 */
#define PIN_TXLED  (1 << 29) /* P4.29 */
#define PIN_CC1V8  (1 << 29) /* P1.29 */
#define PIN_CC3V3  (1 << 0 ) /* P1.0  */
#define PIN_RX     (1 << 1 ) /* P1.1  */
#define PIN_TX     (1 << 4 ) /* P1.4  */
#define PIN_CSN    (1 << 8 ) /* P1.8  */
#define PIN_SCLK   (1 << 9 ) /* P1.9  */
#define PIN_MOSI   (1 << 10) /* P1.10 */
#define PIN_MISO   (1 << 14) /* P1.14 */
#define PIN_GIO6   (1 << 15) /* P1.15 */
#define PIN_BTGR   (1 << 31) /* P1.31 */
#define PIN_SSEL0  (1 << 9 ) /* P2.9  */

/* indicator LED control */
#define USRLED     (FIO0PIN & PIN_USRLED)
#define USRLED_SET (FIO0SET = PIN_USRLED)
#define USRLED_CLR (FIO0CLR = PIN_USRLED)
#define RXLED      (FIO4PIN & PIN_RXLED)
#define RXLED_SET  (FIO4SET = PIN_RXLED)
#define RXLED_CLR  (FIO4CLR = PIN_RXLED)
#define TXLED      (FIO4PIN & PIN_TXLED)
#define TXLED_SET  (FIO4SET = PIN_TXLED)
#define TXLED_CLR  (FIO4CLR = PIN_TXLED)

/* SSEL0 (SPI slave select) control */
#define SSEL0_SET  (FIO2SET = PIN_SSEL0)
#define SSEL0_CLR  (FIO2CLR = PIN_SSEL0)

/* 1V8 regulator control */
#define CC1V8      (FIO1PIN & PIN_CC1V8)
#define CC1V8_SET  (FIO1SET = PIN_CC1V8)
#define CC1V8_CLR  (FIO1CLR = PIN_CC1V8)

/* CC2400 control */
#define CC3V3_SET  (FIO1SET = PIN_CC3V3)
#define CC3V3_CLR  (FIO1CLR = PIN_CC3V3)
#define RX_SET     (FIO1SET = PIN_RX)
#define RX_CLR     (FIO1CLR = PIN_RX)
#define TX_SET     (FIO1SET = PIN_TX)
#define TX_CLR     (FIO1CLR = PIN_TX)
#define CSN_SET    (FIO1SET = PIN_CSN)
#define CSN_CLR    (FIO1CLR = PIN_CSN)
#define SCLK_SET   (FIO1SET = PIN_SCLK)
#define SCLK_CLR   (FIO1CLR = PIN_SCLK)
#define MOSI_SET   (FIO1SET = PIN_MOSI)
#define MOSI_CLR   (FIO1CLR = PIN_MOSI)
#define GIO6_SET   (FIO1SET = PIN_GIO6)
#define GIO6_CLR   (FIO1CLR = PIN_GIO6)
#define BTGR_SET   (FIO1SET = PIN_BTGR)
#define BTGR_CLR   (FIO1CLR = PIN_BTGR)
#define MISO       (FIO1PIN & PIN_MISO)

/*
 * clock configuration
 *
 * main oscillator:  16 MHz
 * CPU clock (PLL0): 100 MHz
 * USB clock (PLL1): 48 MHz
 */
#define CCLKSEL 3
#define MSEL0 24
#define NSEL0 1
#define MSEL1 34
#define PSEL1 0

/* flash accelerator configuration */
#define FLASHTIM 0x4 /* up to 100 MHz CPU clock */

void wait(u8 seconds);
void gpio_init();
void ubertooth_init();
void ssp0_init();
void atest_init();
void cc2400_init();
u32 cc2400_spi(u8 len, u32 data);
u16 cc2400_get(u8 reg);
void cc2400_set(u8 reg, u32 val);
u8 cc2400_status();
u8 cc2400_strobe(u8 reg);
void cc2400_reset();
void clock_start();

#endif /* __UBERTOOTH_H */
