/*
 * adf7242.h
 *
 * registers etc. for the Analog Devices ADF7242 wireless transceiver
 *
 * Copyright 2014 Mike Ryan
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

#ifndef __ADF7242_H
#define __ADF7242_H

#ifdef ARTICHOKE

#include <stdint.h>

#if 0
/* CC2400 registers */
#define MAIN      0x00 /* Main control register */
#define FSCTRL    0x01 /* Frequency synthesiser main control and status */
#define FSDIV     0x02 /* Frequency synthesiser frequency division control */
#define MDMCTRL   0x03 /* Modem main control and status */
#define AGCCTRL   0x04 /* AGC main control and status */
#define FREND     0x05 /* Analog front-end control */
#define RSSI      0x06 /* RSSI information */
#define FREQEST   0x07 /* Received signal frequency offset estimation */
#define IOCFG     0x08 /* I/O configuration register */
#define FSMTC     0x0B /* Finite state machine time constants */
#define MANAND    0x0D /* Manual signal AND-override register */
#define FSMSTATE  0x0E /* Finite state machine information and breakpoint */
#define ADCTST    0x0F /* ADC test register */
#define RXBPFTST  0x10 /* Receiver bandpass filters test register */
#define PAMTST    0x11 /* PA and transmit mixers test register */
#define LMTST     0x12 /* LNA and receive mixers test register */
#define MANOR     0x13 /* Manual signal OR-override register */
#define MDMTST0   0x14 /* Modem test register 0 */
#define MDMTST1   0x15 /* Modem test register 1 */
#define DACTST    0x16 /* DAC test register */
#define AGCTST0   0x17 /* AGC test register: various control and status */
#define AGCTST1   0x18 /* AGC test register: AGC timeout */
#define AGCTST2   0x19 /* AGC test register: AGC various parameters */
#define FSTST0    0x1A /* Test register: VCO array results and override */
#define FSTST1    0x1B /* Test register: VC DAC manual control VCO */
#define FSTST2    0x1C /* Test register: VCO current result and override */
#define FSTST3    0x1D /* Test register: Charge pump current etc */
#define MANFIDL   0x1E /* Manufacturer ID, lower 16 bit */
#define MANFIDH   0x1F /* Manufacturer ID, upper 16 bit */
#define GRMDM     0x20 /* Generic radio modem control */
#define GRDEC     0x21 /* Generic radio decimation control and status */
#define PKTSTATUS 0x22 /* Packet mode status */
#define INT       0x23 /* Interrupt register */
#define SYNCL     0x2C /* Synchronisation word, lower 16 bit */
#define SYNCH     0x2D /* Synchronisation word, upper 16 bit */
#define SXOSCON   0x60 /* Command strobe register: Turn on XOSC */
#define SFSON     0x61 /* Command strobe register: Start and calibrate FS */
#define SRX       0x62 /* Command strobe register: Start RX */
#define STX       0x63 /* Command strobe register: Start TX (turn on PA) */
#define SRFOFF    0x64 /* Command strobe register: Turn off RX/TX and FS */
#define SXOSCOFF  0x65 /* Command strobe register: Turn off XOSC */
#define FIFOREG   0x70 /* Write and read data to and from the 32 byte FIFO */
#endif

/* status byte */
#define SPI_READY           (1 << 7)
#define IRQ_STATUS          (1 << 6)
#define RC_READY            (1 << 5)
#define CCA_RESULT          (1 << 4)
#define RC_STATUS           (0xf)

/* SPI commands */
#define SPI_NOP             0xFF /* TODO comments */
#define SPI_PKT_WR          0x10 /* on each */
#define SPI_PKT_RD          0x30 /* of these */
#define SPI_MEM_WR          0x18
#define SPI_MEM_RD          0x38
#define SPI_MEMR_WR         0x08 /* ... */
#define SPI_MEMR_RD         0x28
#define SPI_PRAM_WR         0x1E
#define RC_SLEEP            0x81
#define RC_IDLE             0xB2
#define RC_PHY_RDY          0xB3
#define RC_RX               0xB4
#define RC_TX               0xB5
#define RC_MEAS             0xB6
#define RC_CCA              0xB7
#define RC_PC_RESET         0xC7
#define RC_RESET            0xC8 /* TODO comments */

/* radio control states */
#define RC_ST_IDLE           1
#define RC_ST_MEAS           2
#define RC_ST_PHY_RDY        3
#define RC_ST_RX             4
#define RC_ST_TX             5

#if 0
/* tuning limits */
#define MIN_FREQ 2268
#define MAX_FREQ 2794
#endif

#endif /* ARTICHOKE */

#endif /* __ADF7242_H */
