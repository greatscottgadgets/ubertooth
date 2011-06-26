/*
 * cc2400.h
 *
 * registers etc. for the Texas Instruments CC2400 wireless transceiver
 *
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

#ifndef __CC2400_H
#define __CC2400_H

#include <stdint.h>

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

/* status byte */
#define XOSC16M_STABLE       (1 << 6)
#define CS_ABOVE_THRESHOLD_N (1 << 5)
#define SYNC_RECEIVED        (1 << 4)
#define STATUS_CRC_OK        (1 << 3)
#define FS_LOCK              (1 << 2)
#define FH_EVENT             (1 << 0)

/* GIO signals */
#define GIO_PA_EN             3  /* Active high PA enable signal */
#define GIO_PA_EN_N           4  /* Active low PA enable signal */
#define GIO_SYNC_RECEIVED     5  /* Set if a valid sync word has been received */
#define GIO_PKT               6  /* Packet status signal See Figure 10, page 28. */
#define GIO_CARRIER_SENSE_N   10 /* Carrier sense output (RSSI above threshold) */
#define GIO_CRC_OK            11 /* CRC check OK after last byte read from FIFO */
#define GIO_AGC_EN            12 /* AGC enable signal */
#define GIO_FS_PD             13 /* Frequency synthesiser power down */
#define GIO_RX_PD             14 /* RX power down */
#define GIO_TX_PD             15 /* TX power down */
#define GIO_PKT_ACTIVE        22 /* Packet reception active */
#define GIO_MDM_TX_DIN        23 /* The TX data sent to modem */
#define GIO_MDM_TX_DCLK       24 /* The TX clock used by modem */
#define GIO_MDM_RX_DOUT       25 /* The RX data received by modem */
#define GIO_MDM_RX_DCLK       26 /* The RX clock recovered by modem */
#define GIO_MDM_RX_BIT_RAW    27 /* The un-synchronized RX data received by modem */
#define GIO_MDM_BACKEND_EN    29 /* The Backend enable signal used by modem in RX */
#define GIO_MDM_DEC_OVRFLW    30 /* Modem decimation overflow */
#define GIO_AGC_CHANGE        31 /* Signal that toggles whenever AGC changes gain. */
#define GIO_VGA_RESET_N       32 /* The VGA peak detectors' reset signal */
#define GIO_CAL_RUNNING       33 /* VCO calibration in progress */
#define GIO_SETTLING_RUNNING  34 /* Stepping CHP current after calibration */
#define GIO_RXBPF_CAL_RUNNING 35 /* RX band-pass filter calibration running */
#define GIO_VCO_CAL_START     36 /* VCO calibration start signal */
#define GIO_RXBPF_CAL_START   37 /* RX band-pass filter start signal */
#define GIO_FIFO_EMPTY        38 /* FIFO empty signal */
#define GIO_FIFO_FULL         39 /* FIFO full signal */
#define GIO_CLKEN_FS_DIG      40 /* Clock enable Frequency Synthesiser */
#define GIO_CLKEN_RXBPF_CAL   41 /* Clock enable RX band-pass filter calibration */
#define GIO_CLKEN_GR          42 /* Clock enable generic radio */
#define GIO_XOSC16M_STABLE    43 /* Indicates that the Main crystal oscillator is stable */
#define GIO_XOSC_16M_EN       44 /* 16 MHz XOSC enable signal */
#define GIO_XOSC_16M          45 /* 16 MHz XOSC output from analog part */
#define GIO_CLK_16M           46 /* 16 MHz clock from main clock tree */
#define GIO_CLK_16M_MOD       47 /* 16 MHz modulator clock tree */
#define GIO_CLK_8M16M_FSDIG   48 /* 8/16 MHz clock tree for fs_dig module */
#define GIO_CLK_8M            49 /* 8 MHz clock tree derived from XOSC_16M */
#define GIO_CLK_8M_DEMOD_AGC  50 /* 8 MHz clock tree for demodulator/AGC */
#define GIO_FREF              53 /* Reference clock (4 MHz) */
#define GIO_FPLL              54 /* Output clock of A/M-counter (4 MHz) */
#define GIO_PD_F_COMP         55 /* Phase detector comparator output */
#define GIO_WINDOW            56 /* Window signal to PD (Phase Detector) */
#define GIO_LOCK_INSTANT      57 /* Window signal latched in PD by the FREF clock */
#define GIO_RESET_N_SYSTEM    58 /* Chip wide reset (except registers) */
#define GIO_FIFO_FLUSH        59 /* FIFO flush signal */
#define GIO_LOCK_STATUS       60 /* The top-level FS in lock status signal */
#define GIO_ZERO              61 /* Output logic zero */
#define GIO_ONE               62 /* Output logic one */
#define GIO_HIGH_Z            63 /* Pin set as high-impedance output */

/* radio control states */
#define STATE_OFF             0
#define STATE_IDLE            1
#define STATE_PIN_RXTX_CAL    8
#define STATE_PIN_FS_ON       9
#define STATE_PIN_RX          10
#define STATE_PIN_RX_OFF      11
#define STATE_PIN_TX          12
#define STATE_PIN_TX_OFF      13
#define STATE_STROBE_RXTX_CAL 14
#define STATE_STROBE_FS_ON    15
#define STATE_STROBE_RX       16
#define STATE_STROBE_TX       17
#define STATE_STROBE_TX_OFF   18
#define STATE_BEFORE_IDLE     24

/* tuning limits */
#define MIN_FREQ 2268
#define MAX_FREQ 2794

#endif /* __CC2400_H */
