/*
 * lpc17.h
 *
 * registers and bit definitions for NXP LPC17xx microcontrollers
 *
 * based on information contained in:
 *
 * UM10360
 * LPC17xx User manual
 * Rev. 2 -- 19 August 2010
 *
 * Copyright 2010, 2011 Michael Ossmann
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

#ifndef __LPC17_H
#define __LPC17_H

#include <stdint.h>
 
#define LPC17_REG(a)   (*(volatile uint32_t *)(a))
#define LPC17_REG8(a)  (*(volatile uint8_t *)(a))
#define LPC17_REG16(a) (*(volatile uint16_t *)(a))


/* system control registers */

/* External Interrupts */
#define EXTINT   LPC17_REG(0x400FC140) /* External Interrupt Flag Register */
#define EXTMODE  LPC17_REG(0x400FC148) /* External Interrupt Mode register */
#define EXTPOLAR LPC17_REG(0x400FC14C) /* External Interrupt Polarity Register */

/* Reset */
#define RSID LPC17_REG(0x400FC180) /* Reset Source Identification Register */

/* Syscon Miscellaneous Registers */
#define SCS LPC17_REG(0x400FC1A0) /* System Control and Status */

/* Reset Source Identification Register (RSID - 0x400F C180) */
#define RSID_POR  (0x1 << 0)
#define RSID_EXTR (0x1 << 1)
#define RSID_WDTR (0x1 << 2)
#define RSID_BODR (0x1 << 3)

/* External Interrupt flag register (EXTINT - 0x400F C140) */
#define EXTINT_EINT0 (0x1 << 0)
#define EXTINT_EINT1 (0x1 << 1)
#define EXTINT_EINT2 (0x1 << 2)
#define EXTINT_EINT3 (0x1 << 3)

/* External Interrupt Mode register (EXTMODE - 0x400F C148) */
#define EXTMODE_EXTMODE0 (0x1 << 0)
#define EXTMODE_EXTMODE1 (0x1 << 1)
#define EXTMODE_EXTMODE2 (0x1 << 2)
#define EXTMODE_EXTMODE3 (0x1 << 3)

/* External Interrupt Polarity register (EXTPOLAR - 0x400F C14C) */
#define EXTPOLAR_EXTPOLAR0 (0x1 << 0)
#define EXTPOLAR_EXTPOLAR1 (0x1 << 1)
#define EXTPOLAR_EXTPOLAR2 (0x1 << 2)
#define EXTPOLAR_EXTPOLAR3 (0x1 << 3)

/*  System Controls and Status register (SCS - 0x400F C1A0) */
#define SCS_OSCRANGE (0x1 << 4)
#define SCS_OSCEN    (0x1 << 5)
#define SCS_OSCSTAT  (0x1 << 6)


/* clocking and power control registers */

/* Clock source selection */
#define CLKSRCSEL LPC17_REG(0x400FC10C) /* Clock Source Select Register */

/* Phase Locked Loop (PLL0, Main PLL) */
#define PLL0CON  LPC17_REG(0x400FC080) /* PLL0 Control Register */
#define PLL0CFG  LPC17_REG(0x400FC084) /* PLL0 Configuration Register */
#define PLL0STAT LPC17_REG(0x400FC088) /* PLL0 Status Register */
#define PLL0FEED LPC17_REG(0x400FC08C) /* PLL0 Feed Register */

/* Phase Locked Loop (PLL1, USB PLL) */
#define PLL1CON  LPC17_REG(0x400FC0A0) /* PLL1 Control Register */
#define PLL1CFG  LPC17_REG(0x400FC0A4) /* PLL1 Configuration Register */
#define PLL1STAT LPC17_REG(0x400FC0A8) /* PLL1 Status Register */
#define PLL1FEED LPC17_REG(0x400FC0AC) /* PLL1 Feed Register */

/* Clock dividers */
#define CCLKCFG   LPC17_REG(0x400FC104) /* CPU Clock Configuration Register */
#define USBCLKCFG LPC17_REG(0x400FC108) /* USB Clock Configuration Register */
#define PCLKSEL0  LPC17_REG(0x400FC1A8) /* Peripheral Clock Selection register 0 */
#define PCLKSEL1  LPC17_REG(0x400FC1AC) /* Peripheral Clock Selection register 1 */

/* Power control */
#define PCON  LPC17_REG(0x400FC0C0) /* Power Control Register */
#define PCONP LPC17_REG(0x400FC0C4) /* Power Control for Peripherals Register */

/* Utility */
#define CLKOUTCFG LPC17_REG(0x400FC1C8) /* Clock Output Configuration Register */

/* Clock Source Select register (CLKSRCSEL - 0x400F C10C) */
#define CLKSRCSEL_CLKSRC (0x3 << 4)

/* PLL0 Control register (PLL0CON - 0x400F C080) */
#define PLL0CON_PLLE0 (0x1 << 0)
#define PLL0CON_PLLC0 (0x1 << 1)

/* PLL0 Configuration register (PLL0CFG - 0x400F C084) */
#define PLL0CFG_MSEL0 (0x7FFF << 0)
#define PLL0CFG_NSEL0 (0xFF << 16)

/* PLL0 Status register (PLL0STAT - 0x400F C088) */
#define PLL0STAT_MSEL0      (0x7FFF << 0)
#define PLL0STAT_NSEL0      (0xFF << 16)
#define PLL0STAT_PLLE0_STAT (0x1 << 24)
#define PLL0STAT_PLLC0_STAT (0x1 << 25)
#define PLL0STAT_PLOCK0     (0x1 << 26)

/* PLL0 Feed register (PLL0FEED - 0x400F C08C) */
#define PLL0FEED_SEQUENCE PLL0FEED = 0xAA; PLL0FEED = 0x55

/* PLL1 Control register (PLL1CON - 0x400F C0A0)  */
#define PLL1CON_PLLE1 (0x1 << 0)
#define PLL1CON_PLLC1 (0x1 << 1)

/* PLL1 Configuration register (PLL1CFG - 0x400F C0A4) */
#define PLL1CFG_MSEL1 (0x1F << 0)
#define PLL1CFG_PSEL1 (0x3 << 5)

/* PLL1 Status register (PLL1STAT - 0x400F C0A8) */
#define PLL1STAT_MSEL1      (0x1F << 0)
#define PLL1STAT_PSEL1      (0x3 << 5)
#define PLL1STAT_PLLE1_STAT (0x1 << 8)
#define PLL1STAT_PLLC1_STAT (0x1 << 9)
#define PLL1STAT_PLOCK1     (0x1 << 10)

/* PLL1 Feed register (PLL1FEED - 0x400F C0AC) */
#define PLL1FEED_SEQUENCE PLL1FEED = 0xAA; PLL1FEED = 0x55

/* CPU Clock Configuration register (CCLKCFG - 0x400F C104) */
#define CCLKCFG_CCLKSEL (0xFF << 0)

/* USB Clock Configuration register (USBCLKCFG - 0x400F C108) */
#define USBCLKCFG_USBSEL (0xF << 0)

/* Peripheral Clock Selection register 0 (PCLKSEL0 - 0x400F C1A8) */
#define PCLKSEL0_PCLK_WDT    (0x3 << 0)
#define PCLKSEL0_PCLK_TIMER0 (0x3 << 2)
#define PCLKSEL0_PCLK_TIMER1 (0x3 << 4)
#define PCLKSEL0_PCLK_UART0  (0x3 << 6)
#define PCLKSEL0_PCLK_UART1  (0x3 << 8)
#define PCLKSEL0_PCLK_PWM1   (0x3 << 12)
#define PCLKSEL0_PCLK_I2C0   (0x3 << 14)
#define PCLKSEL0_PCLK_SPI    (0x3 << 16)
#define PCLKSEL0_PCLK_SSP1   (0x3 << 20)
#define PCLKSEL0_PCLK_DAC    (0x3 << 22)
#define PCLKSEL0_PCLK_ADC    (0x3 << 24)
#define PCLKSEL0_PCLK_CAN1   (0x3 << 26)
#define PCLKSEL0_PCLK_CAN2   (0x3 << 28)
#define PCLKSEL0_PCLK_ACF    (0x3 << 30)

/* Peripheral Clock Selection register 1 (PCLKSEL1 - 0x400F C1AC) */
#define PCLKSEL1_PCLK_QEI     (0x3 << 0)
#define PCLKSEL1_PCLK_GPIOINT (0x3 << 2)
#define PCLKSEL1_PCLK_PCB     (0x3 << 4)
#define PCLKSEL1_PCLK_I2C1    (0x3 << 6)
#define PCLKSEL1_PCLK_SSP0    (0x3 << 10)
#define PCLKSEL1_PCLK_TIMER2  (0x3 << 12)
#define PCLKSEL1_PCLK_TIMER3  (0x3 << 14)
#define PCLKSEL1_PCLK_UART2   (0x3 << 16)
#define PCLKSEL1_PCLK_UART3   (0x3 << 18)
#define PCLKSEL1_PCLK_I2C2    (0x3 << 20)
#define PCLKSEL1_PCLK_I2S     (0x3 << 22)
#define PCLKSEL1_PCLK_RIT     (0x3 << 26)
#define PCLKSEL1_PCLK_SYSCON  (0x3 << 28)
#define PCLKSEL1_PCLK_MC      (0x3 << 30)

/* Power Mode Control register (PCON - 0x400F C0C0) */
#define PCON_PM0     (0x1 << 0)
#define PCON_PM1     (0x1 << 1)
#define PCON_BODRPM  (0x1 << 2)
#define PCON_BOGD    (0x1 << 3)
#define PCON_BORD    (0x1 << 4)
#define PCON_SMFLAG  (0x1 << 8)
#define PCON_DSFLAG  (0x1 << 9)
#define PCON_PDFLAG  (0x1 << 10)
#define PCON_DPDFLAG (0x1 << 11)

/* Power Control for Peripherals register (PCONP - 0x400F C0C4) */
#define PCONP_PCTIM0  (0x1 << 1)
#define PCONP_PCTIM1  (0x1 << 2)
#define PCONP_PCUART0 (0x1 << 3)
#define PCONP_PCUART1 (0x1 << 4)
#define PCONP_PCPWM1  (0x1 << 6)
#define PCONP_PCI2C0  (0x1 << 7)
#define PCONP_PCSPI   (0x1 << 8)
#define PCONP_PCRTC   (0x1 << 9)
#define PCONP_PCSSP1  (0x1 << 10)
#define PCONP_PCADC   (0x1 << 12)
#define PCONP_PCCAN1  (0x1 << 13)
#define PCONP_PCCAN2  (0x1 << 14)
#define PCONP_PCGPIO  (0x1 << 15)
#define PCONP_PCRIT   (0x1 << 16)
#define PCONP_PCMCPWM (0x1 << 17)
#define PCONP_PCQEI   (0x1 << 18)
#define PCONP_PCI2C1  (0x1 << 19)
#define PCONP_PCSSP0  (0x1 << 21)
#define PCONP_PCTIM2  (0x1 << 22)
#define PCONP_PCTIM3  (0x1 << 23)
#define PCONP_PCUART2 (0x1 << 24)
#define PCONP_PCUART3 (0x1 << 25)
#define PCONP_PCI2C2  (0x1 << 26)
#define PCONP_PCI2S   (0x1 << 27)
#define PCONP_PCGPDMA (0x1 << 29)
#define PCONP_PCENET  (0x1 << 30)
#define PCONP_PCUSB   (1U << 31)

/* Clock Output Configuration register (CLKOUTCFG - 0x400F C1C8) */
#define CLKOUTCFG_CLKOUTSEL  (0xF << 0)
#define CLKOUTCFG_CLKOUTDIV  (0xF << 4)
#define CLKOUTCFG_CLKOUT_EN  (0x1 << 8)
#define CLKOUTCFG_CLKOUT_ACT (0x1 << 9)


/* flash accelerator registers */
#define FLASHCFG LPC17_REG(0x400FC000) /* Flash Accelerator Configuration Register */

/* Flash Accelerator Configuration register (FLASHCFG - 0x400F C000) */
#define FLASHCFG_FLASHTIM (0xF << 12)


/* NVIC registers */

#define ISER0 LPC17_REG(0xE000E100) /* Interrupt Set-Enable Register 0 */
#define ISER1 LPC17_REG(0xE000E104) /* Interrupt Set-Enable Register 1 */
#define ICER0 LPC17_REG(0xE000E180) /* Interrupt Clear-Enable Register 0 */
#define ICER1 LPC17_REG(0xE000E184) /* Interrupt Clear-Enable Register 1 */
#define ISPR0 LPC17_REG(0xE000E200) /* Interrupt Set-Pending Register 0 */
#define ISPR1 LPC17_REG(0xE000E204) /* Interrupt Set-Pending Register 1 */
#define ICPR0 LPC17_REG(0xE000E280) /* Interrupt Clear-Pending Register 0 */
#define ICPR1 LPC17_REG(0xE000E284) /* Interrupt Clear-Pending Register 1 */
#define IABR0 LPC17_REG(0xE000E300) /* Interrupt Active Bit Register 0 */
#define IABR1 LPC17_REG(0xE000E304) /* Interrupt Active Bit Register 1 */
#define IPR0  LPC17_REG(0xE000E400) /* Interrupt Priority Register 0 */
#define IPR1  LPC17_REG(0xE000E404) /* Interrupt Priority Register 1 */
#define IPR2  LPC17_REG(0xE000E408) /* Interrupt Priority Register 2 */
#define IPR3  LPC17_REG(0xE000E40C) /* Interrupt Priority Register 3 */
#define IPR4  LPC17_REG(0xE000E410) /* Interrupt Priority Register 4 */
#define IPR5  LPC17_REG(0xE000E414) /* Interrupt Priority Register 5 */
#define IPR6  LPC17_REG(0xE000E418) /* Interrupt Priority Register 6 */
#define IPR7  LPC17_REG(0xE000E41C) /* Interrupt Priority Register 7 */
#define IPR8  LPC17_REG(0xE000E420) /* Interrupt Priority Register 8 */
#define STIR  LPC17_REG(0xE000EF00) /* Software Trigger Interrupt Register */

/* Interrupt Set-Enable Register 0 register (ISER0 - 0xE000 E100) */
#define ISER0_ISE_WDT    (0x1 << 0) /* Watchdog Timer Interrupt Enable */
#define ISER0_ISE_TIMER0 (0x1 << 1) /* Timer 0 Interrupt Enable */
#define ISER0_ISE_TIMER1 (0x1 << 2) /* Timer 1. Interrupt Enable */
#define ISER0_ISE_TIMER2 (0x1 << 3) /* Timer 2 Interrupt Enable */
#define ISER0_ISE_TIMER3 (0x1 << 4) /* Timer 3 Interrupt Enable */
#define ISER0_ISE_UART0  (0x1 << 5) /* UART0 Interrupt Enable */
#define ISER0_ISE_UART1  (0x1 << 6) /* UART1 Interrupt Enable */
#define ISER0_ISE_UART2  (0x1 << 7) /* UART2 Interrupt Enable */
#define ISER0_ISE_UART3  (0x1 << 8) /* UART3 Interrupt Enable */
#define ISER0_ISE_PWM    (0x1 << 9) /* PWM1 Interrupt Enable */
#define ISER0_ISE_I2C0   (0x1 << 10) /* I2C0 Interrupt Enable */
#define ISER0_ISE_I2C1   (0x1 << 11) /* I2C1 Interrupt Enable */
#define ISER0_ISE_I2C2   (0x1 << 12) /* I2C2 Interrupt Enable */
#define ISER0_ISE_SPI    (0x1 << 13) /* SPI Interrupt Enable */
#define ISER0_ISE_SSP0   (0x1 << 14) /* SSP0 Interrupt Enable */
#define ISER0_ISE_SSP1   (0x1 << 15) /* SSP1 Interrupt Enable */
#define ISER0_ISE_PLL0   (0x1 << 16) /* PLL0 (Main PLL) Interrupt Enable */
#define ISER0_ISE_RTC    (0x1 << 17) /* Real Time Clock (RTC) Interrupt Enable */
#define ISER0_ISE_EINT0  (0x1 << 18) /* External Interrupt 0 Interrupt Enable */
#define ISER0_ISE_EINT1  (0x1 << 19) /* External Interrupt 1 Interrupt Enable */
#define ISER0_ISE_EINT2  (0x1 << 20) /* External Interrupt 2 Interrupt Enable */
#define ISER0_ISE_EINT3  (0x1 << 21) /* External Interrupt 3 Interrupt Enable */
#define ISER0_ISE_ADC    (0x1 << 22) /* ADC Interrupt Enable */
#define ISER0_ISE_BOD    (0x1 << 23) /* BOD Interrupt Enable */
#define ISER0_ISE_USB    (0x1 << 24) /* USB Interrupt Enable */
#define ISER0_ISE_CAN    (0x1 << 25) /* CAN Interrupt Enable */
#define ISER0_ISE_DMA    (0x1 << 26) /* GPDMA Interrupt Enable */
#define ISER0_ISE_I2S    (0x1 << 27) /* I2S Interrupt Enable */
#define ISER0_ISE_ENET   (0x1 << 28) /* Ethernet Interrupt Enable */
#define ISER0_ISE_RIT    (0x1 << 29) /* Repetitive Interrupt Timer Interrupt Enable */
#define ISER0_ISE_MCPWM  (0x1 << 30) /* Motor Control PWM Interrupt Enable */
#define ISER0_ISE_QEI    (0x1 << 31) /* Quadrature Encoder Interface Interrupt Enable */

/* Interrupt Set-Enable Register 1 register (ISER1 - 0xE000 E104) */
#define ISER1_ISE_PLL1   (0x1 << 0) /* PLL1 (USB PLL) Interrupt Enable */
#define ISER1_ISE_USBACT (0x1 << 1) /* USB Activity Interrupt Enable */
#define ISER1_ISE_CANACT (0x1 << 2) /* CAN Activity Interrupt Enable */

/* Interrupt Clear-Enable Register 0 (ICER0 - 0xE000 E180) */
#define ICER0_ICE_WDT    (0x1 << 0) /* Watchdog Timer Interrupt Disable */
#define ICER0_ICE_TIMER0 (0x1 << 1) /* Timer 0 Interrupt Disable */
#define ICER0_ICE_TIMER1 (0x1 << 2) /* Timer 1. Interrupt Disable */
#define ICER0_ICE_TIMER2 (0x1 << 3) /* Timer 2 Interrupt Disable */
#define ICER0_ICE_TIMER3 (0x1 << 4) /* Timer 3 Interrupt Disable */
#define ICER0_ICE_UART0  (0x1 << 5) /* UART0 Interrupt Disable */
#define ICER0_ICE_UART1  (0x1 << 6) /* UART1 Interrupt Disable */
#define ICER0_ICE_UART2  (0x1 << 7) /* UART2 Interrupt Disable */
#define ICER0_ICE_UART3  (0x1 << 8) /* UART3 Interrupt Disable */
#define ICER0_ICE_PWM    (0x1 << 9) /* PWM1 Interrupt Disable */
#define ICER0_ICE_I2C0   (0x1 << 10) /* I2C0 Interrupt Disable */
#define ICER0_ICE_I2C1   (0x1 << 11) /* I2C1 Interrupt Disable */
#define ICER0_ICE_I2C2   (0x1 << 12) /* I2C2 Interrupt Disable */
#define ICER0_ICE_SPI    (0x1 << 13) /* SPI Interrupt Disable */
#define ICER0_ICE_SSP0   (0x1 << 14) /* SSP0 Interrupt Disable */
#define ICER0_ICE_SSP1   (0x1 << 15) /* SSP1 Interrupt Disable */
#define ICER0_ICE_PLL0   (0x1 << 16) /* PLL0 (Main PLL) Interrupt Disable */
#define ICER0_ICE_RTC    (0x1 << 17) /* Real Time Clock (RTC) Interrupt Disable */
#define ICER0_ICE_EINT0  (0x1 << 18) /* External Interrupt 0 Interrupt Disable */
#define ICER0_ICE_EINT1  (0x1 << 19) /* External Interrupt 1 Interrupt Disable */
#define ICER0_ICE_EINT2  (0x1 << 20) /* External Interrupt 2 Interrupt Disable */
#define ICER0_ICE_EINT3  (0x1 << 21) /* External Interrupt 3 Interrupt Disable */
#define ICER0_ICE_ADC    (0x1 << 22) /* ADC Interrupt Disable */
#define ICER0_ICE_BOD    (0x1 << 23) /* BOD Interrupt Disable */
#define ICER0_ICE_USB    (0x1 << 24) /* USB Interrupt Disable */
#define ICER0_ICE_CAN    (0x1 << 25) /* CAN Interrupt Disable */
#define ICER0_ICE_DMA    (0x1 << 26) /* GPDMA Interrupt Disable */
#define ICER0_ICE_I2S    (0x1 << 27) /* I2S Interrupt Disable */
#define ICER0_ICE_ENET   (0x1 << 28) /* Ethernet Interrupt Disable */
#define ICER0_ICE_RIT    (0x1 << 29) /* Repetitive Interrupt Timer Interrupt Disable */
#define ICER0_ICE_MCPWM  (0x1 << 30) /* Motor Control PWM Interrupt Disable */
#define ICER0_ICE_QEI    (0x1 << 31) /* Quadrature Encoder Interface Interrupt Disable */

/* Interrupt Clear-Enable Register 1 register (ICER1 - 0xE000 E184) */
#define ICER1_ICE_PLL1   (0x1 << 0) /* PLL1 (USB PLL) Interrupt Disable */
#define ICER1_ICE_USBACT (0x1 << 1) /* USB Activity Interrupt Disable */
#define ICER1_ICE_CANACT (0x1 << 2) /* CAN Activity Interrupt Disable */

/* Interrupt Set-Pending Register 0 register (ISPR0 - 0xE000 E200) */
#define ISPR0_ISP_WDT    (0x1 << 0) /* Watchdog Timer Interrupt Pending set */
#define ISPR0_ISP_TIMER0 (0x1 << 1) /* Timer 0 Interrupt Pending set */
#define ISPR0_ISP_TIMER1 (0x1 << 2) /* Timer 1. Interrupt Pending set */
#define ISPR0_ISP_TIMER2 (0x1 << 3) /* Timer 2 Interrupt Pending set */
#define ISPR0_ISP_TIMER3 (0x1 << 4) /* Timer 3 Interrupt Pending set */
#define ISPR0_ISP_UART0  (0x1 << 5) /* UART0 Interrupt Pending set */
#define ISPR0_ISP_UART1  (0x1 << 6) /* UART1 Interrupt Pending set */
#define ISPR0_ISP_UART2  (0x1 << 7) /* UART2 Interrupt Pending set */
#define ISPR0_ISP_UART3  (0x1 << 8) /* UART3 Interrupt Pending set */
#define ISPR0_ISP_PWM    (0x1 << 9) /* PWM1 Interrupt Pending set */
#define ISPR0_ISP_I2C0   (0x1 << 10) /* I2C0 Interrupt Pending set */
#define ISPR0_ISP_I2C1   (0x1 << 11) /* I2C1 Interrupt Pending set */
#define ISPR0_ISP_I2C2   (0x1 << 12) /* I2C2 Interrupt Pending set */
#define ISPR0_ISP_SPI    (0x1 << 13) /* SPI Interrupt Pending set */
#define ISPR0_ISP_SSP0   (0x1 << 14) /* SSP0 Interrupt Pending set */
#define ISPR0_ISP_SSP1   (0x1 << 15) /* SSP1 Interrupt Pending set */
#define ISPR0_ISP_PLL0   (0x1 << 16) /* PLL0 (Main PLL) Interrupt Pending set */
#define ISPR0_ISP_RTC    (0x1 << 17) /* Real Time Clock (RTC) Interrupt Pending set */
#define ISPR0_ISP_EINT0  (0x1 << 18) /* External Interrupt 0 Interrupt Pending set */
#define ISPR0_ISP_EINT1  (0x1 << 19) /* External Interrupt 1 Interrupt Pending set */
#define ISPR0_ISP_EINT2  (0x1 << 20) /* External Interrupt 2 Interrupt Pending set */
#define ISPR0_ISP_EINT3  (0x1 << 21) /* External Interrupt 3 Interrupt Pending set */
#define ISPR0_ISP_ADC    (0x1 << 22) /* ADC Interrupt Pending set */
#define ISPR0_ISP_BOD    (0x1 << 23) /* BOD Interrupt Pending set */
#define ISPR0_ISP_USB    (0x1 << 24) /* USB Interrupt Pending set */
#define ISPR0_ISP_CAN    (0x1 << 25) /* CAN Interrupt Pending set */
#define ISPR0_ISP_DMA    (0x1 << 26) /* GPDMA Interrupt Pending set */
#define ISPR0_ISP_I2S    (0x1 << 27) /* I2S Interrupt Pending set */
#define ISPR0_ISP_ENET   (0x1 << 28) /* Ethernet Interrupt Pending set */
#define ISPR0_ISP_RIT    (0x1 << 29) /* Repetitive Interrupt Timer Interrupt Pending set */
#define ISPR0_ISP_MCPWM  (0x1 << 30) /* Motor Control PWM Interrupt Pending set */
#define ISPR0_ISP_QEI    (0x1 << 31) /* Quadrature Encoder Interface Interrupt Pending set */

/* Interrupt Set-Pending Register 1 register (ISPR1 - 0xE000 E204) */
#define ISPR1_ISP_PLL1   (0x1 << 0) /* PLL1 (USB PLL) Interrupt Pending set */
#define ISPR1_ISP_USBACT (0x1 << 1) /* USB Activity Interrupt Pending set */
#define ISPR1_ISP_CANACT (0x1 << 2) /* CAN Activity Interrupt Pending set */

/* Interrupt Clear-Pending Register 0 register (ICPR0 - 0xE000 E280) */
#define ICPR0_ICP_WDT    (0x1 << 0) /* Watchdog Timer Interrupt Pending clear */
#define ICPR0_ICP_TIMER0 (0x1 << 1) /* Timer 0 Interrupt Pending clear */
#define ICPR0_ICP_TIMER1 (0x1 << 2) /* Timer 1. Interrupt Pending clear */
#define ICPR0_ICP_TIMER2 (0x1 << 3) /* Timer 2 Interrupt Pending clear */
#define ICPR0_ICP_TIMER3 (0x1 << 4) /* Timer 3 Interrupt Pending clear */
#define ICPR0_ICP_UART0  (0x1 << 5) /* UART0 Interrupt Pending clear */
#define ICPR0_ICP_UART1  (0x1 << 6) /* UART1 Interrupt Pending clear */
#define ICPR0_ICP_UART2  (0x1 << 7) /* UART2 Interrupt Pending clear */
#define ICPR0_ICP_UART3  (0x1 << 8) /* UART3 Interrupt Pending clear */
#define ICPR0_ICP_PWM    (0x1 << 9) /* PWM1 Interrupt Pending clear */
#define ICPR0_ICP_I2C0   (0x1 << 10) /* I2C0 Interrupt Pending clear */
#define ICPR0_ICP_I2C1   (0x1 << 11) /* I2C1 Interrupt Pending clear */
#define ICPR0_ICP_I2C2   (0x1 << 12) /* I2C2 Interrupt Pending clear */
#define ICPR0_ICP_SPI    (0x1 << 13) /* SPI Interrupt Pending clear */
#define ICPR0_ICP_SSP0   (0x1 << 14) /* SSP0 Interrupt Pending clear */
#define ICPR0_ICP_SSP1   (0x1 << 15) /* SSP1 Interrupt Pending clear */
#define ICPR0_ICP_PLL0   (0x1 << 16) /* PLL0 (Main PLL) Interrupt Pending clear */
#define ICPR0_ICP_RTC    (0x1 << 17) /* Real Time Clock (RTC) Interrupt Pending clear */
#define ICPR0_ICP_EINT0  (0x1 << 18) /* External Interrupt 0 Interrupt Pending clear */
#define ICPR0_ICP_EINT1  (0x1 << 19) /* External Interrupt 1 Interrupt Pending clear */
#define ICPR0_ICP_EINT2  (0x1 << 20) /* External Interrupt 2 Interrupt Pending clear */
#define ICPR0_ICP_EINT3  (0x1 << 21) /* External Interrupt 3 Interrupt Pending clear */
#define ICPR0_ICP_ADC    (0x1 << 22) /* ADC Interrupt Pending clear */
#define ICPR0_ICP_BOD    (0x1 << 23) /* BOD Interrupt Pending clear */
#define ICPR0_ICP_USB    (0x1 << 24) /* USB Interrupt Pending clear */
#define ICPR0_ICP_CAN    (0x1 << 25) /* CAN Interrupt Pending clear */
#define ICPR0_ICP_DMA    (0x1 << 26) /* GPDMA Interrupt Pending clear */
#define ICPR0_ICP_I2S    (0x1 << 27) /* I2S Interrupt Pending clear */
#define ICPR0_ICP_ENET   (0x1 << 28) /* Ethernet Interrupt Pending clear */
#define ICPR0_ICP_RIT    (0x1 << 29) /* Repetitive Interrupt Timer Interrupt Pending clear */
#define ICPR0_ICP_MCPWM  (0x1 << 30) /* Motor Control PWM Interrupt Pending clear */
#define ICPR0_ICP_QEI    (0x1 << 31) /* Quadrature Encoder Interface Interrupt Pending clear */

/* Interrupt Clear-Pending Register 1 register (ICPR1 - 0xE000 E284) */
#define ICPR1_ICP_PLL1   (0x1 << 0) /* PLL1 (USB PLL) Interrupt Pending clear */
#define ICPR1_ICP_USBACT (0x1 << 1) /* USB Activity Interrupt Pending clear */
#define ICPR1_ICP_CANACT (0x1 << 2) /* CAN Activity Interrupt Pending clear */

/* Interrupt Active Bit Register 0 (IABR0 - 0xE000 E300) */
#define IABR0_IAB_WDT    (0x1 << 0) /* Watchdog Timer Interrupt Active */
#define IABR0_IAB_TIMER0 (0x1 << 1) /* Timer 0 Interrupt Active */
#define IABR0_IAB_TIMER1 (0x1 << 2) /* Timer 1. Interrupt Active */
#define IABR0_IAB_TIMER2 (0x1 << 3) /* Timer 2 Interrupt Active */
#define IABR0_IAB_TIMER3 (0x1 << 4) /* Timer 3 Interrupt Active */
#define IABR0_IAB_UART0  (0x1 << 5) /* UART0 Interrupt Active */
#define IABR0_IAB_UART1  (0x1 << 6) /* UART1 Interrupt Active */
#define IABR0_IAB_UART2  (0x1 << 7) /* UART2 Interrupt Active */
#define IABR0_IAB_UART3  (0x1 << 8) /* UART3 Interrupt Active */
#define IABR0_IAB_PWM    (0x1 << 9) /* PWM1 Interrupt Active */
#define IABR0_IAB_I2C0   (0x1 << 10) /* I2C0 Interrupt Active */
#define IABR0_IAB_I2C1   (0x1 << 11) /* I2C1 Interrupt Active */
#define IABR0_IAB_I2C2   (0x1 << 12) /* I2C2 Interrupt Active */
#define IABR0_IAB_SPI    (0x1 << 13) /* SPI Interrupt Active */
#define IABR0_IAB_SSP0   (0x1 << 14) /* SSP0 Interrupt Active */
#define IABR0_IAB_SSP1   (0x1 << 15) /* SSP1 Interrupt Active */
#define IABR0_IAB_PLL0   (0x1 << 16) /* PLL0 (Main PLL) Interrupt Active */
#define IABR0_IAB_RTC    (0x1 << 17) /* Real Time Clock (RTC) Interrupt Active */
#define IABR0_IAB_EINT0  (0x1 << 18) /* External Interrupt 0 Interrupt Active */
#define IABR0_IAB_EINT1  (0x1 << 19) /* External Interrupt 1 Interrupt Active */
#define IABR0_IAB_EINT2  (0x1 << 20) /* External Interrupt 2 Interrupt Active */
#define IABR0_IAB_EINT3  (0x1 << 21) /* External Interrupt 3 Interrupt Active */
#define IABR0_IAB_ADC    (0x1 << 22) /* ADC Interrupt Active */
#define IABR0_IAB_BOD    (0x1 << 23) /* BOD Interrupt Active */
#define IABR0_IAB_USB    (0x1 << 24) /* USB Interrupt Active */
#define IABR0_IAB_CAN    (0x1 << 25) /* CAN Interrupt Active */
#define IABR0_IAB_DMA    (0x1 << 26) /* GPDMA Interrupt Active */
#define IABR0_IAB_I2S    (0x1 << 27) /* I2S Interrupt Active */
#define IABR0_IAB_ENET   (0x1 << 28) /* Ethernet Interrupt Active */
#define IABR0_IAB_RIT    (0x1 << 29) /* Repetitive Interrupt Timer Interrupt Active */
#define IABR0_IAB_MCPWM  (0x1 << 30) /* Motor Control PWM Interrupt Active */
#define IABR0_IAB_QEI    (0x1 << 31) /* Quadrature Encoder Interface Interrupt Active */

/* Interrupt Active Bit Register 1 (IABR1 - 0xE000 E304) */
#define IABR1_IAB_PLL1   (0x1 << 0) /* PLL1 (USB PLL) Interrupt Active */
#define IABR1_IAB_USBACT (0x1 << 1) /* USB Activity Interrupt Active */
#define IABR1_IAB_CANACT (0x1 << 2) /* CAN Activity Interrupt Active */

/* Interrupt Priority Register 0 (IPR0 - 0xE000 E400) */
#define IPR0_IP_WDT    (0x1F << 3) /* Watchdog Timer Interrupt Priority */
#define IPR0_IP_TIMER0 (0x1F << 11) /* Timer 0 Interrupt Priority */
#define IPR0_IP_TIMER1 (0x1F << 19) /* Timer 1 Interrupt Priority */
#define IPR0_IP_TIMER2 (0x1F << 27) /* Timer 2 Interrupt Priority */

/* Interrupt Priority Register 1 (IPR1 - 0xE000 E404) */
#define IPR1_IP_TIMER3 (0x1F << 3) /* Timer 3 Interrupt Priority */
#define IPR1_IP_UART0  (0x1F << 11) /* UART0 Interrupt Priority */
#define IPR1_IP_UART1  (0x1F << 19) /* UART1 Interrupt Priority */
#define IPR1_IP_UART2  (0x1F << 27) /* UART2 Interrupt Priority */

/* Interrupt Priority Register 2 (IPR2 - 0xE000 E408) */
#define IPR2_IP_UART3 (0x1F << 3) /* UART3 Interrupt Priority */
#define IPR2_IP_PWM   (0x1F << 11) /* PWM Interrupt Priority */
#define IPR2_IP_I2C0  (0x1F << 19) /* I2C0 Interrupt Priority */
#define IPR2_IP_I2C1  (0x1F << 27) /* I2C1 Interrupt Priority */

/* Interrupt Priority Register 3 (IPR3 - 0xE000 E40C) */
#define IPR3_IP_I2C2 (0x1F << 3) /* I2C2 Interrupt Priority */
#define IPR3_IP_SPI  (0x1F << 11) /* SPI Interrupt Priority */
#define IPR3_IP_SSP0 (0x1F << 19) /* SSP0 Interrupt Priority */
#define IPR3_IP_SSP1 (0x1F << 27) /* SSP1 Interrupt Priority */

/* Interrupt Priority Register 4 (IPR4 - 0xE000 E410) */
#define IPR4_IP_PLL0  (0x1F << 3) /* PLL0 (Main PLL) Interrupt Priority */
#define IPR4_IP_RTC   (0x1F << 11) /* Real Time Clock (RTC) Interrupt Priority */
#define IPR4_IP_EINT0 (0x1F << 19) /* External Interrupt 0 Interrupt Priority */
#define IPR4_IP_EINT1 (0x1F << 27) /* External Interrupt 1 Interrupt Priority */

/* Interrupt Priority Register 5 (IPR5 - 0xE000 E414) */
#define IPR5_IP_EINT2 (0x1F << 3) /* External Interrupt 2 Interrupt Priority */
#define IPR5_IP_EINT3 (0x1F << 11) /* External Interrupt 3 Interrupt Priority */
#define IPR5_IP_ADC   (0x1F << 19) /* ADC Interrupt Priority */
#define IPR5_IP_BOD   (0x1F << 27) /* BOD Interrupt Priority */

/* Interrupt Priority Register 6 (IPR6 - 0xE000 E418) */
#define IPR6_IP_USB (0x1F << 3) /* USB Interrupt Priority */
#define IPR6_IP_CAN (0x1F << 11) /* CAN Interrupt Priority */
#define IPR6_IP_DMA (0x1F << 19) /* GPDMA Interrupt Priority */
#define IPR6_IP_I2S (0x1F << 27) /* I2S Interrupt Priority */

/* Interrupt Priority Register 7 (IPR7 - 0xE000 E41C) */
#define IPR7_IP_ENET  (0x1F << 3) /* Ethernet Interrupt Priority */
#define IPR7_IP_RIT   (0x1F << 11) /* Repetitive Interrupt Timer Interrupt Priority */
#define IPR7_IP_MCPWM (0x1F << 19) /* Motor Control PWM Interrupt Priority */
#define IPR7_IP_QEI   (0x1F << 27) /* Quadrature Encoder Interface Interrupt Priority */

/* Interrupt Priority Register 8 (IPR8 - 0xE000 E420) */
#define IPR8_IP_PLL1   (0x1F << 3) /* PLL1 (USB PLL) Interrupt Priority */
#define IPR8_IP_USBACT (0x1F << 11) /* USB Activity Interrupt Priority */
#define IPR8_IP_CANACT (0x1F << 19) /* CAN Activity Interrupt Priority */

/* Software Trigger Interrupt Register (STIR - 0xE000 EF00) */
#define STIR_INTID (0x1FF << 0)

/* Cortex-M3 interrupt IDs */
#define IRQ_NonMaskableInt   (-14)
#define IRQ_MemoryManagement (-12)
#define IRQ_BusFault         (-11)
#define IRQ_UsageFault       (-10)
#define IRQ_SVCall           (-5)
#define IRQ_DebugMonitor     (-4)
#define IRQ_PendSV           (-2)
#define IRQ_SysTick          (-1)

/* LPC17xx interrupt IDs */
#define IRQ_WDT      (0)
#define IRQ_TIMER0   (1)
#define IRQ_TIMER1   (2)
#define IRQ_TIMER2   (3)
#define IRQ_TIMER3   (4)
#define IRQ_UART0    (5)
#define IRQ_UART1    (6)
#define IRQ_UART2    (7)
#define IRQ_UART3    (8)
#define IRQ_PWM      (9)
#define IRQ_I2C0     (10)
#define IRQ_I2C1     (11)
#define IRQ_I2C2     (12)
#define IRQ_SPI      (13)
#define IRQ_SSP0     (14)
#define IRQ_SSP1     (15)
#define IRQ_PLL0     (16)
#define IRQ_RTC      (17)
#define IRQ_EINT0    (18)
#define IRQ_EINT1    (19)
#define IRQ_EINT2    (20)
#define IRQ_EINT3    (21)
#define IRQ_ADC      (22)
#define IRQ_BOD      (23)
#define IRQ_USB      (24)
#define IRQ_CAN      (25)
#define IRQ_DMA      (26)
#define IRQ_I2S      (27)
#define IRQ_ENET     (28)
#define IRQ_RIT      (29)
#define IRQ_MCPWM    (30)
#define IRQ_QEI      (31)
#define IRQ_PLL1     (32)
#define IRQ_USBACT   (33)
#define IRQ_CANACT   (34)

/* System Control Block (SCB) registers (UM10360 table 654) */
#define SCB_ACTLR   LPC17_REG(0xE000E008) /* Auxiliary Control Register */
#define SCB_CPUID   LPC17_REG(0xE000ED00) /* CPUID Base Register */
#define SCB_ICSR    LPC17_REG(0xE000ED04) /* Interrupt Control and State Register */
#define SCB_VTOR    LPC17_REG(0xE000ED08) /* Vector Table Offset Register */
#define SCB_AIRCR   LPC17_REG(0xE000ED0C) /* Application Interrupt and Reset Control Register */
#define SCB_SCR     LPC17_REG(0xE000ED10) /* System Control Register */
#define SCB_CCR     LPC17_REG(0xE000ED14) /* Configuration and Control Register */
#define SCB_SHPR1   LPC17_REG(0xE000ED18) /* System Handler Priority - register 1 */
#define SCB_SHPR2   LPC17_REG(0xE000ED1C) /* System Handler Priority - register 2 */
#define SCB_SHPR3   LPC17_REG(0xE000ED20) /* System Handler Priority - register 3 */
#define SCB_SHCRS   LPC17_REG(0xE000ED24) /* System Handler Control and State Register */
#define SCB_CFSR    LPC17_REG(0xE000ED28) /* Configurable Fault Status Register */
//#define SCB_MMSR    LPC17_REG(0xE000ED28) /* Memory Management Fault Status Register (CFSR sub-register) */
//#define SCB_BFSR    LPC17_REG(0xE000ED29) /* Bus Fault Status Register (CFSR sub-register) */
//#define SCB_UFSR    LPC17_REG(0xE000ED2A) /* Usage Fault Status Register (CFSR sub-register) */
#define SCB_HFSR    LPC17_REG(0xE000ED2C) /* Hard Fault Status Register */
#define SCB_MMFAR   LPC17_REG(0xE000ED34) /* Memory Management Fault Address Register */
#define SCB_BFAR    LPC17_REG(0xE000ED38) /* Bus Fault Address Register */

/* System Control Register (SCB_SCR - 0xE000 ED10) */
#define SCB_SCR_SEVONPEND   (0x1 << 4)
#define SCB_SCR_SLEEPDEEP   (0x1 << 2)
#define SCB_SCR_SLEEPONEXIT (0x1 << 1)

/* pin connect block registers */
#define PINSEL0     LPC17_REG(0x4002C000) /* Pin function select register 0 */
#define PINSEL1     LPC17_REG(0x4002C004) /* Pin function select register 1 */
#define PINSEL2     LPC17_REG(0x4002C008) /* Pin function select register 2 */
#define PINSEL3     LPC17_REG(0x4002C00C) /* Pin function select register 3 */
#define PINSEL4     LPC17_REG(0x4002C010) /* Pin function select register 4 */
#define PINSEL7     LPC17_REG(0x4002C01C) /* Pin function select register 7 */
#define PINSEL8     LPC17_REG(0x4002C020) /* Pin function select register 8 */
#define PINSEL9     LPC17_REG(0x4002C024) /* Pin function select register 9 */
#define PINSEL10    LPC17_REG(0x4002C028) /* Pin function select register 10 */
#define PINMODE0    LPC17_REG(0x4002C040) /* Pin mode select register 0 */
#define PINMODE1    LPC17_REG(0x4002C044) /* Pin mode select register 1 */
#define PINMODE2    LPC17_REG(0x4002C048) /* Pin mode select register 2 */
#define PINMODE3    LPC17_REG(0x4002C04C) /* Pin mode select register 3 */
#define PINMODE4    LPC17_REG(0x4002C050) /* Pin mode select register 4 */
#define PINMODE5    LPC17_REG(0x4002C054) /* Pin mode select register 5 */
#define PINMODE6    LPC17_REG(0x4002C058) /* Pin mode select register 6 */
#define PINMODE7    LPC17_REG(0x4002C05C) /* Pin mode select register 7 */
#define PINMODE9    LPC17_REG(0x4002C064) /* Pin mode select register 9 */
#define PINMODE_OD0 LPC17_REG(0x4002C068) /* Open drain mode control register 0 */
#define PINMODE_OD1 LPC17_REG(0x4002C06C) /* Open drain mode control register 1 */
#define PINMODE_OD2 LPC17_REG(0x4002C070) /* Open drain mode control register 2 */
#define PINMODE_OD3 LPC17_REG(0x4002C074) /* Open drain mode control register 3 */
#define PINMODE_OD4 LPC17_REG(0x4002C078) /* Open drain mode control register 4 */
#define I2CPADCFG   LPC17_REG(0x4002C07C) /* I2C Pin Configuration register */

/* Pin function select register 0 (PINSEL0 - 0x4002 C000) */
#define PINSEL0_P0_0  (0x3 << 0)
#define PINSEL0_P0_1  (0x3 << 2)
#define PINSEL0_P0_2  (0x3 << 4)
#define PINSEL0_P0_3  (0x3 << 6)
#define PINSEL0_P0_4  (0x3 << 8)
#define PINSEL0_P0_5  (0x3 << 10)
#define PINSEL0_P0_6  (0x3 << 12)
#define PINSEL0_P0_7  (0x3 << 14)
#define PINSEL0_P0_8  (0x3 << 16)
#define PINSEL0_P0_9  (0x3 << 18)
#define PINSEL0_P0_10 (0x3 << 20)
#define PINSEL0_P0_11 (0x3 << 22)
#define PINSEL0_P0_15 (0x3 << 30)

/* Pin function select register 1 (PINSEL1 - 0x4002 C004) */
#define PINSEL1_P0_16 (0x3 << 0)
#define PINSEL1_P0_17 (0x3 << 2)
#define PINSEL1_P0_18 (0x3 << 4)
#define PINSEL1_P0_19 (0x3 << 6)
#define PINSEL1_P0_20 (0x3 << 8)
#define PINSEL1_P0_21 (0x3 << 10)
#define PINSEL1_P0_22 (0x3 << 12)
#define PINSEL1_P0_23 (0x3 << 14)
#define PINSEL1_P0_24 (0x3 << 16)
#define PINSEL1_P0_25 (0x3 << 18)
#define PINSEL1_P0_26 (0x3 << 20)
#define PINSEL1_P0_27 (0x3 << 22)
#define PINSEL1_P0_28 (0x3 << 24)
#define PINSEL1_P0_29 (0x3 << 26)
#define PINSEL1_P0_30 (0x3 << 28)

/* Pin function select register 2 (PINSEL2 - 0x4002 C008) */
#define PINSEL2_P1_0  (0x3 << 0)
#define PINSEL2_P1_1  (0x3 << 2)
#define PINSEL2_P1_4  (0x3 << 8)
#define PINSEL2_P1_8  (0x3 << 16)
#define PINSEL2_P1_9  (0x3 << 18)
#define PINSEL2_P1_10 (0x3 << 20)
#define PINSEL2_P1_14 (0x3 << 28)
#define PINSEL2_P1_15 (0x3 << 30)

/* Pin function select register 3 (PINSEL3 - 0x4002 C00C) */
#define PINSEL3_P1_16 (0x3 << 0)
#define PINSEL3_P1_17 (0x3 << 2)
#define PINSEL3_P1_18 (0x3 << 4)
#define PINSEL3_P1_19 (0x3 << 6)
#define PINSEL3_P1_20 (0x3 << 8)
#define PINSEL3_P1_21 (0x3 << 10)
#define PINSEL3_P1_22 (0x3 << 12)
#define PINSEL3_P1_23 (0x3 << 14)
#define PINSEL3_P1_24 (0x3 << 16)
#define PINSEL3_P1_25 (0x3 << 18)
#define PINSEL3_P1_26 (0x3 << 20)
#define PINSEL3_P1_27 (0x3 << 22)
#define PINSEL3_P1_28 (0x3 << 24)
#define PINSEL3_P1_29 (0x3 << 26)
#define PINSEL3_P1_30 (0x3 << 28)
#define PINSEL3_P1_31 (0x3 << 30)

/* Pin function select register 4 (PINSEL4 - 0x4002 C010) */
#define PINSEL4_P2_0  (0x3 << 0)
#define PINSEL4_P2_1  (0x3 << 2)
#define PINSEL4_P2_2  (0x3 << 4)
#define PINSEL4_P2_3  (0x3 << 6)
#define PINSEL4_P2_4  (0x3 << 8)
#define PINSEL4_P2_5  (0x3 << 10)
#define PINSEL4_P2_6  (0x3 << 12)
#define PINSEL4_P2_7  (0x3 << 14)
#define PINSEL4_P2_8  (0x3 << 16)
#define PINSEL4_P2_9  (0x3 << 18)
#define PINSEL4_P2_10 (0x3 << 20)
#define PINSEL4_P2_11 (0x3 << 22)
#define PINSEL4_P2_12 (0x3 << 24)
#define PINSEL4_P2_13 (0x3 << 26)

/* Pin function select register 7 (PINSEL7 - 0x4002 C01C) */
#define PINSEL7_P3_25 (0x3 << 18)
#define PINSEL7_P3_26 (0x3 << 20)

/* Pin function select register 9 (PINSEL9 - 0x4002 C024) */
#define PINSEL9_P4_28 (0x3 << 24)
#define PINSEL9_P4_29 (0x3 << 26)

/* Pin function select register 10 (PINSEL10 - 0x4002 C028) */
#define PINSEL10_GPIOTRACE (0x1 << 3)

/* Pin Mode select register 0 (PINMODE0 - 0x4002 C040) */
#define PINMODE0_P0_00MODE (0x3 << 0)
#define PINMODE0_P0_01MODE (0x3 << 2)
#define PINMODE0_P0_02MODE (0x3 << 4)
#define PINMODE0_P0_03MODE (0x3 << 6)
#define PINMODE0_P0_04MODE (0x3 << 8)
#define PINMODE0_P0_05MODE (0x3 << 10)
#define PINMODE0_P0_06MODE (0x3 << 12)
#define PINMODE0_P0_07MODE (0x3 << 14)
#define PINMODE0_P0_08MODE (0x3 << 16)
#define PINMODE0_P0_09MODE (0x3 << 18)
#define PINMODE0_P0_10MODE (0x3 << 20)
#define PINMODE0_P0_11MODE (0x3 << 22)
#define PINMODE0_P0_15MODE (0x3 << 30)

/* Pin Mode select register 1 (PINMODE1 - 0x4002 C044) */
#define PIMODE1_P0_16MODE (0x3 << 0)
#define PIMODE1_P0_17MODE (0x3 << 2)
#define PIMODE1_P0_18MODE (0x3 << 4)
#define PIMODE1_P0_19MODE (0x3 << 6)
#define PIMODE1_P0_20MODE (0x3 << 8)
#define PIMODE1_P0_21MODE (0x3 << 10)
#define PIMODE1_P0_22MODE (0x3 << 12)
#define PIMODE1_P0_23MODE (0x3 << 14)
#define PIMODE1_P0_24MODE (0x3 << 16)
#define PIMODE1_P0_25MODE (0x3 << 18)
#define PIMODE1_P0_26MODE (0x3 << 20)

/* Pin Mode select register 2 (PINMODE2 - 0x4002 C048) */
#define PINMODE2_P1_00MODE (0x3 << 0)
#define PINMODE2_P1_01MODE (0x3 << 2)
#define PINMODE2_P1_04MODE (0x3 << 8)
#define PINMODE2_P1_08MODE (0x3 << 16)
#define PINMODE2_P1_09MODE (0x3 << 18)
#define PINMODE2_P1_10MODE (0x3 << 20)
#define PINMODE2_P1_14MODE (0x3 << 28)
#define PINMODE2_P1_15MODE (0x3 << 30)

/* Pin Mode select register 3 (PINMODE3 - 0x4002 C04C) */
#define PINMODE3_P1_16MODE (0x3 << 0)
#define PINMODE3_P1_17MODE (0x3 << 2)
#define PINMODE3_P1_18MODE (0x3 << 4)
#define PINMODE3_P1_19MODE (0x3 << 6)
#define PINMODE3_P1_20MODE (0x3 << 8)
#define PINMODE3_P1_21MODE (0x3 << 10)
#define PINMODE3_P1_22MODE (0x3 << 12)
#define PINMODE3_P1_23MODE (0x3 << 14)
#define PINMODE3_P1_24MODE (0x3 << 16)
#define PINMODE3_P1_25MODE (0x3 << 18)
#define PINMODE3_P1_26MODE (0x3 << 20)
#define PINMODE3_P1_27MODE (0x3 << 22)
#define PINMODE3_P1_28MODE (0x3 << 24)
#define PINMODE3_P1_29MODE (0x3 << 26)
#define PINMODE3_P1_30MODE (0x3 << 28)
#define PINMODE3_P1_31MODE (0x3 << 30)

/* Pin Mode select register 4 (PINMODE4 - 0x4002 C050) */
#define PINMODE4_P2_00MODE (0x3 << 0)
#define PINMODE4_P2_01MODE (0x3 << 2)
#define PINMODE4_P2_02MODE (0x3 << 4)
#define PINMODE4_P2_03MODE (0x3 << 6)
#define PINMODE4_P2_04MODE (0x3 << 8)
#define PINMODE4_P2_05MODE (0x3 << 10)
#define PINMODE4_P2_06MODE (0x3 << 12)
#define PINMODE4_P2_07MODE (0x3 << 14)
#define PINMODE4_P2_08MODE (0x3 << 16)
#define PINMODE4_P2_09MODE (0x3 << 18)
#define PINMODE4_P2_10MODE (0x3 << 20)
#define PINMODE4_P2_11MODE (0x3 << 22)
#define PINMODE4_P2_12MODE (0x3 << 24)
#define PINMODE4_P2_13MODE (0x3 << 26)

/* Pin Mode select register 7 (PINMODE7 - 0x4002 C05C) */
#define PINMODE7_P3_25MODE (0x3 << 18)
#define PINMODE7_P3_26MODE (0x3 << 20)

/* Pin Mode select register 9 (PINMODE9 - 0x4002 C064) */
#define PINMODE9_P4_28MODE (0x3 << 24)
#define PINMODE9_P4_29MODE (0x3 << 26)

/*
 * skipped:
 *
 * Open Drain Pin Mode select register 0 (PINMODE_OD0 - 0x4002 C068)
 * Open Drain Pin Mode select register 1 (PINMODE_OD1 - 0x4002 C06C)
 * Open Drain Pin Mode select register 2 (PINMODE_OD2 - 0x4002 C070)
 * Open Drain Pin Mode select register 3 (PINMODE_OD3 - 0x4002 C074)
 * Open Drain Pin Mode select register 4 (PINMODE_OD4 - 0x4002 C078)
 */

/* I2C Pin Configuration register (I2CPADCFG - 0x4002 C07C) */
#define I2CPADCFG_SDADRV0 (0x1 << 0)
#define I2CPADCFG_SDAI2C0 (0x1 << 1)
#define I2CPADCFG_SCLDRV0 (0x1 << 2)
#define I2CPADCFG_SCLI2C0 (0x1 << 3)


/* GPIO registers */
#define FIO0DIR  LPC17_REG(0x2009C000) /* Fast GPIO Port Direction control register 0 (FIODIR) */
#define FIO1DIR  LPC17_REG(0x2009C020) /* Fast GPIO Port Direction control register 1 (FIODIR) */
#define FIO2DIR  LPC17_REG(0x2009C040) /* Fast GPIO Port Direction control register 2 (FIODIR) */
#define FIO3DIR  LPC17_REG(0x2009C060) /* Fast GPIO Port Direction control register 3 (FIODIR) */
#define FIO4DIR  LPC17_REG(0x2009C080) /* Fast GPIO Port Direction control register 4 (FIODIR) */
#define FIO0MASK LPC17_REG(0x2009C010) /* Fast Mask register for port 0 (FIOMASK) */
#define FIO1MASK LPC17_REG(0x2009C030) /* Fast Mask register for port 1 (FIOMASK) */
#define FIO2MASK LPC17_REG(0x2009C050) /* Fast Mask register for port 2 (FIOMASK) */
#define FIO3MASK LPC17_REG(0x2009C070) /* Fast Mask register for port 3 (FIOMASK) */
#define FIO4MASK LPC17_REG(0x2009C090) /* Fast Mask register for port 4 (FIOMASK) */
#define FIO0PIN  LPC17_REG(0x2009C014) /* Fast Port 0 Pin value register using FIOMASK (FIOPIN) */
#define FIO1PIN  LPC17_REG(0x2009C034) /* Fast Port 1 Pin value register using FIOMASK (FIOPIN) */
#define FIO2PIN  LPC17_REG(0x2009C054) /* Fast Port 2 Pin value register using FIOMASK (FIOPIN) */
#define FIO3PIN  LPC17_REG(0x2009C074) /* Fast Port 3 Pin value register using FIOMASK (FIOPIN) */
#define FIO4PIN  LPC17_REG(0x2009C094) /* Fast Port 4 Pin value register using FIOMASK (FIOPIN) */
#define FIO0SET  LPC17_REG(0x2009C018) /* Fast Port 0 Output Set register using FIOMASK (FIOSET) */
#define FIO1SET  LPC17_REG(0x2009C038) /* Fast Port 1 Output Set register using FIOMASK (FIOSET) */
#define FIO2SET  LPC17_REG(0x2009C058) /* Fast Port 2 Output Set register using FIOMASK (FIOSET) */
#define FIO3SET  LPC17_REG(0x2009C078) /* Fast Port 3 Output Set register using FIOMASK (FIOSET) */
#define FIO4SET  LPC17_REG(0x2009C098) /* Fast Port 4 Output Set register using FIOMASK (FIOSET) */
#define FIO0CLR  LPC17_REG(0x2009C01C) /* Fast Port 0 Output Clear register using FIOMASK (FIOCLR) */
#define FIO1CLR  LPC17_REG(0x2009C03C) /* Fast Port 1 Output Clear register using FIOMASK (FIOCLR) */
#define FIO2CLR  LPC17_REG(0x2009C05C) /* Fast Port 2 Output Clear register using FIOMASK (FIOCLR) */
#define FIO3CLR  LPC17_REG(0x2009C07C) /* Fast Port 3 Output Clear register using FIOMASK (FIOCLR) */
#define FIO4CLR  LPC17_REG(0x2009C09C) /* Fast Port 4 Output Clear register using FIOMASK (FIOCLR) */

/* GPIO interrupt registers */
#define IO0IntEnR   LPC17_REG(0x40028090) /* IntEnR: GPIO Interrupt Enable for Rising edge */
#define IO2IntEnR   LPC17_REG(0x400280B0) /* IntEnR: GPIO Interrupt Enable for Rising edge */
#define IO0IntEnF   LPC17_REG(0x40028094) /* IntEnF: GPIO Interrupt Enable for Falling edge */
#define IO2IntEnF   LPC17_REG(0x400280B4) /* IntEnF: GPIO Interrupt Enable for Falling edge */
#define IO0IntStatR LPC17_REG(0x40028084) /* IntStatR: GPIO Interrupt Status for Rising edge */
#define IO2IntStatR LPC17_REG(0x400280A4) /* IntStatR: GPIO Interrupt Status for Rising edge */
#define IO0IntStatF LPC17_REG(0x40028088) /* IntStatF: GPIO Interrupt Status for Falling edge */
#define IO2IntStatF LPC17_REG(0x400280A8) /* IntStatF: GPIO Interrupt Status for Falling edge */
#define IO0IntClr   LPC17_REG(0x4002808C) /* IntClr: GPIO Interrupt Clear */
#define IO2IntClr   LPC17_REG(0x400280AC) /* IntClr: GPIO Interrupt Clear */
#define IOIntStatus LPC17_REG(0x40028080) /* IntStatus: GPIO overall Interrupt Status */

/* byte accessible FIODIR registers */
#define FIO0DIR0 LPC17_REG8(0x2009C000)
#define FIO1DIR0 LPC17_REG8(0x2009C020)
#define FIO2DIR0 LPC17_REG8(0x2009C040)
#define FIO3DIR0 LPC17_REG8(0x2009C060)
#define FIO4DIR0 LPC17_REG8(0x2009C080)
#define FIO0DIR1 LPC17_REG8(0x2009C001)
#define FIO1DIR1 LPC17_REG8(0x2009C021)
#define FIO2DIR1 LPC17_REG8(0x2009C041)
#define FIO3DIR1 LPC17_REG8(0x2009C061)
#define FIO4DIR1 LPC17_REG8(0x2009C081)
#define FIO0DIR2 LPC17_REG8(0x2009C002)
#define FIO1DIR2 LPC17_REG8(0x2009C022)
#define FIO2DIR2 LPC17_REG8(0x2009C042)
#define FIO3DIR2 LPC17_REG8(0x2009C062)
#define FIO4DIR2 LPC17_REG8(0x2009C082)
#define FIO0DIR3 LPC17_REG8(0x2009C003)
#define FIO1DIR3 LPC17_REG8(0x2009C023)
#define FIO2DIR3 LPC17_REG8(0x2009C043)
#define FIO3DIR3 LPC17_REG8(0x2009C063)
#define FIO4DIR3 LPC17_REG8(0x2009C083)

/* half-word accessible FIODIR registers */
#define FIO0DIRL LPC17_REG16(0x2009C000)
#define FIO1DIRL LPC17_REG16(0x2009C020)
#define FIO2DIRL LPC17_REG16(0x2009C040)
#define FIO3DIRL LPC17_REG16(0x2009C060)
#define FIO4DIRL LPC17_REG16(0x2009C080)
#define FIO0DIRU LPC17_REG16(0x2009C002)
#define FIO1DIRU LPC17_REG16(0x2009C022)
#define FIO2DIRU LPC17_REG16(0x2009C042)
#define FIO3DIRU LPC17_REG16(0x2009C062)
#define FIO4DIRU LPC17_REG16(0x2009C082)

/* byte accessible FIOSET registers */
#define FIO0SET0 LPC17_REG8(0x2009C018)
#define FIO1SET0 LPC17_REG8(0x2009C038)
#define FIO2SET0 LPC17_REG8(0x2009C058)
#define FIO3SET0 LPC17_REG8(0x2009C078)
#define FIO4SET0 LPC17_REG8(0x2009C098)
#define FIO0SET1 LPC17_REG8(0x2009C019)
#define FIO1SET1 LPC17_REG8(0x2009C039)
#define FIO2SET1 LPC17_REG8(0x2009C059)
#define FIO3SET1 LPC17_REG8(0x2009C079)
#define FIO4SET1 LPC17_REG8(0x2009C099)
#define FIO0SET2 LPC17_REG8(0x2009C01A)
#define FIO1SET2 LPC17_REG8(0x2009C03A)
#define FIO2SET2 LPC17_REG8(0x2009C05A)
#define FIO3SET2 LPC17_REG8(0x2009C07A)
#define FIO4SET2 LPC17_REG8(0x2009C09A)
#define FIO0SET3 LPC17_REG8(0x2009C01B)
#define FIO1SET3 LPC17_REG8(0x2009C03B)
#define FIO2SET3 LPC17_REG8(0x2009C05B)
#define FIO3SET3 LPC17_REG8(0x2009C07B)
#define FIO4SET3 LPC17_REG8(0x2009C09B)

/* half-word accessible FIOSET registers */
#define FIO0SETL LPC17_REG16(0x2009C018)
#define FIO1SETL LPC17_REG16(0x2009C038)
#define FIO2SETL LPC17_REG16(0x2009C058)
#define FIO3SETL LPC17_REG16(0x2009C078)
#define FIO4SETL LPC17_REG16(0x2009C098)
#define FIO0SETU LPC17_REG16(0x2009C01A)
#define FIO1SETU LPC17_REG16(0x2009C03A)
#define FIO2SETU LPC17_REG16(0x2009C05A)
#define FIO3SETU LPC17_REG16(0x2009C07A)
#define FIO4SETU LPC17_REG16(0x2009C09A)

/* byte accessible FIOCLR registers */
#define FIO0CLR0 LPC17_REG8(0x2009C01C)
#define FIO1CLR0 LPC17_REG8(0x2009C03C)
#define FIO2CLR0 LPC17_REG8(0x2009C05C)
#define FIO3CLR0 LPC17_REG8(0x2009C07C)
#define FIO4CLR0 LPC17_REG8(0x2009C09C)
#define FIO0CLR1 LPC17_REG8(0x2009C01D)
#define FIO1CLR1 LPC17_REG8(0x2009C03D)
#define FIO2CLR1 LPC17_REG8(0x2009C05D)
#define FIO3CLR1 LPC17_REG8(0x2009C07D)
#define FIO4CLR1 LPC17_REG8(0x2009C09D)
#define FIO0CLR2 LPC17_REG8(0x2009C01E)
#define FIO1CLR2 LPC17_REG8(0x2009C03E)
#define FIO2CLR2 LPC17_REG8(0x2009C05E)
#define FIO3CLR2 LPC17_REG8(0x2009C07E)
#define FIO4CLR2 LPC17_REG8(0x2009C09E)
#define FIO0CLR3 LPC17_REG8(0x2009C01F)
#define FIO1CLR3 LPC17_REG8(0x2009C03F)
#define FIO2CLR3 LPC17_REG8(0x2009C05F)
#define FIO3CLR3 LPC17_REG8(0x2009C07F)
#define FIO4CLR3 LPC17_REG8(0x2009C09F)

/* half-word accessible FIOCLR registers */
#define FIO0CLRL LPC17_REG16(0x2009C01C)
#define FIO1CLRL LPC17_REG16(0x2009C03C)
#define FIO2CLRL LPC17_REG16(0x2009C05C)
#define FIO3CLRL LPC17_REG16(0x2009C07C)
#define FIO4CLRL LPC17_REG16(0x2009C09C)
#define FIO0CLRU LPC17_REG16(0x2009C01E)
#define FIO1CLRU LPC17_REG16(0x2009C03E)
#define FIO2CLRU LPC17_REG16(0x2009C05E)
#define FIO3CLRU LPC17_REG16(0x2009C07E)
#define FIO4CLRU LPC17_REG16(0x2009C09E)

/* byte accessible FIOPIN registers */
#define FIO0PIN0 LPC17_REG8(0x2009C014)
#define FIO1PIN0 LPC17_REG8(0x2009C034)
#define FIO2PIN0 LPC17_REG8(0x2009C054)
#define FIO3PIN0 LPC17_REG8(0x2009C074)
#define FIO4PIN0 LPC17_REG8(0x2009C094)
#define FIO0PIN1 LPC17_REG8(0x2009C015)
#define FIO1PIN1 LPC17_REG8(0x2009C035)
#define FIO2PIN1 LPC17_REG8(0x2009C055)
#define FIO3PIN1 LPC17_REG8(0x2009C075)
#define FIO4PIN1 LPC17_REG8(0x2009C095)
#define FIO0PIN2 LPC17_REG8(0x2009C016)
#define FIO1PIN2 LPC17_REG8(0x2009C036)
#define FIO2PIN2 LPC17_REG8(0x2009C056)
#define FIO3PIN2 LPC17_REG8(0x2009C076)
#define FIO4PIN2 LPC17_REG8(0x2009C096)
#define FIO0PIN3 LPC17_REG8(0x2009C017)
#define FIO1PIN3 LPC17_REG8(0x2009C037)
#define FIO2PIN3 LPC17_REG8(0x2009C057)
#define FIO3PIN3 LPC17_REG8(0x2009C077)
#define FIO4PIN3 LPC17_REG8(0x2009C097)

/* half-word accessible FIOPIN registers */
#define FIO0PINL LPC17_REG16(0x2009C014)
#define FIO1PINL LPC17_REG16(0x2009C034)
#define FIO2PINL LPC17_REG16(0x2009C054)
#define FIO3PINL LPC17_REG16(0x2009C074)
#define FIO4PINL LPC17_REG16(0x2009C094)
#define FIO0PINU LPC17_REG16(0x2009C016)
#define FIO1PINU LPC17_REG16(0x2009C036)
#define FIO2PINU LPC17_REG16(0x2009C056)
#define FIO3PINU LPC17_REG16(0x2009C076)
#define FIO4PINU LPC17_REG16(0x2009C096)

/* byte accessible FIOMASK registers */
#define FIO0MASK0 LPC17_REG8(0x2009C010)
#define FIO1MASK0 LPC17_REG8(0x2009C030)
#define FIO2MASK0 LPC17_REG8(0x2009C050)
#define FIO3MASK0 LPC17_REG8(0x2009C070)
#define FIO4MASK0 LPC17_REG8(0x2009C090)
#define FIO0MASK1 LPC17_REG8(0x2009C011)
#define FIO1MASK1 LPC17_REG8(0x2009C031)
#define FIO2MASK1 LPC17_REG8(0x2009C051)
#define FIO3MASK1 LPC17_REG8(0x2009C071)
#define FIO4MASK1 LPC17_REG8(0x2009C091)
#define FIO0MASK2 LPC17_REG8(0x2009C012)
#define FIO1MASK2 LPC17_REG8(0x2009C032)
#define FIO2MASK2 LPC17_REG8(0x2009C052)
#define FIO3MASK2 LPC17_REG8(0x2009C072)
#define FIO4MASK2 LPC17_REG8(0x2009C092)
#define FIO0MASK3 LPC17_REG8(0x2009C013)
#define FIO1MASK3 LPC17_REG8(0x2009C033)
#define FIO2MASK3 LPC17_REG8(0x2009C053)
#define FIO3MASK3 LPC17_REG8(0x2009C073)
#define FIO4MASK3 LPC17_REG8(0x2009C093)

/* half-word accessible FIOMASK registers */
#define FIO0MASKL LPC17_REG16(0x2009C010)
#define FIO1MASKL LPC17_REG16(0x2009C030)
#define FIO2MASKL LPC17_REG16(0x2009C050)
#define FIO3MASKL LPC17_REG16(0x2009C070)
#define FIO4MASKL LPC17_REG16(0x2009C090)
#define FIO0MASKU LPC17_REG16(0x2009C012)
#define FIO1MASKU LPC17_REG16(0x2009C032)
#define FIO2MASKU LPC17_REG16(0x2009C052)
#define FIO3MASKU LPC17_REG16(0x2009C072)
#define FIO4MASKU LPC17_REG16(0x2009C092)

/* GPIO overall Interrupt Status register (IOIntStatus - 0x4002 8080) */
#define IOIntStatus_P0Int (0x1 << 0)
#define IOIntStatus_P2Int (0x1 << 2)

/*
 * skipped:
 *
 * GPIO Interrupt Enable for port 0 Rising Edge (IO0IntEnR - 0x4002 8090)
 * GPIO Interrupt Enable for port 2 Rising Edge (IO2IntEnR - 0x4002 80B0)
 * GPIO Interrupt Enable for port 0 Falling Edge (IO0IntEnF - 0x4002 8094)
 * GPIO Interrupt Enable for port 2 Falling Edge (IO2IntEnF - 0x4002 80B4)
 * GPIO Interrupt Status for port 0 Rising Edge Interrupt (IO0IntStatR - 0x4002 8084)
 * GPIO Interrupt Status for port 2 Rising Edge Interrupt (IO2IntStatR - 0x4002 80A4)
 * GPIO Interrupt Status for port 0 Falling Edge Interrupt (IO0IntStatF - 0x4002 8088)
 * GPIO Interrupt Status for port 2 Falling Edge Interrupt (IO2IntStatF - 0x4002 80A8)
 * GPIO Interrupt Clear register for port 0 (IO0IntClr - 0x4002 808C)
 * GPIO Interrupt Clear register for port 0 (IO2IntClr - 0x4002 80AC)
 */


/* ethernet registers */

/* MAC registers */
#define MAC1 LPC17_REG(0x50000000) /* MAC configuration register 1 */
#define MAC2 LPC17_REG(0x50000004) /* MAC configuration register 2 */
#define IPGT LPC17_REG(0x50000008) /* Back-to-Back Inter-Packet-Gap register */
#define IPGR LPC17_REG(0x5000000C) /* Non Back-to-Back Inter-Packet-Gap register */
#define CLRT LPC17_REG(0x50000010) /* Collision window / Retry register */
#define MAXF LPC17_REG(0x50000014) /* Maximum Frame register */
#define SUPP LPC17_REG(0x50000018) /* PHY Support register */
#define TEST LPC17_REG(0x5000001C) /* Test register */
#define MCFG LPC17_REG(0x50000020) /* MII Mgmt Configuration register */
#define MCMD LPC17_REG(0x50000024) /* MII Mgmt Command register */
#define MADR LPC17_REG(0x50000028) /* MII Mgmt Address register */
#define MWTD LPC17_REG(0x5000002C) /* MII Mgmt Write Data register */
#define MRDD LPC17_REG(0x50000030) /* MII Mgmt Read Data register */
#define MIND LPC17_REG(0x50000034) /* MII Mgmt Indicators register */
#define SA0  LPC17_REG(0x50000040) /* Station Address 0 register */
#define SA1  LPC17_REG(0x50000044) /* Station Address 1 register */
#define SA2  LPC17_REG(0x50000048) /* Station Address 2 register */

/* Control registers */
#define Command            LPC17_REG(0x50000100) /* Command register */
#define Status             LPC17_REG(0x50000104) /* Status register */
#define RxDescriptor       LPC17_REG(0x50000108) /* Receive descriptor base address register */
#define RxStatus           LPC17_REG(0x5000010C) /* Receive status base address register */
#define RxDescriptorNumber LPC17_REG(0x50000110) /* Receive number of descriptors register */
#define RxProduceIndex     LPC17_REG(0x50000114) /* Receive produce index register */
#define RxConsumeIndex     LPC17_REG(0x50000118) /* Receive consume index register */
#define TxDescriptor       LPC17_REG(0x5000011C) /* Transmit descriptor base address register */
#define TxStatus           LPC17_REG(0x50000120) /* Transmit status base address register */
#define TxDescriptorNumber LPC17_REG(0x50000124) /* Transmit number of descriptors register */
#define TxProduceIndex     LPC17_REG(0x50000128) /* Transmit produce index register */
#define TxConsumeIndex     LPC17_REG(0x5000012C) /* Transmit consume index register */
#define TSV0               LPC17_REG(0x50000158) /* Transmit status vector 0 register */
#define TSV1               LPC17_REG(0x5000015C) /* Transmit status vector 1 register */
#define RSV                LPC17_REG(0x50000160) /* Receive status vector register */
#define FlowControlCounter LPC17_REG(0x50000170) /* Flow control counter register */
#define FlowControlStatus  LPC17_REG(0x50000170) /* Flow control status register */

/* Rx filter registers */
#define RxFliterCtrl      LPC17_REG(0x50000200) /* Receive filter control register */
#define RxFilterWoLStatus LPC17_REG(0x50000204) /* Receive filter WoL status register */
#define RxFilterWoLClear  LPC17_REG(0x50000208) /* Receive filter WoL clear register */
#define HashFilterL       LPC17_REG(0x50000210) /* Hash filter table LSBs register */
#define HashFilterH       LPC17_REG(0x50000214) /* Hash filter table MSBs register */

/* Module control registers */
#define IntStatus LPC17_REG(0x50000FE0) /* Interrupt status register */
#define IntEnable LPC17_REG(0x50000FE4) /* Interrupt enable register */
#define IntClear  LPC17_REG(0x50000FE8) /* Interrupt clear register */
#define IntSet    LPC17_REG(0x50000FEC) /* Interrupt set register */
#define PowerDown LPC17_REG(0x50000FF4) /* Power-down register */

/* MAC Configuration register 1 (MAC1 - 0x5000 0000) */
#define MAC1_RECEIVE_ENABLE          (0x1 << 0)
#define MAC1_PASS_ALL_RECEIVE_FRAMES (0x1 << 1)
#define MAC1_RX_FLOW_CONTROL         (0x1 << 2)
#define MAC1_TX_FLOW_CONTROL         (0x1 << 3)
#define MAC1_LOOPBACK                (0x1 << 4)
#define MAC1_RESET_TX                (0x1 << 8)
#define MAC1_RESET_MCS_TX            (0x1 << 9)
#define MAC1_RESET_RX                (0x1 << 10)
#define MAC1_RESET_MCS_RX            (0x1 << 11)
#define MAC1_SIMULATION_RESET        (0x1 << 14)
#define MAC1_SOFT_RESET              (0x1 << 15)

/* MAC Configuration register 2 (MAC2 - 0x5000 0004) */
#define MAC2_FULL_DUPLEX                (0x1 << 0)
#define MAC2_FRAME_LENGTH_CHECKING      (0x1 << 1)
#define MAC2_HUGE_FRAME_ENABLE          (0x1 << 2)
#define MAC2_DELAYED_CRC                (0x1 << 3)
#define MAC2_CRC_ENABLE                 (0x1 << 4)
#define MAC2_PAD_CRC_ENABLE             (0x1 << 5)
#define MAC2_VLAN_PAD_ENABLE            (0x1 << 6)
#define MAC2_AUTO_DETECT_PAD_ENABLE     (0x1 << 7)
#define MAC2_PURE_PREAMBLE_ENFORCEMENT  (0x1 << 8)
#define MAC2_LONG_PREAMBLE_ENFORCEMENT  (0x1 << 9)
#define MAC2_NO_BACKOFF                 (0x1 << 12)
#define MAC2_BACK_PRESSURE_NO_BACKOFF   (0x1 << 13)
#define MAC2_EXCESS_DEFER               (0x1 << 14)

/* skipped Back-to-back Inter-packet-gap register (IPGT - 0x5000 0008) */

/* Non Back-to-back Inter-packet-gap register (IPGR - 0x5000 000C) */
#define IPGR_PART2 (0x7F << 0)
#define IPGR_PART1 (0x7F << 8)

/* Collision Window / Retry register (CLRT - 0x5000 0010) */
#define CLRT_RETRANSMISSION_MAXIMUM (0xF << 0)
#define CLRT_COLLISION_WINDOW       (0x3F << 8)

/* skipped Maximum Frame register (MAXF - 0x5000 0014) */

/* PHY Support register (SUPP - 0x5000 0018) */
#define SUPP_SPEED (0x1 << 8)

/* Test register (TEST - 0x5000 ) */
#define TEST_SHORTCUT_PAUSE_QUANTA (0x1 << 0)
#define TEST_PAUSE                 (0x1 << 1)
#define TEST_BACKPRESSURE          (0x1 << 2)

/* MII Mgmt Configuration register (MCFG - 0x5000 0020) */
#define MCFG_SCAN_INCREMENT    (0x1 << 0)
#define MCFG_SUPPRESS_PREAMBLE (0x1 << 1)
#define MCFG_CLOCK_SELECT      (0xF << 2)
#define MCFG_RESET_MII_MGMT    (0x1 << 15)

/* MII Mgmt Command register (MCMD - 0x5000 0024) */
#define MCMD_READ (0x1 << 0)
#define MCMD_SCAN (0x1 << 1)

/* MII Mgmt Address register (MADR - 0x5000 0028) */
#define MADR_REGISTER_ADDRESS (0x1F << 0)
#define MADR_PHY_ADDRESS      (0x1F << 8)

/*
 * skipped:
 *
 * MII Mgmt Write Data register (MWTD - 0x5000 002C)
 * MII Mgmt Read Data register (MRDD - 0x5000 0030)
 */

/* MII Mgmt Indicators register (MIND - 0x5000 0034) */
#define MIND_BUSY          (0x1 << 0)
#define MIND_SCANNING      (0x1 << 1)
#define MIND_NOT_VALID     (0x1 << 2)
#define MIND_MII_LINK_FAIL (0x1 << 3)

/* Station Address register (SA0 - 0x5000 0040) */
#define SA0_OCTET2 (0xFF << 0)
#define SA0_OCTET1 (0xFF << 8)

/* Station Address register (SA1 - 0x5000 0044) */
#define SA1_OCTET4 (0xFF << 0)
#define SA1_OCTET3 (0xFF << 8)

/* Station Address register (SA2 - 0x5000 0048) */
#define SA2_OCTET6 (0xFF << 0)
#define SA2_OCTET5 (0xFF << 8)

/* Command register (Command - 0x5000 0100) */
#define Command_RxEnable      (0x1 << 0)
#define Command_TxEnable      (0x1 << 1)
#define Command_RegReset      (0x1 << 3)
#define Command_TxReset       (0x1 << 4)
#define Command_RxReset       (0x1 << 5)
#define Command_PassRuntFrame (0x1 << 6)
#define Command_PassRxFilter  (0x1 << 7)
#define Command_TxFlowControl (0x1 << 8)
#define Command_RMII          (0x1 << 9)
#define Command_FullDuplex    (0x1 << 10)

/* Status register (Status - 0x5000 0104) */
#define Status_RxStatus (0x1 << 0)
#define Status_TxStatus (0x1 << 1)

/*
 * skipped:
 *
 * Receive Descriptor Base Address register (RxDescriptor - 0x5000 0108)
 * Receive Status Base Address register (RxStatus - 0x5000 010C)
 * Receive Number of Descriptors register (RxDescriptor - 0x5000 0110) 
 * Receive Produce Index register (RxProduceIndex - 0x5000 0114)
 * Receive Consume Index register (RxConsumeIndex - 0x5000 0118)
 * Transmit Descriptor Base Address register (TxDescriptor - 0x5000 011C)
 * Transmit Status Base Address register (TxStatus - 0x5000 0120)
 * Transmit Number of Descriptors register (TxDescriptorNumber - 0x5000 0124)
 * Transmit Produce Index register (TxProduceIndex - 0x5000 0128)
 * Transmit Consume Index register (TxConsumeIndex - 0x5000 012C)
 */

/* Transmit Status Vector 0 register (TSV0 - 0x5000 0158) */
#define TSV0_CRC_error           (0x1 << 0)
#define TSV0_Length_check_error  (0x1 << 1)
#define TSV0_Length_out_of_range (0x1 << 2)
#define TSV0_Done                (0x1 << 3)
#define TSV0_Multicast           (0x1 << 4)
#define TSV0_Broadcast           (0x1 << 5)
#define TSV0_Packet_Defer        (0x1 << 6)
#define TSV0_Excessive_Defer     (0x1 << 7)
#define TSV0_Excessive_Collision (0x1 << 8)
#define TSV0_Late_Collision      (0x1 << 9)
#define TSV0_Giant               (0x1 << 10)
#define TSV0_Underrun            (0x1 << 11)
#define TSV0_Total_bytes         (0xFFFF << 12)
#define TSV0_Control_frame       (0x1 << 28)
#define TSV0_Pause               (0x1 << 29)
#define TSV0_Backpressure        (0x1 << 30)
#define TSV0_VLAN                (0x1 << 31)

/* Transmit Status Vector 1 register (TSV1 - 0x5000 015C) */
#define TSV1_Transmit_byte_count      (0xFFFF << 0)
#define TSV1_Transmit_collision_count (0xF << 16)

/* Receive Status Vector register (RSV - 0x5000 0160) */
#define RSV_Received_byte_count           (0xFFFF << 0)
#define RSV_Packet_previously_ignored     (0x1 << 16)
#define RSV_RXDV_event_previously_seen    (0x1 << 17)
#define RSV_Carrier_event_previously_seen (0x1 << 18)
#define RSV_Receive_code_violation        (0x1 << 19)
#define RSV_CRC_error                     (0x1 << 20)
#define RSV_Length_check_error            (0x1 << 21)
#define RSV_Length_out_of_range           (0x1 << 22)
#define RSV_Receive_OK                    (0x1 << 23)
#define RSV_Multicast                     (0x1 << 24)
#define RSV_Broadcast                     (0x1 << 25)
#define RSV_Dribble_Nibble                (0x1 << 26)
#define RSV_Control_frame                 (0x1 << 27)
#define RSV_PAUSE                         (0x1 << 28)
#define RSV_Unsupported_Opcode            (0x1 << 29)
#define RSV_VLAN                          (0x1 << 30)

/* Flow Control Counter register (FlowControlCounter - 0x5000 0170) */
#define FlowControlCounter_MirrorCounter (0xFFFF << 0)
#define FlowControlCounter_PauseTimer    (0xFFFF << 16)

/* Flow Control Status register (FlowControlStatus - 0x5000 0174) */
#define FlowControlStatus_MirrorCounterCurrent (0xFFFF << 0)

/* Receive Filter Control register (RxFilterCtrl - 0x5000 0200) */
#define RXFILTERCTRL_AcceptUnicastEn       (0x1 << 0)
#define RXFILTERCTRL_AcceptBroadcastEn     (0x1 << 1)
#define RXFILTERCTRL_AcceptMulticastEn     (0x1 << 2)
#define RXFILTERCTRL_AcceptUnicastHashEn   (0x1 << 3)
#define RXFILTERCTRL_AcceptMulticastHashEn (0x1 << 4)
#define RXFILTERCTRL_AcceptPerfectEn       (0x1 << 5)
#define RXFILTERCTRL_MagicPacketEnWoL      (0x1 << 12)
#define RXFILTERCTRL_RxFilterEnWoL         (0x1 << 13)

/* Receive Filter WoL Status register (RxFilterWoLStatus - 0x5000 0204) */
#define RxFilterWoLStatus_AcceptUnicastWoL       (0x1 << 0)
#define RxFilterWoLStatus_AcceptBroadcastWoL     (0x1 << 1)
#define RxFilterWoLStatus_AcceptMulticastWoL     (0x1 << 2)
#define RxFilterWoLStatus_AcceptUnicastHashWoL   (0x1 << 3)
#define RxFilterWoLStatus_AcceptMulticastHashWoL (0x1 << 4)
#define RxFilterWoLStatus_AcceptPerfectWoL       (0x1 << 5)
#define RxFilterWoLStatus_RxFilterWoL            (0x1 << 7)
#define RxFilterWoLStatus_MagicPacketWoL         (0x1 << 8)

/* Receive Filter WoL Clear register (RxFilterWoLClear - 0x5000 0208) */
#define RxFilterWoLClear_AcceptUnicastWoLClr       (0x1 << 0)
#define RxFilterWoLClear_AcceptBroadcastWoLClr     (0x1 << 1)
#define RxFilterWoLClear_AcceptMulticastWoLClr     (0x1 << 2)
#define RxFilterWoLClear_AcceptUnicastHashWoLClr   (0x1 << 3)
#define RxFilterWoLClear_AcceptMulticastHashWoLClr (0x1 << 4)
#define RxFilterWoLClear_AcceptPerfectWoLClr       (0x1 << 5)
#define RxFilterWoLClear_RxFilterWoLClr            (0x1 << 7)
#define RxFilterWoLClear_MagicPacketWoLClr         (0x1 << 6)

/*
 * skipped:
 *
 * Hash Filter Table LSBs register (HashFilterL - 0x5000 0210)
 * Hash Filter MSBs register (HashFilterH - 0x5000 0214)
 */

/* Interrupt Status register (IntStatus - 0x5000 0FE0) */
#define IntStatus_RxOverrunInt  (0x1 << 0)
#define IntStatus_RxErrorInt    (0x1 << 1)
#define IntStatus_RxFinishedInt (0x1 << 2)
#define IntStatus_RxDoneInt     (0x1 << 3)
#define IntStatus_TxUnderrunInt (0x1 << 4)
#define IntStatus_TxErrorInt    (0x1 << 5)
#define IntStatus_TxFinishedInt (0x1 << 6)
#define IntStatus_TxDoneInt     (0x1 << 7)
#define IntStatus_SoftInt       (0x1 << 12)
#define IntStatus_WakeupInt     (0x1 << 13)

/* Interrupt Enable register (intEnable - 0x5000 0FE4) */
#define IntEnable_RxOverrunIntEn  (0x1 << 0)
#define IntEnable_RxErrorIntEn    (0x1 << 1)
#define IntEnable_RxFinishedIntEn (0x1 << 2)
#define IntEnable_RxDoneIntEn     (0x1 << 3)
#define IntEnable_TxUnderrunIntEn (0x1 << 4)
#define IntEnable_TxErrorIntEn    (0x1 << 5)
#define IntEnable_TxFinishedIntEn (0x1 << 6)
#define IntEnable_TxDoneIntEn     (0x1 << 7)
#define IntEnable_SoftIntEn       (0x1 << 12)
#define IntEnable_WakeupIntEn     (0x1 << 13)

/* Interrupt Clear register (IntClear - 0x5000 0FE8) */
#define IntClear_RxOverrunIntClr  (0x1 << 0)
#define IntClear_RxErrorIntClr    (0x1 << 1)
#define IntClear_RxFinishedIntClr (0x1 << 2)
#define IntClear_RxDoneIntClr     (0x1 << 3)
#define IntClear_TxUnderrunIntClr (0x1 << 4)
#define IntClear_TxErrorIntClr    (0x1 << 5)
#define IntClear_TxFinishedIntClr (0x1 << 6)
#define IntClear_TxDoneIntClr     (0x1 << 7)
#define IntClear_SoftIntClr       (0x1 << 12)
#define IntClear_WakeupIntClr     (0x1 << 13)

/* Interrupt Set register (IntSet - 0x5000 0FEC) */
#define IntSet_RxOverrunIntSet  (0x1 << 0)
#define IntSet_RxErrorIntSet    (0x1 << 1)
#define IntSet_RxFinishedIntSet (0x1 << 2)
#define IntSet_RxDoneIntSet     (0x1 << 3)
#define IntSet_TxUnderrunIntSet (0x1 << 4)
#define IntSet_TxErrorIntSet    (0x1 << 5)
#define IntSet_TxFinishedIntSet (0x1 << 6)
#define IntSet_TxDoneIntSet     (0x1 << 7)
#define IntSet_SoftIntSet       (0x1 << 12)
#define IntSet_WakeupIntSet     (0x1 << 13)

/* Power-Down register (PowerDown - 0x5000 0FF4) */
#define PowerDown_PowerDownMACAHB (0x1 << 31)


/* USB device registers */

/* Clock control registers */
#define USBClkCtrl LPC17_REG(0x5000CFF4) /* USB Clock Control */
#define USBClkSt   LPC17_REG(0x5000CFF8) /* USB Clock Status */

/* Device interrupt registers */
#define USBIntSt     LPC17_REG(0x400FC1C0) /* USB Interrupt Status */
#define USBDevIntSt  LPC17_REG(0x5000C200) /* USB Device Interrupt Status */
#define USBDevIntEn  LPC17_REG(0x5000C204) /* USB Device Interrupt Enable */
#define USBDevIntClr LPC17_REG(0x5000C208) /* USB Device Interrupt Clear */
#define USBDevIntSet LPC17_REG(0x5000C20C) /* USB Device Interrupt Set */
#define USBDevIntPri LPC17_REG(0x5000C22C) /* USB Device Interrupt Priority */

/* Endpoint interrupt registers */
#define USBEpIntSt  LPC17_REG(0x5000C230) /* USB Endpoint Interrupt Status */
#define USBEpIntEn  LPC17_REG(0x5000C234) /* USB Endpoint Interrupt Enable */
#define USBEpIntClr LPC17_REG(0x5000C238) /* USB Endpoint Interrupt Clear */
#define USBEpIntSet LPC17_REG(0x5000C23C) /* USB Endpoint Interrupt Set */
#define USBEpIntPri LPC17_REG(0x5000C240) /* USB Endpoint Priority */

/* Endpoint realization registers */
#define USBReEp     LPC17_REG(0x5000C244) /* USB Realize Endpoint */
#define USBEpIn     LPC17_REG(0x5000C248) /* USB Endpoint Index */
#define USBMaxPSize LPC17_REG(0x5000C24C) /* USB MaxPacketSize */

/* USB transfer registers */
#define USBRxData LPC17_REG(0x5000C218) /* USB Receive Data */
#define USBRxPLen LPC17_REG(0x5000C220) /* USB Receive Packet Length */
#define USBTxData LPC17_REG(0x5000C21C) /* USB Transmit Data */
#define USBTxPLen LPC17_REG(0x5000C224) /* USB Transmit Packet Length */
#define USBCtrl   LPC17_REG(0x5000C228) /* USB Control */

/* SIE Command registers */
#define USBCmdCode LPC17_REG(0x5000C210) /* USB Command Code */
#define USBCmdData LPC17_REG(0x5000C214) /* USB Command Data */

/* DMA registers */
#define USBDMARSt       LPC17_REG(0x5000C250) /* USB DMA Request Status */
#define USBDMARClr      LPC17_REG(0x5000C254) /* USB DMA Request Clear */
#define USBDMARSet      LPC17_REG(0x5000C258) /* USB DMA Request Set */
#define USBUDCAH        LPC17_REG(0x5000C280) /* USB UDCA Head */
#define USBEpDMASt      LPC17_REG(0x5000C284) /* USB Endpoint DMA Status */
#define USBEpDMAEn      LPC17_REG(0x5000C288) /* USB Endpoint DMA Enable */
#define USBEpDMADis     LPC17_REG(0x5000C28C) /* USB Endpoint DMA Disable */
#define USBDMAIntSt     LPC17_REG(0x5000C290) /* USB DMA Interrupt Status */
#define USBDMAIntEn     LPC17_REG(0x5000C294) /* USB DMA Interrupt Enable */
#define USBEoTIntSt     LPC17_REG(0x5000C2A0) /* USB End of Transfer Interrupt Status */
#define USBEoTIntClr    LPC17_REG(0x5000C2A4) /* USB End of Transfer Interrupt Clear */
#define USBEoTIntSet    LPC17_REG(0x5000C2A8) /* USB End of Transfer Interrupt Set */
#define USBNDDRIntSt    LPC17_REG(0x5000C2AC) /* USB New DD Request Interrupt Status */
#define USBNDDRIntClr   LPC17_REG(0x5000C2B0) /* USB New DD Request Interrupt Clear */
#define USBNDDRIntSet   LPC17_REG(0x5000C2B4) /* USB New DD Request Interrupt Set */
#define USBSysErrIntSt  LPC17_REG(0x5000C2B8) /* USB System Error Interrupt Status */
#define USBSysErrIntClr LPC17_REG(0x5000C2BC) /* USB System Error Interrupt Clear */
#define USBSysErrIntSet LPC17_REG(0x5000C2C0) /* USB System Error Interrupt Set */

/* USBClkCtrl register (USBClkCtrl - 0x5000 CFF4) */
#define USBClkCtrl_DEV_CLK_EN (0x1 << 1)
#define USBClkCtrl_AHB_CLK_EN (0x1 << 4)

/* USB Clock Status register (USBClkSt - 0x5000 CFF8) */
#define USBClkSt_DEV_CLK_ON (0x1 << 1)
#define USBClkSt_AHB_CLK_ON (0x1 << 4)

/* USB Interrupt Status register (USBIntSt - 0x5000 C1C0) */
#define USBIntSt_USB_INT_REQ_LP  (0x1 << 0)
#define USBIntSt_USB_INT_REQ_HP  (0x1 << 1)
#define USBIntSt_USB_INT_REQ_DMA (0x1 << 2)
#define USBIntSt_USB_NEED_CLK    (0x1 << 8)
#define USBIntSt_EN_USB_INTS     (0x1 << 31)

/* USB Device Interrupt Status register (USBDevIntSt - 0x5000 C200) */
#define USBDevIntSt_FRAME    (0x1 << 0)
#define USBDevIntSt_EP_FAST  (0x1 << 1)
#define USBDevIntSt_EP_SLOW  (0x1 << 2)
#define USBDevIntSt_DEV_STAT (0x1 << 3)
#define USBDevIntSt_CCEMPTY  (0x1 << 4)
#define USBDevIntSt_CDFULL   (0x1 << 5)
#define USBDevIntSt_RxENDPKT (0x1 << 6)
#define USBDevIntSt_TxENDPKT (0x1 << 7)
#define USBDevIntSt_EP_RLZED (0x1 << 8)
#define USBDevIntSt_ERR_INT  (0x1 << 9)

/* USB Device Interrupt Enable register (USBDevIntEn - 0x5000 C204) */
#define USBDevIntEn_FRAME    (0x1 << 0)
#define USBDevIntEn_EP_FAST  (0x1 << 1)
#define USBDevIntEn_EP_SLOW  (0x1 << 2)
#define USBDevIntEn_DEV_STAT (0x1 << 3)
#define USBDevIntEn_CCEMPTY  (0x1 << 4)
#define USBDevIntEn_CDFULL   (0x1 << 5)
#define USBDevIntEn_RxENDPKT (0x1 << 6)
#define USBDevIntEn_TxENDPKT (0x1 << 7)
#define USBDevIntEn_EP_RLZED (0x1 << 8)
#define USBDevIntEn_ERR_INT  (0x1 << 9)

/* USB Device Interrupt Clear register (USBDevIntClr - 0x5000 C208) */
#define USBDevIntClr_FRAME    (0x1 << 0)
#define USBDevIntClr_EP_FAST  (0x1 << 1)
#define USBDevIntClr_EP_SLOW  (0x1 << 2)
#define USBDevIntClr_DEV_STAT (0x1 << 3)
#define USBDevIntClr_CCEMPTY  (0x1 << 4)
#define USBDevIntClr_CDFULL   (0x1 << 5)
#define USBDevIntClr_RxENDPKT (0x1 << 6)
#define USBDevIntClr_TxENDPKT (0x1 << 7)
#define USBDevIntClr_EP_RLZED (0x1 << 8)
#define USBDevIntClr_ERR_INT  (0x1 << 9)

/* USB Device Interrupt Set register (USBDevIntSet - 0x5000 C20C) */
#define USBDevIntSet_FRAME    (0x1 << 0)
#define USBDevIntSet_EP_FAST  (0x1 << 1)
#define USBDevIntSet_EP_SLOW  (0x1 << 2)
#define USBDevIntSet_DEV_STAT (0x1 << 3)
#define USBDevIntSet_CCEMPTY  (0x1 << 4)
#define USBDevIntSet_CDFULL   (0x1 << 5)
#define USBDevIntSet_RxENDPKT (0x1 << 6)
#define USBDevIntSet_TxENDPKT (0x1 << 7)
#define USBDevIntSet_EP_RLZED (0x1 << 8)
#define USBDevIntSet_ERR_INT  (0x1 << 9)

/* USB Device Interrupt Priority register (USBDevIntPri - 0x5000 C22C) */
#define USBDevIntPri_FRAME   (0x1 << 0)
#define USBDevIntPri_EP_FAST (0x1 << 1)

/* USB Endpoint Interrupt Status register (USBEpIntSt - 0x5000 C230) */
#define USBEpIntSt_EP0RX  (0x1 << 0)
#define USBEpIntSt_EP0TX  (0x1 << 1)
#define USBEpIntSt_EP1RX  (0x1 << 2)
#define USBEpIntSt_EP1TX  (0x1 << 3)
#define USBEpIntSt_EP2RX  (0x1 << 4)
#define USBEpIntSt_EP2TX  (0x1 << 5)
#define USBEpIntSt_EP3RX  (0x1 << 6)
#define USBEpIntSt_EP3TX  (0x1 << 7)
#define USBEpIntSt_EP4RX  (0x1 << 8)
#define USBEpIntSt_EP4TX  (0x1 << 9)
#define USBEpIntSt_EP5RX  (0x1 << 10)
#define USBEpIntSt_EP5TX  (0x1 << 11)
#define USBEpIntSt_EP6RX  (0x1 << 12)
#define USBEpIntSt_EP6TX  (0x1 << 13)
#define USBEpIntSt_EP7RX  (0x1 << 14)
#define USBEpIntSt_EP7TX  (0x1 << 15)
#define USBEpIntSt_EP8RX  (0x1 << 16)
#define USBEpIntSt_EP8TX  (0x1 << 17)
#define USBEpIntSt_EP9RX  (0x1 << 18)
#define USBEpIntSt_EP9TX  (0x1 << 19)
#define USBEpIntSt_EP10RX (0x1 << 20)
#define USBEpIntSt_EP10TX (0x1 << 21)
#define USBEpIntSt_EP11RX (0x1 << 22)
#define USBEpIntSt_EP11TX (0x1 << 23)
#define USBEpIntSt_EP12RX (0x1 << 24)
#define USBEpIntSt_EP12TX (0x1 << 25)
#define USBEpIntSt_EP13RX (0x1 << 26)
#define USBEpIntSt_EP13TX (0x1 << 27)
#define USBEpIntSt_EP14RX (0x1 << 28)
#define USBEpIntSt_EP14TX (0x1 << 29)
#define USBEpIntSt_EP15RX (0x1 << 30)
#define USBEpIntSt_EP15TX (0x1 << 31)

/* USB Endpoint Interrupt Enable register (USBEpIntEn - 0x5000 C234) */
#define USBEpIntEn_EP0RX  (0x1 << 0)
#define USBEpIntEn_EP0TX  (0x1 << 1)
#define USBEpIntEn_EP1RX  (0x1 << 2)
#define USBEpIntEn_EP1TX  (0x1 << 3)
#define USBEpIntEn_EP2RX  (0x1 << 4)
#define USBEpIntEn_EP2TX  (0x1 << 5)
#define USBEpIntEn_EP3RX  (0x1 << 6)
#define USBEpIntEn_EP3TX  (0x1 << 7)
#define USBEpIntEn_EP4RX  (0x1 << 8)
#define USBEpIntEn_EP4TX  (0x1 << 9)
#define USBEpIntEn_EP5RX  (0x1 << 10)
#define USBEpIntEn_EP5TX  (0x1 << 11)
#define USBEpIntEn_EP6RX  (0x1 << 12)
#define USBEpIntEn_EP6TX  (0x1 << 13)
#define USBEpIntEn_EP7RX  (0x1 << 14)
#define USBEpIntEn_EP7TX  (0x1 << 15)
#define USBEpIntEn_EP8RX  (0x1 << 16)
#define USBEpIntEn_EP8TX  (0x1 << 17)
#define USBEpIntEn_EP9RX  (0x1 << 18)
#define USBEpIntEn_EP9TX  (0x1 << 19)
#define USBEpIntEn_EP10RX (0x1 << 20)
#define USBEpIntEn_EP10TX (0x1 << 21)
#define USBEpIntEn_EP11RX (0x1 << 22)
#define USBEpIntEn_EP11TX (0x1 << 23)
#define USBEpIntEn_EP12RX (0x1 << 24)
#define USBEpIntEn_EP12TX (0x1 << 25)
#define USBEpIntEn_EP13RX (0x1 << 26)
#define USBEpIntEn_EP13TX (0x1 << 27)
#define USBEpIntEn_EP14RX (0x1 << 28)
#define USBEpIntEn_EP14TX (0x1 << 29)
#define USBEpIntEn_EP15RX (0x1 << 30)
#define USBEpIntEn_EP15TX (0x1 << 31)

/* USB Endpoint Interrupt Clear register (USBEpIntClr - 0x5000 C238) */
#define USBEpIntClr_EP0RX  (0x1 << 0)
#define USBEpIntClr_EP0TX  (0x1 << 1)
#define USBEpIntClr_EP1RX  (0x1 << 2)
#define USBEpIntClr_EP1TX  (0x1 << 3)
#define USBEpIntClr_EP2RX  (0x1 << 4)
#define USBEpIntClr_EP2TX  (0x1 << 5)
#define USBEpIntClr_EP3RX  (0x1 << 6)
#define USBEpIntClr_EP3TX  (0x1 << 7)
#define USBEpIntClr_EP4RX  (0x1 << 8)
#define USBEpIntClr_EP4TX  (0x1 << 9)
#define USBEpIntClr_EP5RX  (0x1 << 10)
#define USBEpIntClr_EP5TX  (0x1 << 11)
#define USBEpIntClr_EP6RX  (0x1 << 12)
#define USBEpIntClr_EP6TX  (0x1 << 13)
#define USBEpIntClr_EP7RX  (0x1 << 14)
#define USBEpIntClr_EP7TX  (0x1 << 15)
#define USBEpIntClr_EP8RX  (0x1 << 16)
#define USBEpIntClr_EP8TX  (0x1 << 17)
#define USBEpIntClr_EP9RX  (0x1 << 18)
#define USBEpIntClr_EP9TX  (0x1 << 19)
#define USBEpIntClr_EP10RX (0x1 << 20)
#define USBEpIntClr_EP10TX (0x1 << 21)
#define USBEpIntClr_EP11RX (0x1 << 22)
#define USBEpIntClr_EP11TX (0x1 << 23)
#define USBEpIntClr_EP12RX (0x1 << 24)
#define USBEpIntClr_EP12TX (0x1 << 25)
#define USBEpIntClr_EP13RX (0x1 << 26)
#define USBEpIntClr_EP13TX (0x1 << 27)
#define USBEpIntClr_EP14RX (0x1 << 28)
#define USBEpIntClr_EP14TX (0x1 << 29)
#define USBEpIntClr_EP15RX (0x1 << 30)
#define USBEpIntClr_EP15TX (0x1 << 31)

/* USB Endpoint Interrupt Set register (USBEpIntSet - 0x5000 C23C) */
#define USBEpIntSet_EP0RX  (0x1 << 0)
#define USBEpIntSet_EP0TX  (0x1 << 1)
#define USBEpIntSet_EP1RX  (0x1 << 2)
#define USBEpIntSet_EP1TX  (0x1 << 3)
#define USBEpIntSet_EP2RX  (0x1 << 4)
#define USBEpIntSet_EP2TX  (0x1 << 5)
#define USBEpIntSet_EP3RX  (0x1 << 6)
#define USBEpIntSet_EP3TX  (0x1 << 7)
#define USBEpIntSet_EP4RX  (0x1 << 8)
#define USBEpIntSet_EP4TX  (0x1 << 9)
#define USBEpIntSet_EP5RX  (0x1 << 10)
#define USBEpIntSet_EP5TX  (0x1 << 11)
#define USBEpIntSet_EP6RX  (0x1 << 12)
#define USBEpIntSet_EP6TX  (0x1 << 13)
#define USBEpIntSet_EP7RX  (0x1 << 14)
#define USBEpIntSet_EP7TX  (0x1 << 15)
#define USBEpIntSet_EP8RX  (0x1 << 16)
#define USBEpIntSet_EP8TX  (0x1 << 17)
#define USBEpIntSet_EP9RX  (0x1 << 18)
#define USBEpIntSet_EP9TX  (0x1 << 19)
#define USBEpIntSet_EP10RX (0x1 << 20)
#define USBEpIntSet_EP10TX (0x1 << 21)
#define USBEpIntSet_EP11RX (0x1 << 22)
#define USBEpIntSet_EP11TX (0x1 << 23)
#define USBEpIntSet_EP12RX (0x1 << 24)
#define USBEpIntSet_EP12TX (0x1 << 25)
#define USBEpIntSet_EP13RX (0x1 << 26)
#define USBEpIntSet_EP13TX (0x1 << 27)
#define USBEpIntSet_EP14RX (0x1 << 28)
#define USBEpIntSet_EP14TX (0x1 << 29)
#define USBEpIntSet_EP15RX (0x1 << 30)
#define USBEpIntSet_EP15TX (0x1 << 31)

/* USB Endpoint Interrupt Priority register (USBEpIntPri - 0x5000 C240) */
#define USBEpIntPri_EP0RX  (0x1 << 0)
#define USBEpIntPri_EP0TX  (0x1 << 1)
#define USBEpIntPri_EP1RX  (0x1 << 2)
#define USBEpIntPri_EP1TX  (0x1 << 3)
#define USBEpIntPri_EP2RX  (0x1 << 4)
#define USBEpIntPri_EP2TX  (0x1 << 5)
#define USBEpIntPri_EP3RX  (0x1 << 6)
#define USBEpIntPri_EP3TX  (0x1 << 7)
#define USBEpIntPri_EP4RX  (0x1 << 8)
#define USBEpIntPri_EP4TX  (0x1 << 9)
#define USBEpIntPri_EP5RX  (0x1 << 10)
#define USBEpIntPri_EP5TX  (0x1 << 11)
#define USBEpIntPri_EP6RX  (0x1 << 12)
#define USBEpIntPri_EP6TX  (0x1 << 13)
#define USBEpIntPri_EP7RX  (0x1 << 14)
#define USBEpIntPri_EP7TX  (0x1 << 15)
#define USBEpIntPri_EP8RX  (0x1 << 16)
#define USBEpIntPri_EP8TX  (0x1 << 17)
#define USBEpIntPri_EP9RX  (0x1 << 18)
#define USBEpIntPri_EP9TX  (0x1 << 19)
#define USBEpIntPri_EP10RX (0x1 << 20)
#define USBEpIntPri_EP10TX (0x1 << 21)
#define USBEpIntPri_EP11RX (0x1 << 22)
#define USBEpIntPri_EP11TX (0x1 << 23)
#define USBEpIntPri_EP12RX (0x1 << 24)
#define USBEpIntPri_EP12TX (0x1 << 25)
#define USBEpIntPri_EP13RX (0x1 << 26)
#define USBEpIntPri_EP13TX (0x1 << 27)
#define USBEpIntPri_EP14RX (0x1 << 28)
#define USBEpIntPri_EP14TX (0x1 << 29)
#define USBEpIntPri_EP15RX (0x1 << 30)
#define USBEpIntPri_EP15TX (0x1 << 31)

/* skipped USB Realize Endpoint register (USBReEp - 0x5000 C244) */

/* USB Endpoint Index register (USBEpIn - 0x5000 C248) */
#define USBEpIn_PHY_EP (0x1F << 0)

/*
 * skipped:
 *
 * USB MaxPacketSize register (USBMaxPSize - 0x5000 C24C)
 * USB Receive Data register (USBRxData - 0x5000 C218)
 */

/* USB Receive Packet Length register (USBRxPlen - 0x5000 C220) */
#define USBRxPLen_PKT_LNGTH (0x3FF << 0)
#define USBRxPLen_DV        (0x1 << 10)
#define USBRxPLen_PKT_RDY   (0x1 << 11)

/*
 * skipped:
 *
 * USB Transmit Data register (USBTxData - 0x5000 C21C)
 * USB Transmit Packet Length register (USBTxPLen - 0x5000 C224)
 */

/* USB Control register (USBCtrl - 0x5000 C228) */
#define USBCtrl_RD_EN        (0x1 << 0)
#define USBCtrl_WR_EN        (0x1 << 1)
#define USBCtrl_LOG_ENDPOINT (0xF << 2)

/* USB Command Code register (USBCmdCode - 0x5000 C210) */
#define USBCmdCode_CMD_PHASE          (0xFF << 8)
#define USBCmdCode_CMD_CODE_CMD_WDATA (0xFF << 16)

/*
 * skipped:
 *
 * USB Command Data register (USBCmdData - 0x5000 C214)
 * USB DMA Request Status register (USBDMARSt - 0x5000 C250)
 * USB DMA Request Clear register (USBDMARClr - 0x5000 C254)
 * USB DMA Request Set register (USBDMARSet - 0x5000 C258)
 * USB UDCA Head register (USBUDCAH - 0x5000 C280)
 * USB EP DMA Status register (USBEpDMASt - 0x5000 C284)
 * USB EP DMA Enable register (USBEpDMAEn - 0x5000 C288)
 * USB EP DMA Disable register (USBEpDMADis - 0x5000 C28C)
 */

/* USB DMA Interrupt Status register (USBDMAIntSt - 0x5000 C290) */
#define USBDMAIntSt_EOT  (0x1 << 0)
#define USBDMAIntSt_NDDR (0x1 << 1)
#define USBDMAIntSt_ERR  (0x1 << 2)

/* USB DMA Interrupt Enable register (USBDMAIntEn - 0x5000 C294) */
#define USBDMAIntEn_EOT  (0x1 << 0)
#define USBDMAIntEn_NDDR (0x1 << 1)
#define USBDMAIntEn_ERR  (0x1 << 2)

/*
 * skipped:
 *
 * USB End of Transfer Interrupt Status register (USBEoTIntSt - 0x5000 C2A0s)
 * USB End of Transfer Interrupt Clear register (USBEoTIntClr - 0x5000 C2A4)
 * USB End of Transfer Interrupt Set register (USBEoTIntSet - 0x5000 C2A8)
 * USB New DD Request Interrupt Status register (USBNDDRIntSt - 0x5000 C2AC)
 * USB New DD Request Interrupt Clear register (USBNDDRIntClr - 0x5000 C2B0)
 * USB New DD Request Interrupt Set register (USBNDDRIntSet - 0x5000 C2B4)
 * USB System Error Interrupt Status register (USBSysErrIntSt - 0x5000 C2B8)
 * USB System Error Interrupt Clear register (USBSysErrIntClr - 0x5000 C2BC)
 * USB System Error Interrupt Set register (USBSysErrIntSet - 0x5000 C2C0)
 */


/* USB host registers */
#define HcRevision           LPC17_REG(0x5000C000)
#define HcControl            LPC17_REG(0x5000C004)
#define HcCommandStatus      LPC17_REG(0x5000C008)
#define HcInterruptStatus    LPC17_REG(0x5000C00C)
#define HcInterruptEnable    LPC17_REG(0x5000C010)
#define HcInterruptDisable   LPC17_REG(0x5000C014)
#define HcHCCA               LPC17_REG(0x5000C018)
#define HcPeriodCurrentED    LPC17_REG(0x5000C01C)
#define HcControlHeadED      LPC17_REG(0x5000C020)
#define HcControlCurrentED   LPC17_REG(0x5000C024)
#define HcBulkHeadED         LPC17_REG(0x5000C028)
#define HcBulkCurrentED      LPC17_REG(0x5000C02C)
#define HcDoneHead           LPC17_REG(0x5000C030)
#define HcFmInterval         LPC17_REG(0x5000C034)
#define HcFmRemaining        LPC17_REG(0x5000C038)
#define HcFmNumber           LPC17_REG(0x5000C03C)
#define HcPeriodicStart      LPC17_REG(0x5000C040)
#define HcLSThreshold        LPC17_REG(0x5000C044)
#define HcRhDescriptorA      LPC17_REG(0x5000C048)
#define HcRhDescriptorB      LPC17_REG(0x5000C04C)
#define HcRhStatus           LPC17_REG(0x5000C050)
#define HcRhPortStatus1      LPC17_REG(0x5000C054)
#define HcRhPortStatus2      LPC17_REG(0x5000C058)
#define Module_ID_Ver_Rev_ID LPC17_REG(0x5000C0FC)


/* USB OTG and I2C registers */

/* Interrupt register */
#define USBIntSt   LPC17_REG(0x400FC1C0) /* USB Interrupt Status */

/* OTG registers */
#define OTGIntSt   LPC17_REG(0x5000C100) /* OTG Interrupt Status */
#define OTGIntEn   LPC17_REG(0x5000C104) /* OTG Interrupt Enable */
#define OTGIntSet  LPC17_REG(0x5000C108) /* OTG Interrupt Set */
#define OTGIntClr  LPC17_REG(0x5000C10C) /* OTG Interrupt Clear */
#define OTGStCtrl  LPC17_REG(0x5000C110) /* OTG Status and Control */
#define OTGTmr     LPC17_REG(0x5000C114) /* OTG Timer */

/* I2C registers */
#define I2C_RX     LPC17_REG(0x5000C300) /* I2C Receive */
#define I2C_TX     LPC17_REG(0x5000C300) /* I2C Transmit */
#define I2C_STS    LPC17_REG(0x5000C304) /* I2C Status */
#define I2C_CTL    LPC17_REG(0x5000C308) /* I2C Control */
#define I2C_CLKHI  LPC17_REG(0x5000C30C) /* I2C Clock High */
#define I2C_CLKLO  LPC17_REG(0x5000C310) /* I2C Clock Low */

/* Clock control registers */
#define OTGClkCtrl LPC17_REG(0x5000CFF4) /* OTG clock controller */
#define OTGClkSt   LPC17_REG(0x5000CFF8) /* OTG clock status */

/* USB Interrupt Status register - (USBIntSt - 0x5000 C1C0) */
#define USBIntSt_USB_INT_REQ_LP  (0x1 << 0)
#define USBIntSt_USB_INT_REQ_HP  (0x1 << 1)
#define USBIntSt_USB_INT_REQ_DMA (0x1 << 2)
#define USBIntSt_USB_HOST_INT    (0x1 << 3)
#define USBIntSt_USB_ATX_INT     (0x1 << 4)
#define USBIntSt_USB_OTG_INT     (0x1 << 5)
#define USBIntSt_USB_I2C_INT     (0x1 << 6)
#define USBIntSt_USB_NEED_CLK    (0x1 << 8)
#define USBIntSt_EN_USB_INTS     (0x1 << 31)

/* OTG Interrupt Status register (OTGIntSt - 0x5000 C100) */
#define OTGIntSt_TMR         (0x1 << 0)
#define OTGIntSt_REMOVE_PU   (0x1 << 1)
#define OTGIntSt_HNP_FAILURE (0x1 << 2)
#define OTGIntSt_HNP_SUCCESS (0x1 << 3)

/* OTG Interrupt Enable Register (OTGIntEn - 0x5000 C104) */
#define OTGIntEn_TMR         (0x1 << 0)
#define OTGIntEn_REMOVE_PU   (0x1 << 1)
#define OTGIntEn_HNP_FAILURE (0x1 << 2)
#define OTGIntEn_HNP_SUCCESS (0x1 << 3)

/* OTG Interrupt Set Register (OTGIntSet - 0x5000 C20C) */
#define OTGIntSet_TMR         (0x1 << 0)
#define OTGIntSet_REMOVE_PU   (0x1 << 1)
#define OTGIntSet_HNP_FAILURE (0x1 << 2)
#define OTGIntSet_HNP_SUCCESS (0x1 << 3)

/* OTG Interrupt Clear Register (OTGIntClr - 0x5000 C10C) */
#define OTGIntClr_TMR         (0x1 << 0)
#define OTGIntClr_REMOVE_PU   (0x1 << 1)
#define OTGIntClr_HNP_FAILURE (0x1 << 2)
#define OTGIntClr_HNP_SUCCESS (0x1 << 3)

/* OTG Status and Control Register (OTGStCtrl - 0x5000 C110) */
#define OTGStCtrl_PORT_FUNC   (0x3 << 0)
#define OTGStCtrl_TMR_SCALE   (0x3 << 2)
#define OTGStCtrl_TMR_MODE    (0x1 << 4)
#define OTGStCtrl_TMR_EN      (0x1 << 5)
#define OTGStCtrl_TMR_RST     (0x1 << 6)
#define OTGStCtrl_B_HNP_TRACK (0x1 << 8)
#define OTGStCtrl_A_HNP_TRACK (0x1 << 9)
#define OTGStCtrl_PU_REMOVED  (0x1 << 10)
#define OTGStCtrl_TMR_CNT     (0xFFFF << 16)

/* OTG Timer Register (OTGTmr - 0x5000 C114) */
#define OTGTmr_TIMEOUT_CNT (0xFFFF << 0)

/* OTG Clock Control Register (OTGClkCtrl - 0x5000 CFF4) */
#define OTGClkCtrl_HOST_CLK_EN (0x1 << 0)
#define OTGClkCtrl_DEV_CLK_EN  (0x1 << 1)
#define OTGClkCtrl_I2C_CLK_EN  (0x1 << 2)
#define OTGClkCtrl_OTG_CLK_EN  (0x1 << 3)
#define OTGClkCtrl_AHB_CLK_EN  (0x1 << 4)

/* OTG Clock Status Register (OTGClkSt - 0x5000 CFF8) */
#define OTGClkSt_HOST_CLK_ON (0x1 << 0)
#define OTGClkSt_DEV_CLK_ON  (0x1 << 1)
#define OTGClkSt_I2C_CLK_ON  (0x1 << 2)
#define OTGClkSt_OTG_CLK_ON  (0x1 << 3)
#define OTGClkSt_AHB_CLK_ON  (0x1 << 4)

/* skipped I2C Receive Register (I2C_RX - 0x5000 C300) */

/* I2C Transmit register (I2C_TX - 0x5000 C300)  */
#define I2C_TX_TX_Data (0xFF << 0)
#define I2C_TX_START   (0x1 << 8)
#define I2C_TX_STOP    (0x1 << 9)

/* I2C Status Register (I2C_STS - 0x5000 C304) */
#define I2C_STS_TDI   (0x1 << 0)
#define I2C_STS_AFI   (0x1 << 1)
#define I2C_STS_NAI   (0x1 << 2)
#define I2C_STS_DRM   (0x1 << 3)
#define I2C_STS_DRSI  (0x1 << 4)
#define I2C_STS_Activ (0x1 << 5)
#define I2C_STS_SCL   (0x1 << 6)
#define I2C_STS_SDA   (0x1 << 7)
#define I2C_STS_RFF   (0x1 << 8)
#define I2C_STS_RFE   (0x1 << 9)
#define I2C_STS_TFF   (0x1 << 10)
#define I2C_STS_TFE   (0x1 << 11)

/* I2C Control Register (I2C_CTL - 0x5000 C308) */
#define I2C_CTL_TDIE   (0x1 << 0)
#define I2C_CTL_AFIE   (0x1 << 1)
#define I2C_CTL_NAIE   (0x1 << 2)
#define I2C_CTL_DRMIE  (0x1 << 3)
#define I2C_CTL_DRSIE  (0x1 << 4)
#define I2C_CTL_REFIE  (0x1 << 5)
#define I2C_CTL_RFDAIE (0x1 << 6)
#define I2C_CTL_TFFIE  (0x1 << 7)
#define I2C_CTL_SRST   (0x1 << 8)

/*
 * skipped:
 *
 * I2C Clock High Register (I2C_CLKHI - 0x5000 C30C)
 * I2C Clock Low Register (I2C_CLKLO - 0x5000 C310)5
 */


/* UART 0/2/3 registers */

/* UART 0 */
#define U0RBR LPC17_REG(0x4000C000) /* Receiver Buffer Register */
#define U0THR LPC17_REG(0x4000C000) /* Transmit Holding Register */
#define U0DLL LPC17_REG(0x4000C000) /* Divisor Latch LSB */
#define U0DLM LPC17_REG(0x4000C004) /* Divisor Latch MSB */
#define U0IER LPC17_REG(0x4000C004) /* Interrupt Enable Register */
#define U0IIR LPC17_REG(0x4000C008) /* Interrupt ID Register */
#define U0FCR LPC17_REG(0x4000C008) /* FIFO Control Register */
#define U0LCR LPC17_REG(0x4000C00C) /* Line Control Register */
#define U0LSR LPC17_REG(0x4000C014) /* Line Status Register */
#define U0SCR LPC17_REG(0x4000C01C) /* Scratch Pad Register */
#define U0ACR LPC17_REG(0x4000C020) /* Auto-baud Control Register */
#define U0ICR LPC17_REG(0x4000C024) /* IrDA Control Register */
#define U0FDR LPC17_REG(0x4000C028) /* Fractional Divider Register */
#define U0TER LPC17_REG(0x4000C030) /* Transmit Enable Register */

/* UART 2 */
#define U2RBR LPC17_REG(0x40098000) /* Receiver Buffer Register */
#define U2THR LPC17_REG(0x40098000) /* Transmit Holding Register */
#define U2DLL LPC17_REG(0x40098000) /* Divisor Latch LSB */
#define U2DLM LPC17_REG(0x40098004) /* Divisor Latch MSB */
#define U2IER LPC17_REG(0x40098004) /* Interrupt Enable Register */
#define U2IIR LPC17_REG(0x40098008) /* Interrupt ID Register */
#define U2FCR LPC17_REG(0x40098008) /* FIFO Control Register */
#define U2LCR LPC17_REG(0x4009800C) /* Line Control Register */
#define U2LSR LPC17_REG(0x40098014) /* Line Status Register */
#define U2SCR LPC17_REG(0x4009801C) /* Scratch Pad Register */
#define U2ACR LPC17_REG(0x40098020) /* Auto-baud Control Register */
#define U2ICR LPC17_REG(0x40098024) /* IrDA Control Register */
#define U2FDR LPC17_REG(0x40098028) /* Fractional Divider Register */
#define U2TER LPC17_REG(0x40098030) /* Transmit Enable Register */

/* UART 3 */
#define U3RBR LPC17_REG(0x4009C000) /* Receiver Buffer Register */
#define U3THR LPC17_REG(0x4009C000) /* Transmit Holding Register */
#define U3DLL LPC17_REG(0x4009C000) /* Divisor Latch LSB */
#define U3DLM LPC17_REG(0x4009C004) /* Divisor Latch MSB */
#define U3IER LPC17_REG(0x4009C004) /* Interrupt Enable Register */
#define U3IIR LPC17_REG(0x4009C008) /* Interrupt ID Register */
#define U3FCR LPC17_REG(0x4009C008) /* FIFO Control Register */
#define U3LCR LPC17_REG(0x4009C00C) /* Line Control Register */
#define U3LSR LPC17_REG(0x4009C014) /* Line Status Register */
#define U3SCR LPC17_REG(0x4009C01C) /* Scratch Pad Register */
#define U3ACR LPC17_REG(0x4009C020) /* Auto-baud Control Register */
#define U3ICR LPC17_REG(0x4009C024) /* IrDA Control Register */
#define U3FDR LPC17_REG(0x4009C028) /* Fractional Divider Register */
#define U3TER LPC17_REG(0x4009C030) /* Transmit Enable Register */

/*
 * skipped:
 *
 * UARTn Receiver Buffer Register (U0RBR - 0x4000 C000, U2RBR - 0x4009 8000, U3RBR - 0x4009 C000)
 * UARTn Transmit Holding Register (U0THR - 0x4000 C000, U2THR - 0x4009 8000, U3THR - 0x4009 C000)
 * UARTn Divisor Latch LSB register (U0DLL - 0x4000 C000, U2DLL - 0x4009 8000, U3DLL - 0x4009 C000)
 * UARTn Divisor Latch MSB register (U0DLM - 0x4000 C004, U2DLL - 0x4009 8004, U3DLL - 0x4009 C004)
 */

/* UARTn Interrupt Enable Register (U0IER - 0x4000 C004, U2IER - 0x4009 8004, U3IER - 0x4009 C004) */
#define UIER_RBR_Interrupt_Enable            (0x1 << 0)
#define UIER_THRE_Interrupt_Enable           (0x1 << 1)
#define UIER_RX_Line_Status_Interrupt_Enable (0x1 << 2)
#define UIER_ABEOIntEn                       (0x1 << 8)
#define UIER_ABTOIntEn                       (0x1 << 9)

/* UARTn Interrupt Identification Register (U0IIR - 0x4000 C008, U2IIR - 0x4009 8008, U3IIR - 0x4009 C008) */
#define UIIR_IntStatus   (0x1 << 0)
#define UIIR_IntId       (0x7 << 1)
#define UIIR_FIFO Enable (0x3 << 6)
#define UIIR_ABEOInt     (0x1 << 8)
#define UIIR_ABTOInt     (0x1 << 9)

/* UARTn FIFO Control Register (U0FCR - 0x4000 C008, U2FCR - 0x4009 8008, U3FCR - 0x4009 C008) */
#define UFCR_FIFO_Enable      (0x1 << 0)
#define UFCR_RX_FIFO_Reset    (0x1 << 1)
#define UFCR_TX_FIFO_Reset    (0x1 << 2)
#define UFCR_DMA_Mode_Select  (0x1 << 3)
#define UFCR_RX_Trigger_Level (0x3 << 6)

/* UARTn Line Control Register (U0LCR - 0x4000 C00C, U2LCR - 0x4009 800C, U3LCR - 0x4009 C00C) */
#define ULCR_Word_Length_Select (0x3 << 0)
#define ULCR_Stop_Bit_Select    (0x1 << 2)
#define ULCR_Parity_Enable      (0x1 << 3)
#define ULCR_Parity_Select      (0x3 << 4)
#define ULCR_Break_Control      (0x1 << 6)
#define ULCR_DLAB               (0x1 << 7)

/* UARTn Line Status Register (U0LSR - 0x4000 C014, U2LSR - 0x4009 8014, U3LSR - 0x4009 C014) */
#define ULSR_RDR  (0x1 << 0)
#define ULSR_OE   (0x1 << 1)
#define ULSR_PE   (0x1 << 2)
#define ULSR_FE   (0x1 << 3)
#define ULSR_BI   (0x1 << 4)
#define ULSR_THRE (0x1 << 5)
#define ULSR_TEMT (0x1 << 6)
#define ULSR_RXFE (0x1 << 7)

/* skipped UARTn Scratch Pad Register (U0SCR - 0x4000 C01C, U2SCR - 0x4009 801C U3SCR - 0x4009 C01C) */

/* UARTn Auto-baud Control Register (U0ACR - 0x4000 C020, U2ACR - 0x4009 8020, U3ACR - 0x4009 C020) */
#define UACR_Start       (0x1 << 0)
#define UACR_Mode        (0x1 << 1)
#define UACR_AutoRestart (0x1 << 2)
#define UACR_ABEOIntClr  (0x1 << 8)
#define UACR_ABTOIntClr  (0x1 << 9)

/* UARTn IrDA Control Register (U0ICR - 0x4000 C024, U2ICR - 0x4009 8024, U3ICR - 0x4009 C024) */
#define UICR_IrDAEn     (0x1 << 0)
#define UICR_IrDAInv    (0x1 << 1)
#define UICR_FixPulseEn (0x1 << 2)
#define UICR_PulseDiv   (0x7 << 3)

/* UARTn Fractional Divider Register (U0FDR - 0x4000 C028, U2FDR - 0x4009 8028, U3FDR - 0x4009 C028) */
#define UFDR_DIVADDVAL (0xF << 0)
#define UFDR_MULVAL    (0xF << 4)

/* UARTn Transmit Enable Register (U0TER - 0x4000 C030, U2TER - 0x4009 8030, U3TER - 0x4009 C030) */
#define UTER_TXEN (0x1 << 7)

/* UART 1 registers */

#define U1RBR       LPC17_REG(0x40010000) /* Receiver Buffer Register */
#define U1THR       LPC17_REG(0x40010000) /* Transmit Holding Register */
#define U1DLL       LPC17_REG(0x40010000) /* Divisor Latch LSB */
#define U1DLM       LPC17_REG(0x40010004) /* Divisor Latch MSB */
#define U1IER       LPC17_REG(0x40010004) /* Interrupt Enable Register */
#define U1IIR       LPC17_REG(0x40010008) /* Interrupt ID Register */
#define U1FCR       LPC17_REG(0x40010008) /* FIFO Control Register */
#define U1LCR       LPC17_REG(0x4001000C) /* Line Control Register */
#define U1MCR       LPC17_REG(0x40010010) /* Modem Control Register */
#define U1LSR       LPC17_REG(0x40010014) /* Line Status Register */
#define U1MSR       LPC17_REG(0x40010018) /* Modem Status Register */
#define U1SCR       LPC17_REG(0x4001001C) /* Scratch Pad Register */
#define U1ACR       LPC17_REG(0x40010020) /* Auto-baud Control Register */
#define U1FDR       LPC17_REG(0x40010028) /* Fractional Divider Register */
#define U1TER       LPC17_REG(0x40010030) /* Transmit Enable Register */
#define U1RS485CTRL LPC17_REG(0x4001004C) /* RS-485/EIA-485 Control */
#define U1ADRMATCH  LPC17_REG(0x40010050) /* RS-485/EIA-485 address match */
#define U1RS485DLY  LPC17_REG(0x40010054) /* RS-485/EIA-485 direction control delay */

/*
 * skipped:
 *
 * UART1 Receiver Buffer Register (U1RBR - 0x4001 0000)
 * UART1 Transmitter Holding Register (U1THR - 0x4001 0000)
 * UART1 Divisor Latch LSB Register (U1DLL - 0x4001 0000)
 * UART1 Divisor Latch MSB Register (U1DLM - 0x4001 0004)
 */

/* UART1 Interrupt Enable Register (U1IER - 0x4001 0004) */
#define U1IER_RBR          (0x1 << 0)
#define U1IER_THRE         (0x1 << 1)
#define U1IER_RX_Line      (0x1 << 2)
#define U1IER_Modem_Status (0x1 << 3)
#define U1IER_CTS          (0x1 << 7)
#define U1IER_ABEOIntEn    (0x1 << 8)
#define U1IER_ABTOIntEn    (0x1 << 9)

/* UART1 Interrupt Identification Register (U1IIR - 0x4001 0008) */
#define U1IIR_IntStatus   (0x1 << 0)
#define U1IIR_IntId       (0x7 << 1)
#define U1IIR_FIFO_Enable (0x3 << 6)
#define U1IIR_ABEOInt     (0x1 << 8)
#define U1IIR_ABTOInt     (0x1 << 9)

/* UART1 FIFO Control Register (U1FCR - 0x4001 0008) */
#define U1FCR_FIFO_Enable      (0x1 << 0)
#define U1FCR_RX_FIFO_Reset    (0x1 << 1)
#define U1FCR_TX_FIFO_Reset    (0x1 << 2)
#define U1FCR_DMA_Mode_Select  (0x1 << 3)
#define U1FCR_RX_Trigger_Level (0x3 << 6)

/* UART1 Line Control Register (U1LCR - 0x4001 000C) */
#define U1LCR_Word_Length_Select (0x3 << 0)
#define U1LCR_Stop_Bit_Select    (0x1 << 2)
#define U1LCR_Parity_Enable      (0x1 << 3)
#define U1LCR_Parity_Select      (0x3 << 4)
#define U1LCR_Break_Control      (0x1 << 6)
#define U1LCR_DLAB               (0x1 << 7)

/* UART1 Modem Control Register (U1MCR - 0x4001 0010) */
#define U1MCR_DTR_Control          (0x1 << 0)
#define U1MCR_RTS_Control          (0x1 << 1)
#define U1MCR_Loopback_Mode_Select (0x1 << 4)
#define U1MCR_RTSen                (0x1 << 6)
#define U1MCR_CTSen                (0x1 << 7)

/* UART1 Line Status Register (U1LSR - 0x4001 0014) */
#define U1LSR_RDR  (0x1 << 0)
#define U1LSR_OE   (0x1 << 1)
#define U1LSR_PE   (0x1 << 2)
#define U1LSR_FE   (0x1 << 3)
#define U1LSR_BI   (0x1 << 4)
#define U1LSR_THRE (0x1 << 5)
#define U1LSR_TEMT (0x1 << 6)
#define U1LSR_RXFE (0x1 << 7)

/* UART1 Modem Status Register (U1MSR - 0x4001 0018) */
#define U1MSR_Delta_CTS        (0x1 << 0)
#define U1MSR_Delta_DSR        (0x1 << 1)
#define U1MSR_Trailing_Edge_RI (0x1 << 2)
#define U1MSR_Delta_DCD        (0x1 << 3)
#define U1MSR_CTS              (0x1 << 4)
#define U1MSR_DSR              (0x1 << 5)
#define U1MSR_RI               (0x1 << 6)
#define U1MSR_DCD              (0x1 << 7)

/* skipped UART1 Scratch Pad Register (U1SCR - 0x4001 001C) */

/* UART1 Auto-baud Control Register (U1ACR - 0x4001 0020) */
#define U1ACR_Start       (0x1 << 0)
#define U1ACR_Mode        (0x1 << 1)
#define U1ACR_AutoRestart (0x1 << 2)
#define U1ACR_ABEOIntClr  (0x1 << 8)
#define U1ACR_ABTOIntClr  (0x1 << 9)

/* UART1 Fractional Divider Register (U1FDR - 0x4001 0028) */
#define U1FDR_DIVADDVAL (0xF << 0)
#define U1FDR_MULVAL    (0xF << 4)

/* UART1 Transmit Enable Register (U1TER - 0x4001 0030) */
#define U1TER_TXEN (0x1 << 7)

/* UART1 RS485 Control register (U1RS485CTRL - 0x4001 004C) */
#define U1RS485CTRL_NMMEN (0x1 << 0)
#define U1RS485CTRL_RXDIS (0x1 << 1)
#define U1RS485CTRL_AADEN (0x1 << 2)
#define U1RS485CTRL_SEL   (0x1 << 3)
#define U1RS485CTRL_DCTRL (0x1 << 4)
#define U1RS485CTRL_OINV  (0x1 << 5)

/*
 * skipped:
 *
 * UART1 RS-485 Address Match register (U1RS485ADRMATCH - 0x4001 0050)
 * UART1 RS-485 Delay value register (U1RS485DLY - 0x4001 0054)
 */


/* CAN registers */

/* CAN acceptance filter and central CAN registers */
#define AFMR         LPC17_REG(0x4003C000) /* Acceptance Filter Register */
#define SFF_sa       LPC17_REG(0x4003C004) /* Standard Frame Individual Start Address Register */
#define SFF_GRP_sa   LPC17_REG(0x4003C008) /* Standard Frame Group Start Address Register */
#define EFF_sa       LPC17_REG(0x4003C00C) /* Extended Frame Start Address Register */
#define EFF_GRP_sa   LPC17_REG(0x4003C010) /* Extended Frame Group Start Address Register */
#define ENDofTable   LPC17_REG(0x4003C014) /* End of AF Tables register */
#define LUTerrAd     LPC17_REG(0x4003C018) /* LUT Error Address register */
#define LUTerr       LPC17_REG(0x4003C01C) /* LUT Error Register */
#define CANTxSR      LPC17_REG(0x40040000) /* CAN Central Transmit Status Register */
#define CANRxSR      LPC17_REG(0x40040004) /* CAN Central Receive Status Register */
#define CANMSR       LPC17_REG(0x40040008) /* CAN Central Miscellaneous Register */

/* CAN1 controller registers */
#define CAN1MOD      LPC17_REG(0x40044000) /* MOD  Controls the operating mode */
#define CAN1CMR      LPC17_REG(0x40044004) /* CMR  Command bits that affect the state of the CAN Controller */
#define CAN1GSR      LPC17_REG(0x40044008) /* GSR  Global Controller Status and Error Counters */
#define CAN1ICR      LPC17_REG(0x4004400C) /* ICR  Interrupt status, Arbitration Lost Capture, Error Code Capture */
#define CAN1IER      LPC17_REG(0x40044010) /* IER  Interrupt Enable */
#define CAN1BTR      LPC17_REG(0x40044014) /* BTR  Bus Timing */
#define CAN1EWL      LPC17_REG(0x40044018) /* EWL  Error Warning Limit */
#define CAN1SR       LPC17_REG(0x4004401C) /* SR   Status Register */
#define CAN1RFS      LPC17_REG(0x40044020) /* RFS  Receive frame status */
#define CAN1RID      LPC17_REG(0x40044024) /* RID  Received Identifier */
#define CAN1RDA      LPC17_REG(0x40044028) /* RDA  Received data bytes 1-4 */
#define CAN1RDB      LPC17_REG(0x4004402C) /* RDB  Received data bytes 5-8 */
#define CAN1TFI1     LPC17_REG(0x40044030) /* TFI1 Transmit frame info (Tx Buffer 1) */
#define CAN1TID1     LPC17_REG(0x40044034) /* TID1 Transmit Identifier (Tx Buffer 1) */
#define CAN1TDA1     LPC17_REG(0x40044038) /* TDA1 Transmit data bytes 1-4 (Tx Buffer 1) */
#define CAN1TDB1     LPC17_REG(0x4004403C) /* TDB1 Transmit data bytes 5-8 (Tx Buffer 1) */
#define CAN1TFI2     LPC17_REG(0x40044040) /* TFI2 Transmit frame info (Tx Buffer 2) */
#define CAN1TID2     LPC17_REG(0x40044044) /* TID2 Transmit Identifier (Tx Buffer 2) */
#define CAN1TDA2     LPC17_REG(0x40044048) /* TDA2 Transmit data bytes 1-4 (Tx Buffer 2) */
#define CAN1TDB2     LPC17_REG(0x4004404C) /* TDB2 Transmit data bytes 5-8 (Tx Buffer 2) */
#define CAN1TFI3     LPC17_REG(0x40044050) /* TFI3 Transmit frame info (Tx Buffer 3) */
#define CAN1TID3     LPC17_REG(0x40044054) /* TID3 Transmit Identifier (Tx Buffer 3) */
#define CAN1TDA3     LPC17_REG(0x40044058) /* TDA3 Transmit data bytes 1-4 (Tx Buffer 3) */
#define CAN1TDB3     LPC17_REG(0x4004405C) /* TDB3 Transmit data bytes 5-8 (Tx Buffer 3) */

/* CAN2 controller registers */
#define CAN2MOD      LPC17_REG(0x40048000) /* MOD  Controls the operating mode */
#define CAN2CMR      LPC17_REG(0x40048004) /* CMR  Command bits that affect the state of the CAN Controller */
#define CAN2GSR      LPC17_REG(0x40048008) /* GSR  Global Controller Status and Error Counters */
#define CAN2ICR      LPC17_REG(0x4004800C) /* ICR  Interrupt status, Arbitration Lost Capture, Error Code Capture */
#define CAN2IER      LPC17_REG(0x40048010) /* IER  Interrupt Enable */
#define CAN2BTR      LPC17_REG(0x40048014) /* BTR  Bus Timing */
#define CAN2EWL      LPC17_REG(0x40048018) /* EWL  Error Warning Limit */
#define CAN2SR       LPC17_REG(0x4004801C) /* SR   Status Register */
#define CAN2RFS      LPC17_REG(0x40048020) /* RFS  Receive frame status */
#define CAN2RID      LPC17_REG(0x40048024) /* RID  Received Identifier */
#define CAN2RDA      LPC17_REG(0x40048028) /* RDA  Received data bytes 1-4 */
#define CAN2RDB      LPC17_REG(0x4004802C) /* RDB  Received data bytes 5-8 */
#define CAN2TFI1     LPC17_REG(0x40048030) /* TFI1 Transmit frame info (Tx Buffer 1) */
#define CAN2TID1     LPC17_REG(0x40048034) /* TID1 Transmit Identifier (Tx Buffer 1) */
#define CAN2TDA1     LPC17_REG(0x40048038) /* TDA1 Transmit data bytes 1-4 (Tx Buffer 1) */
#define CAN2TDB1     LPC17_REG(0x4004803C) /* TDB1 Transmit data bytes 5-8 (Tx Buffer 1) */
#define CAN2TFI2     LPC17_REG(0x40048040) /* TFI2 Transmit frame info (Tx Buffer 2) */
#define CAN2TID2     LPC17_REG(0x40048044) /* TID2 Transmit Identifier (Tx Buffer 2) */
#define CAN2TDA2     LPC17_REG(0x40048048) /* TDA2 Transmit data bytes 1-4 (Tx Buffer 2) */
#define CAN2TDB2     LPC17_REG(0x4004804C) /* TDB2 Transmit data bytes 5-8 (Tx Buffer 2) */
#define CAN2TFI3     LPC17_REG(0x40048050) /* TFI3 Transmit frame info (Tx Buffer 3) */
#define CAN2TID3     LPC17_REG(0x40048054) /* TID3 Transmit Identifier (Tx Buffer 3) */
#define CAN2TDA3     LPC17_REG(0x40048058) /* TDA3 Transmit data bytes 1-4 (Tx Buffer 3) */
#define CAN2TDB3     LPC17_REG(0x4004805C) /* TDB3 Transmit data bytes 5-8 (Tx Buffer 3) */

/* CAN Wake and Sleep registers */
#define CANSLEEPCLR  LPC17_REG(0x400FC110) /* clear and read CAN channel sleep state */
#define CANWAKEFLAGS LPC17_REG(0x400FC114) /* read the wake-up state of the CAN channels */

/* CAN Mode register (CAN1MOD - 0x4004 4000, CAN2MOD - 0x4004 8000) */
#define CANMOD_RM  (0x1 << 0)
#define CANMOD_LOM (0x1 << 1)
#define CANMOD_STM (0x1 << 2)
#define CANMOD_TPM (0x1 << 3)
#define CANMOD_SM  (0x1 << 4)
#define CANMOD_RPM (0x1 << 5)
#define CANMOD_TM  (0x1 << 7)

/* CAN Command Register (CAN1CMR - 0x4004 x004, CAN2CMR - 0x4004 8004) */
#define CANCMR_TR   (0x1 << 0)
#define CANCMR_AT   (0x1 << 1)
#define CANCMR_RRB  (0x1 << 2)
#define CANCMR_CDO  (0x1 << 3)
#define CANCMR_SRR  (0x1 << 4)
#define CANCMR_STB1 (0x1 << 5)
#define CANCMR_STB2 (0x1 << 6)
#define CANCMR_STB3 (0x1 << 7)

/* CAN Global Status Register (CAN1GSR - 0x4004 x008, CAN2GSR - 0x4004 8008) */
#define CANGSR_RBS   (0x1 << 0)
#define CANGSR_DOS   (0x1 << 1)
#define CANGSR_TBS   (0x1 << 2)
#define CANGSR_TCS   (0x1 << 3)
#define CANGSR_RS    (0x1 << 4)
#define CANGSR_TS    (0x1 << 5)
#define CANGSR_ES    (0x1 << 6)
#define CANGSR_BS    (0x1 << 7)
#define CANGSR_RXERR (0xFF << 16)
#define CANGSR_TXERR (0xFF << 24)

/* CAN Interrupt and Capture Register (CAN1ICR - 0x4004 400C, CAN2ICR - 0x4004 800C) */
#define CANICR_RI     (0x1 << 0)
#define CANICR_TI1    (0x1 << 1)
#define CANICR_EI     (0x1 << 2)
#define CANICR_DOI    (0x1 << 3)
#define CANICR_WUI    (0x1 << 4)
#define CANICR_EPI    (0x1 << 5)
#define CANICR_ALI    (0x1 << 6)
#define CANICR_BEI    (0x1 << 7)
#define CANICR_IDI    (0x1 << 8)
#define CANICR_TI2    (0x1 << 9)
#define CANICR_TI3    (0x1 << 10)
#define CANICR_ERRBIT (0x1F << 16)
#define CANICR_ERRDIR (0x1 << 21)
#define CANICR_ERRC   (0x3 << 22)
#define CANICR_ALCBIT (0xFF << 24)

/* CAN Interrupt Enable Register (CAN1IER - 0x4004 4010, CAN2IER - 0x4004 8010) */
#define CANIER_RIE  (0x1 << 0)
#define CANIER_TIE1 (0x1 << 1)
#define CANIER_EIE  (0x1 << 2)
#define CANIER_DOIE (0x1 << 3)
#define CANIER_WUIE (0x1 << 4)
#define CANIER_EPIE (0x1 << 5)
#define CANIER_ALIE (0x1 << 6)
#define CANIER_BEIE (0x1 << 7)
#define CANIER_IDIE (0x1 << 8)
#define CANIER_TIE2 (0x1 << 9)
#define CANIER_TIE3 (0x1 << 10)

/* CAN Bus Timing Register (CAN1BTR - 0x4004 4014, CAN2BTR - 0x4004 8014) */
#define CANBTR_BRP   (0x3FF << 0)
#define CANBTR_SJW   (0x3 << 14)
#define CANBTR_TESG1 (0xF << 16)
#define CANBTR_TESG2 (0x7 << 20)
#define CANBTR_SAM   (0x1 << 23)

/* skipped CAN Error Warning Limit register (CAN1EWL - 0x4004 4018, CAN2EWL - 0x4004 8018) */

/* CAN Status Register (CAN1SR - 0x4004 401C, CAN2SR - 0x4004 801C) */
#define CANSR_RBS1 (0x1 << 0)
#define CANSR_DOS1 (0x1 << 1)
#define CANSR_TBS1 (0x1 << 2)
#define CANSR_TCS1 (0x1 << 3)
#define CANSR_RS1  (0x1 << 4)
#define CANSR_TS1  (0x1 << 5)
#define CANSR_ES1  (0x1 << 6)
#define CANSR_BS1  (0x1 << 7)
#define CANSR_RBS2 (0x1 << 8)
#define CANSR_DOS2 (0x1 << 9)
#define CANSR_TBS2 (0x1 << 10)
#define CANSR_TCS2 (0x1 << 11)
#define CANSR_RS2  (0x1 << 12)
#define CANSR_TS2  (0x1 << 13)
#define CANSR_ES2  (0x1 << 14)
#define CANSR_BS2  (0x1 << 15)
#define CANSR_RBS3 (0x1 << 16)
#define CANSR_DOS3 (0x1 << 17)
#define CANSR_TBS3 (0x1 << 18)
#define CANSR_TCS3 (0x1 << 19)
#define CANSR_RS3  (0x1 << 20)
#define CANSR_TS3  (0x1 << 21)
#define CANSR_ES3  (0x1 << 22)
#define CANSR_BS3  (0x1 << 23)

/* CAN Receive Frame Status register (CAN1RFS - 0x4004 4020, CAN2RFS - 0x4004 8020) */
#define CANRFS_ID_Index (0x3FF << 0)
#define CANRFS_BP       (0x1 << 10)
#define CANRFS_DLC      (0xF << 16)
#define CANRFS_RTR      (0x1 << 30)
#define CANRFS_FF       (0x1 << 31)

/* skipped CAN Receive Identifier register (CAN1RID - 0x4004 4024, CAN2RID - 0x4004 8024) */

/* CAN Receive Data register A (CAN1RDA - 0x4004 4028, CAN2RDA - 0x4004 8028) */
#define CANRDA_Data_1 (0xFF << 0)
#define CANRDA_Data_2 (0xFF << 8)
#define CANRDA_Data_3 (0xFF << 16)
#define CANRDA_Data_4 (0xFF << 24)

/* CAN Receive Data register B (CAN1RDB - 0x4004 402C, CAN2RDB - 0x4004 802C) */
#define CANRDB_Data_5 (0xFF << 0)
#define CANRDB_Data_6 (0xFF << 8)
#define CANRDB_Data_7 (0xFF << 16)
#define CANRDB_Data_8 (0xFF << 24)

/* CAN Transmit Frame Information register (CAN1TFI[1/2/3] - 0x4004 40[30/ 40/50], CAN2TFI[1/2/3] - 0x4004 80[30/40/50]) */
#define CANTFIm_PRIO (0xFF << 0)
#define CANTFIm_DLC  (0xF << 16)
#define CANTFIm_RTR  (0x1 << 30)
#define CANTFIm_FF   (0x1 << 31)

/* skipped CAN Transmit Identifier register (CAN1TID[1/2/3] - 0x4004 40[34/44/54], CAN2TID[1/2/3] - 0x4004 80[34/44/54]) */

/* CAN Transmit Data register A (CAN1TDA[1/2/3] - 0x4004 40[38/48/58], CAN2TDA[1/2/3] - 0x4004 80[38/48/58]) */
#define CANTDAm_Data_1 (0xFF << 0)
#define CANTDAm_Data_2 (0xFF << 8)
#define CANTDAm_Data_3 (0xFF << 16)
#define CANTDAm_Data_4 (0xFF << 24)

/* CAN Transmit Data register B (CAN1TDB[1/2/3] - 0x4004 40[3C/4C/5C], CAN2TDB[1/2/3] - 0x4004 80[3C/4C/5C]) */
#define CANTDBm_Data_5 (0xFF << 0)
#define CANTDBm_Data_6 (0xFF << 8)
#define CANTDBm_Data_7 (0xFF << 16)
#define CANTDBm_Data_8 (0xFF << 24)

/* CAN Sleep Clear register (CANSLEEPCLR - 0x400F C110) */
#define CANSLEEPCLR_CAN1SLEEP (0x1 << 1)
#define CANSLEEPCLR_CAN2SLEEP (0x1 << 2)

/* CAN Wake-up Flags register (CANWAKEFLAGS - 0x400F C114) */
#define CANWAKEFLAGS_CAN1WAKE (0x1 << 1)
#define CANWAKEFLAGS_CAN2WAKE (0x1 << 2)

/* Central Transmit Status Register (CANTxSR - 0x4004 0000) */
#define CANTxSR_TS1  (0x1 << 0)
#define CANTxSR_TS2  (0x1 << 1)
#define CANTxSR_TBS1 (0x1 << 8)
#define CANTxSR_TBS2 (0x1 << 9)
#define CANTxSR_TCS1 (0x1 << 16)
#define CANTxSR_TCS2 (0x1 << 17)

/* Central Receive Status Register (CANRxSR - 0x4004 0004) */
#define CANRxSR_RS1  (0x1 << 0)
#define CANRxSR_RS2  (0x1 << 1)
#define CANRxSR_RB1  (0x1 << 8)
#define CANRxSR_RB2  (0x1 << 9)
#define CANRxSR_DOS1 (0x1 << 16)
#define CANRxSR_DOS2 (0x1 << 17)

/* Central Miscellaneous Status Register (CANMSR - 0x4004 0008) */
#define CANMSR_E1  (0x1 << 0)
#define CANMSR_E2  (0x1 << 1)
#define CANMSR_BS1 (0x1 << 8)
#define CANMSR_BS2 (0x1 << 9)

/* Acceptance Filter Mode Register (AFMR - 0x4003 C000) */
#define AFMR_AccOff (0x1 << 0)
#define AFMR_AccBP  (0x1 << 1)
#define AFMR_eFCAN  (0x1 << 2)

/*
 * skipped:
 *
 * Standard Frame Individual Start Address register (SFF_sa - 0x4003 C004)
 * Standard Frame Group Start Address register (SFF_GRP_sa - 0x4003 C008)
 * Extended Frame Start Address register (EFF_sa - 0x4003 C00C)
 * Extended Frame Group Start Address register (EFF_GRP_sa - 0x4003 C010)
 * End of AF Tables register (ENDofTable - 0x4003 C014)
 * LUT Error Address register (LUTerrAd - 0x4003 C018)
 * LUT Error register (LUTerr - 0x4003 C01C)
 * Global FullCANInterrupt Enable register (FCANIE - 0x4003 C020)
 */

/* FullCAN Interrupt and Capture registers (FCANIC0 - 0x4003 C024 and FCANIC1 - 0x4003 C028) */
#define FCANIC0_IntPnd0  (0x1 << 0)
#define FCANIC0_IntPnd1  (0x1 << 1)
#define FCANIC0_IntPnd2  (0x1 << 2)
#define FCANIC0_IntPnd3  (0x1 << 3)
#define FCANIC0_IntPnd4  (0x1 << 4)
#define FCANIC0_IntPnd5  (0x1 << 5)
#define FCANIC0_IntPnd6  (0x1 << 6)
#define FCANIC0_IntPnd7  (0x1 << 7)
#define FCANIC0_IntPnd8  (0x1 << 8)
#define FCANIC0_IntPnd9  (0x1 << 9)
#define FCANIC0_IntPnd10 (0x1 << 10)
#define FCANIC0_IntPnd11 (0x1 << 11)
#define FCANIC0_IntPnd12 (0x1 << 12)
#define FCANIC0_IntPnd13 (0x1 << 13)
#define FCANIC0_IntPnd14 (0x1 << 14)
#define FCANIC0_IntPnd15 (0x1 << 15)
#define FCANIC0_IntPnd16 (0x1 << 16)
#define FCANIC0_IntPnd17 (0x1 << 17)
#define FCANIC0_IntPnd18 (0x1 << 18)
#define FCANIC0_IntPnd19 (0x1 << 19)
#define FCANIC0_IntPnd20 (0x1 << 20)
#define FCANIC0_IntPnd21 (0x1 << 21)
#define FCANIC0_IntPnd22 (0x1 << 22)
#define FCANIC0_IntPnd23 (0x1 << 23)
#define FCANIC0_IntPnd24 (0x1 << 24)
#define FCANIC0_IntPnd25 (0x1 << 25)
#define FCANIC0_IntPnd26 (0x1 << 26)
#define FCANIC0_IntPnd27 (0x1 << 27)
#define FCANIC0_IntPnd28 (0x1 << 28)
#define FCANIC0_IntPnd29 (0x1 << 29)
#define FCANIC0_IntPnd30 (0x1 << 30)
#define FCANIC0_IntPnd31 (0x1 << 31)
#define FCANIC1_IntPnd32 (0x1 << 0)
#define FCANIC1_IntPnd33 (0x1 << 1)
#define FCANIC1_IntPnd34 (0x1 << 2)
#define FCANIC1_IntPnd35 (0x1 << 3)
#define FCANIC1_IntPnd36 (0x1 << 4)
#define FCANIC1_IntPnd37 (0x1 << 5)
#define FCANIC1_IntPnd38 (0x1 << 6)
#define FCANIC1_IntPnd39 (0x1 << 7)
#define FCANIC1_IntPnd40 (0x1 << 8)
#define FCANIC1_IntPnd41 (0x1 << 9)
#define FCANIC1_IntPnd42 (0x1 << 10)
#define FCANIC1_IntPnd43 (0x1 << 11)
#define FCANIC1_IntPnd44 (0x1 << 12)
#define FCANIC1_IntPnd45 (0x1 << 13)
#define FCANIC1_IntPnd46 (0x1 << 14)
#define FCANIC1_IntPnd47 (0x1 << 15)
#define FCANIC1_IntPnd48 (0x1 << 16)
#define FCANIC1_IntPnd49 (0x1 << 17)
#define FCANIC1_IntPnd50 (0x1 << 18)
#define FCANIC1_IntPnd51 (0x1 << 19)
#define FCANIC1_IntPnd52 (0x1 << 20)
#define FCANIC1_IntPnd53 (0x1 << 21)
#define FCANIC1_IntPnd54 (0x1 << 22)
#define FCANIC1_IntPnd55 (0x1 << 23)
#define FCANIC1_IntPnd56 (0x1 << 24)
#define FCANIC1_IntPnd57 (0x1 << 25)
#define FCANIC1_IntPnd58 (0x1 << 26)
#define FCANIC1_IntPnd59 (0x1 << 27)
#define FCANIC1_IntPnd60 (0x1 << 28)
#define FCANIC1_IntPnd61 (0x1 << 29)
#define FCANIC1_IntPnd62 (0x1 << 30)
#define FCANIC1_IntPnd63 (0x1 << 31)


/* SPI registers */
#define S0SPCR  LPC17_REG(0x40020000) /* SPI Control Register */
#define S0SPSR  LPC17_REG(0x40020004) /* SPI Status Register */
#define S0SPDR  LPC17_REG(0x40020008) /* SPI Data Register */
#define S0SPCCR LPC17_REG(0x4002000C) /* SPI Clock Counter Register */
#define S0SPINT LPC17_REG(0x4002001C) /* SPI Interrupt Flag */

/* SPI Control Register (S0SPCR - 0x4002 0000) */
#define S0SPCR_BitEnab (0x1 << 2)
#define S0SPCR_CPHA    (0x1 << 3)
#define S0SPCR_CPOL    (0x1 << 4)
#define S0SPCR_MSTR    (0x1 << 5)
#define S0SPCR_LSBF    (0x1 << 6)
#define S0SPCR_SPIE    (0x1 << 7)
#define S0SPCR_BITS    (0xF << 8)

/* SPI Status Register (S0SPSR - 0x4002 0004) */
#define S0SPSR_ABRT (0x1 << 3)
#define S0SPSR_MODF (0x1 << 4)
#define S0SPSR_ROVR (0x1 << 5)
#define S0SPSR_WCOL (0x1 << 6)
#define S0SPSR_SPIF (0x1 << 7)

/*
 * skipped:
 *
 * SPI Data Register (S0SPDR - 0x4002 0008)
 * SPI Clock Counter Register (S0SPCCR - 0x4002 000C)
 */

/* SPI Test Control Register (SPTCR - 0x4002 0010) */
#define SPTCR_Test (0x7F << 1)

/* SPI Test Status Register (SPTSR - 0x4002 0014) */
#define SPTSR_ABRT (0x1 << 3)
#define SPTSR_MODF (0x1 << 4)
#define SPTSR_ROVR (0x1 << 5)
#define SPTSR_WCOL (0x1 << 6)
#define SPTSR_SPIF (0x1 << 7)

/* skipped SPI Interrupt Register (S0SPINT - 0x4002 001C) */


/* SSP registers */

/* SSP0 registers */
#define SSP0CR0   LPC17_REG(0x40088000) /* CR0   Control Register 0 */
#define SSP0CR1   LPC17_REG(0x40088004) /* CR1   Control Register 1 */
#define SSP0DR    LPC17_REG(0x40088008) /* DR    Data Register */
#define SSP0SR    LPC17_REG(0x4008800C) /* SR    Status Register */
#define SSP0CPSR  LPC17_REG(0x40088010) /* CPSR  Clock Prescale Register */
#define SSP0IMSC  LPC17_REG(0x40088014) /* IMSC  Interrupt Mask Set and Clear Register                                     */
#define SSP0RIS   LPC17_REG(0x40088018) /* RIS   Raw Interrupt Status Register */
#define SSP0MIS   LPC17_REG(0x4008801C) /* MIS   Masked Interrupt Status Register */
#define SSP0ICR   LPC17_REG(0x40088020) /* ICR   SSPICR Interrupt Clear Register */
#define SSP0DMACR LPC17_REG(0x40088024) /* DMACR DMA Control Register */

/* SSP1 registers */
#define SSP1CR0   LPC17_REG(0x40030000) /* CR0   Control Register 0 */
#define SSP1CR1   LPC17_REG(0x40030004) /* CR1   Control Register 1 */
#define SSP1DR    LPC17_REG(0x40030008) /* DR    Data Register */
#define SSP1SR    LPC17_REG(0x4003000C) /* SR    Status Register */
#define SSP1CPSR  LPC17_REG(0x40030010) /* CPSR  Clock Prescale Register */
#define SSP1IMSC  LPC17_REG(0x40030014) /* IMSC  Interrupt Mask Set and Clear Register                                     */
#define SSP1RIS   LPC17_REG(0x40030018) /* RIS   Raw Interrupt Status Register */
#define SSP1MIS   LPC17_REG(0x4003001C) /* MIS   Masked Interrupt Status Register */
#define SSP1ICR   LPC17_REG(0x40030020) /* ICR   SSPICR Interrupt Clear Register */
#define SSP1DMACR LPC17_REG(0x40030024) /* DMACR DMA Control Register */

/* SSPn Control Register 0 (SSP0CR0 - 0x4008 8000, SSP1CR0 - 0x4003 0000) */
#define SSPCR0_DSS  (0xF << 0)
#define SSPCR0_FRF  (0x3 << 4)
#define SSPCR0_CPOL (0x1 << 6)
#define SSPCR0_CPHA (0x1 << 7)
#define SSPCR0_SCR  (0xFF << 8)

/* SSPn Control Register 1 (SSP0CR1 - 0x4008 8004, SSP1CR1 - 0x4003 0004) */
#define SSPCR1_LBM (0x1 << 0)
#define SSPCR1_SSE (0x1 << 1)
#define SSPCR1_MS  (0x1 << 2)
#define SSPCR1_SOD (0x1 << 3)

/* skipped SSPn Data Register (SSP0DR - 0x4008 8008, SSP1DR - 0x4003 0008) */

/* SSPn Status Register (SSP0SR - 0x4008 800C, SSP1SR - 0x4003 000C) */
#define SSPSR_TFE (0x1 << 0)
#define SSPSR_TNF (0x1 << 1)
#define SSPSR_RNE (0x1 << 2)
#define SSPSR_RFF (0x1 << 3)
#define SSPSR_BSY (0x1 << 4)

/* skipped SSPn Clock Prescale Register (SSP0CPSR - 0x4008 8010, SSP1CPSR - 0x4003 0010) */

/* SSPn Interrupt Mask Set/Clear Register (SSP0IMSC - 0x4008 8014, SSP1IMSC - 0x4003 0014) */
#define SSPIMSC_RORIM (0x1 << 0)
#define SSPIMSC_RTIM  (0x1 << 1)
#define SSPIMSC_RXIM  (0x1 << 2)
#define SSPIMSC_TXIM  (0x1 << 3)

/* SSPn Raw Interrupt Status Register (SSP0RIS - 0x4008 8018, SSP1RIS - 0x4003 0018) */
#define SSPRIS_RORRIS (0x1 << 0)
#define SSPRIS_RTRIS  (0x1 << 1)
#define SSPRIS_RXRIS  (0x1 << 2)
#define SSPRIS_TXRIS  (0x1 << 3)

/* SSPn Masked Interrupt Status Register (SSP0MIS - 0x4008 801C, SSP1MIS - 0x4003 001C) */
#define SSPMIS_RORMIS (0x1 << 0)
#define SSPMIS_RTMIS  (0x1 << 1)
#define SSPMIS_RXMIS  (0x1 << 2)
#define SSPMIS_TXMIS  (0x1 << 3)

/* SSPn Interrupt Clear Register (SSP0ICR - 0x4008 8020, SSP1ICR - 0x4003 0020) */
#define SSPICR_RORIC (0x1 << 0)
#define SSPICR_RTIC  (0x1 << 1)

/* SSPn DMA Control Register (SSP0DMACR - 0x4008 8024, SSP1DMACR - 0x4003 0024) */
#define SSPDMACR_RXDMAE (0x1 << 0)
#define SSPDMACR_TXDMAE (0x1 << 1)


/* I2C registers */

/* I2C0 registers */
#define I2C0CONSET      LPC17_REG(0x4001C000) /* I2CONSET I2C Control Set Register */
#define I2C0STAT        LPC17_REG(0x4001C004) /* I2STAT   I2C Status Register */
#define I2C0DAT         LPC17_REG(0x4001C008) /* I2DAT    I2C Data Register */
#define I2C0ADR0        LPC17_REG(0x4001C00C) /* I2ADR0   I2C Slave Address Register 0 */
#define I2C0SCLH        LPC17_REG(0x4001C010) /* I2SCLH   SCH Duty Cycle Register High Half Word */
#define I2C0SCLL        LPC17_REG(0x4001C014) /* I2SCLL   SCL Duty Cycle Register Low Half Word */
#define I2C0CONCLR      LPC17_REG(0x4001C018) /* I2CONCLR I2C Control Clear Register */
#define I2C0MMCTRL      LPC17_REG(0x4001C01C) /* MMCTRL   Monitor mode control register */
#define I2C0ADR1        LPC17_REG(0x4001C020) /* I2ADR1   I2C Slave Address Register 1 */
#define I2C0ADR2        LPC17_REG(0x4001C024) /* 2ADR2    I2C Slave Address Register 2 */
#define I2C0ADR3        LPC17_REG(0x4001C028) /* I2ADR3   I2C Slave Address Register 3 */
#define I2C0DATA_BUFFER LPC17_REG(0x4001C02C) /* I2DATA_BUFFER Data buffer register */
#define I2C0MASK0       LPC17_REG(0x4001C030) /* I2MASK0  I2C Slave address mask register 0 */
#define I2C0MASK1       LPC17_REG(0x4001C034) /* I2MASK1  I2C Slave address mask register 1 */
#define I2C0MASK2       LPC17_REG(0x4001C038) /* I2MASK2  I2C Slave address mask register 2 */
#define I2C0MASK3       LPC17_REG(0x4001C03C) /* I2MASK3  I2C Slave address mask register 3 */

/* I2C1 registers */
#define I2C1CONSET      LPC17_REG(0x4005C000) /* I2CONSET I2C Control Set Register */
#define I2C1STAT        LPC17_REG(0x4005C004) /* I2STAT   I2C Status Register */
#define I2C1DAT         LPC17_REG(0x4005C008) /* I2DAT    I2C Data Register */
#define I2C1ADR0        LPC17_REG(0x4005C00C) /* I2ADR0   I2C Slave Address Register 0 */
#define I2C1SCLH        LPC17_REG(0x4005C010) /* I2SCLH   SCH Duty Cycle Register High Half Word */
#define I2C1SCLL        LPC17_REG(0x4005C014) /* I2SCLL   SCL Duty Cycle Register Low Half Word */
#define I2C1CONCLR      LPC17_REG(0x4005C018) /* I2CONCLR I2C Control Clear Register */
#define I2C1MMCTRL      LPC17_REG(0x4005C01C) /* MMCTRL   Monitor mode control register */
#define I2C1ADR1        LPC17_REG(0x4005C020) /* I2ADR1   I2C Slave Address Register 1 */
#define I2C1ADR2        LPC17_REG(0x4005C024) /* 2ADR2    I2C Slave Address Register 2 */
#define I2C1ADR3        LPC17_REG(0x4005C028) /* I2ADR3   I2C Slave Address Register 3 */
#define I2C1DATA_BUFFER LPC17_REG(0x4005C02C) /* I2DATA_BUFFER Data buffer register */
#define I2C1MASK0       LPC17_REG(0x4005C030) /* I2MASK0  I2C Slave address mask register 0 */
#define I2C1MASK1       LPC17_REG(0x4005C034) /* I2MASK1  I2C Slave address mask register 1 */
#define I2C1MASK2       LPC17_REG(0x4005C038) /* I2MASK2  I2C Slave address mask register 2 */
#define I2C1MASK3       LPC17_REG(0x4005C03C) /* I2MASK3  I2C Slave address mask register 3 */

/* I2C2 registers */
#define I2C2CONSET      LPC17_REG(0x400A0000) /* I2CONSET I2C Control Set Register */
#define I2C2STAT        LPC17_REG(0x400A0004) /* I2STAT   I2C Status Register */
#define I2C2DAT         LPC17_REG(0x400A0008) /* I2DAT    I2C Data Register */
#define I2C2ADR0        LPC17_REG(0x400A000C) /* I2ADR0   I2C Slave Address Register 0 */
#define I2C2SCLH        LPC17_REG(0x400A0010) /* I2SCLH   SCH Duty Cycle Register High Half Word */
#define I2C2SCLL        LPC17_REG(0x400A0014) /* I2SCLL   SCL Duty Cycle Register Low Half Word */
#define I2C2CONCLR      LPC17_REG(0x400A0018) /* I2CONCLR I2C Control Clear Register */
#define I2C2MMCTRL      LPC17_REG(0x400A001C) /* MMCTRL   Monitor mode control register */
#define I2C2ADR1        LPC17_REG(0x400A0020) /* I2ADR1   I2C Slave Address Register 1 */
#define I2C2ADR2        LPC17_REG(0x400A0024) /* 2ADR2    I2C Slave Address Register 2 */
#define I2C2ADR3        LPC17_REG(0x400A0028) /* I2ADR3   I2C Slave Address Register 3 */
#define I2C2DATA_BUFFER LPC17_REG(0x400A002C) /* I2DATA_BUFFER Data buffer register */
#define I2C2MASK0       LPC17_REG(0x400A0030) /* I2MASK0  I2C Slave address mask register 0 */
#define I2C2MASK1       LPC17_REG(0x400A0034) /* I2MASK1  I2C Slave address mask register 1 */
#define I2C2MASK2       LPC17_REG(0x400A0038) /* I2MASK2  I2C Slave address mask register 2 */
#define I2C2MASK3       LPC17_REG(0x400A003C) /* I2MASK3  I2C Slave address mask register 3 */

/* I2C Control Set register (I2CONSET: I2C0, I2C0CONSET - 0x4001 C000; I2C1, I2C1CONSET - 0x4005 C000; I2C2, I2C2CONSET - 0x400A 0000) */
#define I2CONSET_AA   (0x1 << 2)
#define I2CONSET_SI   (0x1 << 3)
#define I2CONSET_STO  (0x1 << 4)
#define I2CONSET_STA  (0x1 << 5)
#define I2CONSET_I2EN (0x1 << 6)

/* I2C Control Clear register (I2CONCLR: I2C0, I2C0CONCLR - 0x4001 C018; I2C1, I2C1CONCLR - 0x4005 C018; I2C2, I2C2CONCLR - 0x400A 0018) */
#define I2CONCLR_AAC   (0x1 << 2)
#define I2CONCLR_SIC   (0x1 << 3)
#define I2CONCLR_STAC  (0x1 << 5)
#define I2CONCLR_I2ENC (0x1 << 6)

/*
 * skipped:
 *
 * I2C Status register (I2STAT: I2C0, I2C0STAT - 0x4001 C004; I2C1, I2C1STAT - 0x4005 C004; I2C2, I2C2STAT - 0x400A 0004)
 * I2C Data register (I2DAT: I2C0, I2C0DAT - 0x4001 C008; I2C1, I2C1DAT - 0x4005 C008; I2C2, I2C2DAT - 0x400A 0008)
 */

/* I2C Monitor mode control register (I2MMCTRL: I2C0, I2C0MMCTRL - 0x4001 C01C; I2C1, I2C1MMCTRL- 0x4005 C01C; I2C2, I2C2MMCTRL- 0x400A 001C) */
#define I2MMCTRL_MM_ENA    (0x1 << 0)
#define I2MMCTRL_ENA_SCL   (0x1 << 1)
#define I2MMCTRL_MATCH_ALL (0x1 << 2)

/* skipped I2C Data buffer register (I2DATA_BUFFER: I2C0, I2CDATA_BUFFER - 0x4001 C02C; I2C1, I2C1DATA_BUFFER- 0x4005 C02C; I2C2, I2C2DATA_BUFFER- 0x400A 002C) */

/* I2C Slave Address registers (I2ADR0 to 3: I2C0, I2C0ADR[0, 1, 2, 3]- 0x4001 C0[0C, 20, 24, 28]; I2C1, I2C1ADR[0, 1, 2, 3] - address 0x4005 C0[0C, 20, 24, 28]; I2C2, I2C2ADR[0, 1, 2, 3] - address 0x400A 00[0C, 20, 24, 28]) */
#define I2ADR_GC      (0x1 << 0)
#define I2ADR_Address (0x7F << 1)

/* I2C Mask registers (I2MASK0 to 3: I2C0, I2C0MASK[0, 1, 2, 3] - 0x4001 C0[30, 34, 38, 3C]; I2C1, I2C1MASK[0, 1, 2, 3] - address 0x4005 C0[30, 34, 38, 3C]; I2C2, I2C2MASK[0, 1, 2, 3] - address 0x400A 00[30, 34, 38, 3C]) */
#define I2MASK_MASK (0x7F << 1)

/*
 * skipped:
 *
 * I2C SCL HIGH duty cycle register (I2SCLH: I2C0, I2C0SCLH - 0x4001 C010; I2C1, I2C1SCLH - 0x4005 C010; I2C2, I2C2SCLH - 0x400A 0010)
 * I2C SCL Low duty cycle register (I2SCLL: I2C0 - I2C0SCLL: 0x4001 C014; I2C1 - I2C1SCLL: 0x4005 C014; I2C2 - I2C2SCLL: 0x400A 0014)
 */


/* I2S registers */

#define I2SDAO       LPC17_REG(0x400A8000) /* Digital Audio Output Register */
#define I2SDAI       LPC17_REG(0x400A8004) /* Digital Audio Input Register */
#define I2STXFIFO    LPC17_REG(0x400A8008) /* Transmit FIFO */
#define I2SRXFIFO    LPC17_REG(0x400A800C) /* Receive FIFO */
#define I2SSTATE     LPC17_REG(0x400A8010) /* Status Feedback Register */
#define I2SDMA1      LPC17_REG(0x400A8014) /* DMA Configuration Register 1 */
#define I2SDMA2      LPC17_REG(0x400A8018) /* DMA Configuration Register 2 */
#define I2SIRQ       LPC17_REG(0x400A801C) /* Interrupt Request Control Register */
#define I2STXRATE    LPC17_REG(0x400A8020) /* Transmit MCLK divider */
#define I2SRXRATE    LPC17_REG(0x400A8024) /* Receive MCLK divider */
#define I2STXBITRATE LPC17_REG(0x400A8028) /* Transmit bit rate divider */
#define I2SRXBITRATE LPC17_REG(0x400A802C) /* Receive bit rate divider */
#define I2STXMODE    LPC17_REG(0x400A8030) /* Transmit mode control */
#define I2SRXMODE    LPC17_REG(0x400A8034) /* Receive mode control */

/* Digital Audio Output register (I2SDAO - 0x400A 8000) */
#define I2SDAO_wordwidth     (0x3 << 0)
#define I2SDAO_mono          (0x1 << 2)
#define I2SDAO_stop          (0x1 << 3)
#define I2SDAO_reset         (0x1 << 4)
#define I2SDAO_ws_sel        (0x1 << 5)
#define I2SDAO_ws_halfperiod (0x1FF << 6)
#define I2SDAO_mute          (0x1 << 15)

/* Digital Audio Input register (I2SDAI - 0x400A 8004) */
#define I2SDAI_wordwidth     (0x3 << 0)
#define I2SDAI_mono          (0x1 << 2)
#define I2SDAI_stop          (0x1 << 3)
#define I2SDAI_reset         (0x1 << 4)
#define I2SDAI_ws_sel        (0x1 << 5)
#define I2SDAI_ws_halfperiod (0x1FF << 6)

/*
 * skipped:
 *
 * Transmit FIFO register (I2STXFIFO - 0x400A 8008)
 * Receive FIFO register (I2SRXFIFO - 0x400A 800C)
 */

/* Status Feedback register (I2SSTATE - 0x400A 8010) */
#define I2SSTATE_irq      (0x1 << 0)
#define I2SSTATE_dmareq1  (0x1 << 1)
#define I2SSTATE_dmareq2  (0x1 << 2)
#define I2SSTATE_Unused   (0x1F << 3)
#define I2SSTATE_rx_level (0xF << 8)
#define I2SSTATE_tx_level (0xF << 16)

/* DMA Configuration Register 1 (I2SDMA1 - 0x400A 8014) */
#define I2SDMA1_rx_dma1_enable (0x1 << 0)
#define I2SDMA1_tx_dma1_enable (0x1 << 1)
#define I2SDMA1_rx_depth_dma1  (0xF << 8)
#define I2SDMA1_tx_depth_dma1  (0xF << 16)

/* DMA Configuration Register 2 (I2SDMA2 - 0x400A 8018) */
#define I2SDMA2_rx_dma2_enable (0x1 << 0)
#define I2SDMA2_tx_dma2_enable (0x1 << 1)
#define I2SDMA2_rx_depth_dma2  (0xF << 8)
#define I2SDMA2_tx_depth_dma2  (0xF << 16)

/* Interrupt Request Control register (I2SIRQ - 0x400A 801C) */
#define I2SIRQ_rx_Irq_enable (0x1 << 0)
#define I2SIRQ_tx_Irq_enable (0x1 << 1)
#define I2SIRQ_rx_depth_irq  (0xF << 8)
#define I2SIRQ_tx_depth_irq  (0xF << 16)

/* Transmit Clock Rate register (I2STXRATE - 0x400A 8020) */
#define I2STXRATE_Y_divider (0xFF << 0)
#define I2STXRATE_X_divider (0xFF << 8)

/* Receive Clock Rate register (I2SRXRATE - 0x400A 8024) */
#define I2SRXRATE_Y_divider (0xFF << 0)
#define I2SRXRATE_X_divider (0xFF << 8)

/*
 * skipped:
 *
 * Transmit Clock Bit Rate register (I2STXBITRATE - 0x400A 8028)
 * Receive Clock Bit Rate register (I2SRXBITRATE - 0x400A 802C)
 */

/* Transmit Mode Control register (I2STXMODE - 0x400A 8030) */
#define I2STXMODE_TXCLKSEL (0x3 << 0)
#define I2STXMODE_TX4PIN   (0x1 << 2)
#define I2STXMODE_TXMCENA  (0x1 << 3)

/* Receive Mode Control register (I2SRXMODE - 0x400A 8034) */
#define I2SRXMODE_RXCLKSEL (0x3 << 0)
#define I2SRXMODE_RX4PIN   (0x1 << 2)
#define I2SRXMODE_RXMCENA  (0x1 << 3)


/* timer registers */

/* timer 1 registers */
#define T0IR   LPC17_REG(0x40004000) /* IR   Interrupt Register */
#define T0TCR  LPC17_REG(0x40004004) /* TCR  Timer Control Register */
#define T0TC   LPC17_REG(0x40004008) /* TC   Timer Counter */
#define T0PR   LPC17_REG(0x4000400C) /* PR   Prescale Register */
#define T0PC   LPC17_REG(0x40004010) /* PC   Prescale Counter */
#define T0MCR  LPC17_REG(0x40004014) /* MCR  Match Control Register */
#define T0MR0  LPC17_REG(0x40004018) /* MR0  Match Register 0 */
#define T0MR1  LPC17_REG(0x4000401C) /* MR1  Match Register 1 */
#define T0MR2  LPC17_REG(0x40004020) /* MR2  Match Register 2 */
#define T0MR3  LPC17_REG(0x40004024) /* MR3  Match Register 3 */
#define T0CCR  LPC17_REG(0x40004028) /* CCR  Capture Control Register */
#define T0CR0  LPC17_REG(0x4000402C) /* CR0  Capture Register 0 */
#define T0CR1  LPC17_REG(0x40004030) /* CR1  Capture Register 1 */
#define T0EMR  LPC17_REG(0x4000403C) /* EMR  External Match Register */
#define T0CTCR LPC17_REG(0x40004070) /* CTCR Count Control Register */

/* timer 2 registers */
#define T1IR   LPC17_REG(0x40008000) /* IR   Interrupt Register */
#define T1TCR  LPC17_REG(0x40008004) /* TCR  Timer Control Register */
#define T1TC   LPC17_REG(0x40008008) /* TC   Timer Counter */
#define T1PR   LPC17_REG(0x4000800C) /* PR   Prescale Register */
#define T1PC   LPC17_REG(0x40008010) /* PC   Prescale Counter */
#define T1MCR  LPC17_REG(0x40008014) /* MCR  Match Control Register */
#define T1MR0  LPC17_REG(0x40008018) /* MR0  Match Register 0 */
#define T1MR1  LPC17_REG(0x4000801C) /* MR1  Match Register 1 */
#define T1MR2  LPC17_REG(0x40008020) /* MR2  Match Register 2 */
#define T1MR3  LPC17_REG(0x40008024) /* MR3  Match Register 3 */
#define T1CCR  LPC17_REG(0x40008028) /* CCR  Capture Control Register */
#define T1CR0  LPC17_REG(0x4000802C) /* CR0  Capture Register 0 */
#define T1CR1  LPC17_REG(0x40008030) /* CR1  Capture Register 1 */
#define T1EMR  LPC17_REG(0x4000803C) /* EMR  External Match Register */
#define T1CTCR LPC17_REG(0x40008070) /* CTCR Count Control Register */

/* timer 3 registers */
#define T2IR   LPC17_REG(0x40090000) /* IR   Interrupt Register */
#define T2TCR  LPC17_REG(0x40090004) /* TCR  Timer Control Register */
#define T2TC   LPC17_REG(0x40090008) /* TC   Timer Counter */
#define T2PR   LPC17_REG(0x4009000C) /* PR   Prescale Register */
#define T2PC   LPC17_REG(0x40090010) /* PC   Prescale Counter */
#define T2MCR  LPC17_REG(0x40090014) /* MCR  Match Control Register */
#define T2MR0  LPC17_REG(0x40090018) /* MR0  Match Register 0 */
#define T2MR1  LPC17_REG(0x4009001C) /* MR1  Match Register 1 */
#define T2MR2  LPC17_REG(0x40090020) /* MR2  Match Register 2 */
#define T2MR3  LPC17_REG(0x40090024) /* MR3  Match Register 3 */
#define T2CCR  LPC17_REG(0x40090028) /* CCR  Capture Control Register */
#define T2CR0  LPC17_REG(0x4009002C) /* CR0  Capture Register 0 */
#define T2CR1  LPC17_REG(0x40090030) /* CR1  Capture Register 1 */
#define T2EMR  LPC17_REG(0x4009003C) /* EMR  External Match Register */
#define T2CTCR LPC17_REG(0x40090070) /* CTCR Count Control Register */

/* timer 4 registers */
#define T3IR   LPC17_REG(0x40094000) /* IR   Interrupt Register */
#define T3TCR  LPC17_REG(0x40094004) /* TCR  Timer Control Register */
#define T3TC   LPC17_REG(0x40094008) /* TC   Timer Counter */
#define T3PR   LPC17_REG(0x4009400C) /* PR   Prescale Register */
#define T3PC   LPC17_REG(0x40094010) /* PC   Prescale Counter */
#define T3MCR  LPC17_REG(0x40094014) /* MCR  Match Control Register */
#define T3MR0  LPC17_REG(0x40094018) /* MR0  Match Register 0 */
#define T3MR1  LPC17_REG(0x4009401C) /* MR1  Match Register 1 */
#define T3MR2  LPC17_REG(0x40094020) /* MR2  Match Register 2 */
#define T3MR3  LPC17_REG(0x40094024) /* MR3  Match Register 3 */
#define T3CCR  LPC17_REG(0x40094028) /* CCR  Capture Control Register */
#define T3CR0  LPC17_REG(0x4009402C) /* CR0  Capture Register 0 */
#define T3CR1  LPC17_REG(0x40094030) /* CR1  Capture Register 1 */
#define T3EMR  LPC17_REG(0x4009403C) /* EMR  External Match Register */
#define T3CTCR LPC17_REG(0x40094070) /* CTCR Count Control Register */

/* Interrupt Register (T[0/1/2/3]IR - 0x4000 4000, 0x4000 8000, 0x4009 0000, 0x4009 4000) */
#define TIR_MR0_Interrupt (0x1 << 0)
#define TIR_MR1_Interrupt (0x1 << 1)
#define TIR_MR2 Interrupt (0x1 << 2)
#define TIR_MR3_Interrupt (0x1 << 3)
#define TIR_CR0_Interrupt (0x1 << 4)
#define TIR_CR1_Interrupt (0x1 << 5)

/* Timer Control Register (T[0/1/2/3]CR - 0x4000 4004, 0x4000 8004, 0x4009 0004, 0x4009 4004) */
#define TCR_Counter_Enable (0x1 << 0)
#define TCR_Counter_Reset  (0x1 << 1)

/* Count Control Register (T[0/1/2/3]CTCR - 0x4000 4070, 0x4000 8070, 0x4009 0070, 0x4009 4070) */
#define TCTCR_Counter_Timer_Mode (0x3 << 0)
#define TCTCR_Count_Input_Select (0x3 << 2)

/*
 * skipped:
 *
 * Timer Counter registers (T0TC - T3TC, 0x4000 4008, 0x4000 8008, 0x4009 0008, 0x4009 4008)
 * Prescale register (T0PR - T3PR, 0x4000 400C, 0x4000 800C, 0x4009 000C, 0x4009 400C)
 * Prescale Counter register (T0PC - T3PC, 0x4000 4010, 0x4000 8010, 0x4009 0010, 0x4009 4010)
 * Match Registers (MR0 - MR3)
 */

/* Match Control Register (T[0/1/2/3]MCR - 0x4000 4014, 0x4000 8014, 0x4009 0014, 0x4009 4014) */
#define TMCR_MR0I (0x1 << 0)
#define TMCR_MR0R (0x1 << 1)
#define TMCR_MR0S (0x1 << 2)
#define TMCR_MR1I (0x1 << 3)
#define TMCR_MR1R (0x1 << 4)
#define TMCR_MR1S (0x1 << 5)
#define TMCR_MR2I (0x1 << 6)
#define TMCR_MR2R (0x1 << 7)
#define TMCR_MR2S (0x1 << 8)
#define TMCR_MR3I (0x1 << 9)
#define TMCR_MR3R (0x1 << 10)
#define TMCR_MR3S (0x1 << 11)

/* skipped Capture Registers (CR0 - CR1) */

/* Capture Control Register (T[0/1/2/3]CCR - 0x4000 4028, 0x4000 8028, 0x4009 0028, 0x4009 4028) */
#define TCCR_CAP0RE (0x1 << 0)
#define TCCR_CAP0FE (0x1 << 1)
#define TCCR_CAP0I  (0x1 << 2)
#define TCCR_CAP1RE (0x1 << 3)
#define TCCR_CAP1FE (0x1 << 4)
#define TCCR_CAP1I  (0x1 << 5)

/* External Match Register (T[0/1/2/3]EMR - 0x4000 403C, 0x4000 803C, 0x4009 003C, 0x4009 403C) */
#define TEMR_EM0  (0x1 << 0)
#define TEMR_EM1  (0x1 << 1)
#define TEMR_EM2  (0x1 << 2)
#define TEMR_EM3  (0x1 << 3)
#define TEMR_EMC0 (0x3 << 4)
#define TEMR_EMC1 (0x3 << 6)
#define TEMR_EMC2 (0x3 << 8)
#define TEMR_EMC3 (0x3 << 10)


/* repetitive interrupt timer registers */

#define RICOMPVAL LPC17_REG(0x400B0000) /* Compare register */
#define RIMASK    LPC17_REG(0x400B0004) /* Mask register */
#define RICTRL    LPC17_REG(0x400B0008) /* Control register */
#define RICOUNTER LPC17_REG(0x400B000C) /* 32-bit counter */

/*
 * skipped:
 *
 * RI Compare Value register (RICOMPVAL - 0x400B 0000)
 * RI Mask register (RIMASK - 0x400B 0004)
 */

/* RI Control register (RICTRL - 0x400B 0008) */
#define RICTRL_RITINT   (0x1 << 0)
#define RICTRL_RITENCLR (0x1 << 1)
#define RICTRL_RITENBR  (0x1 << 2)
#define RICTRL_RITEN    (0x1 << 3)

/* skipped RI Counter register (RICOUNTER - 0x400B 000C) */

/* system tick timer resgisters */

#define STCTRL   LPC17_REG(0xE000E010) /* System Timer Control and status register */
#define STRELOAD LPC17_REG(0xE000E014) /* System Timer Reload value register */
#define STCURR   LPC17_REG(0xE000E018) /* System Timer Current value register */
#define STCALIB  LPC17_REG(0xE000E01C) /* System Timer Calibration value register */

/* System Timer Control and status register (STCTRL - 0xE000 E010) */
#define STCTRL_ENABLE    (0x1 << 0)
#define STCTRL_TICKINT   (0x1 << 1)
#define STCTRL_CLKSOURCE (0x1 << 2)
#define STCTRL_COUNTFLAG (0x1 << 16)

/*
 * skipped:
 *
 * System Timer Reload value register (STRELOAD - 0xE000 E014)
 * System Timer Current value register (STCURR - 0xE000 E018)
 */

/* System Timer Calibration value register (STCALIB - 0xE000 E01C) */
#define STCALIB_TENMS (0xFFFFFF << 0)
#define STCALIB_SKEW  (0x1 << 30)
#define STCALIB_NOREF (0x1 << 31)


/* PWM1 registers */

#define PWM1IR   LPC17_REG(0x40018000) /* IR   Interrupt Register */
#define PWM1TCR  LPC17_REG(0x40018004) /* TCR  Timer Control Register */
#define PWM1TC   LPC17_REG(0x40018008) /* TC   Timer Counter */
#define PWM1PR   LPC17_REG(0x4001800C) /* PR   Prescale Register */
#define PWM1PC   LPC17_REG(0x40018010) /* PC   Prescale Counter */
#define PWM1MCR  LPC17_REG(0x40018014) /* MCR  Match Control Register */
#define PWM1MR0  LPC17_REG(0x40018018) /* MR0  Match Register 0 */
#define PWM1MR1  LPC17_REG(0x4001801C) /* MR1  Match Register 1 */
#define PWM1MR2  LPC17_REG(0x40018020) /* MR2  Match Register 2 */
#define PWM1MR3  LPC17_REG(0x40018024) /* MR3  Match Register 3 */
#define PWM1CCR  LPC17_REG(0x40018028) /* CCR  Capture Control Register */
#define PWM1CR0  LPC17_REG(0x4001802C) /* CR0  Capture Register 0 */
#define PWM1CR1  LPC17_REG(0x40018030) /* CR1  Capture Register 1 */
#define PWM1CR2  LPC17_REG(0x40018034) /* CR2  Capture Register 2 */
#define PWM1CR3  LPC17_REG(0x40018038) /* CR3  Capture Register 3 */
#define PWM1MR4  LPC17_REG(0x40018040) /* MR4  Match Register 4 */
#define PWM1MR5  LPC17_REG(0x40018044) /* MR5  Match Register 5 */
#define PWM1MR6  LPC17_REG(0x40018048) /* MR6  Match Register 6 */
#define PWM1PCR  LPC17_REG(0x4001804C) /* PCR  PWM Control Register */
#define PWM1LER  LPC17_REG(0x40018050) /* LER  Load Enable Register */
#define PWM1CTCR LPC17_REG(0x40018070) /* CTCR Count Control Register */

/* PWM Interrupt Register (PWM1IR - 0x4001 8000) */
#define PWM1IR_PWMMR0_Interrupt  (0x1 << 0)
#define PWM1IR_PWMMR1_Interrupt  (0x1 << 1)
#define PWM1IR_PWMMR2_Interrupt  (0x1 << 2)
#define PWM1IR_PWMMR3_Interrupt  (0x1 << 3)
#define PWM1IR_PWMCAP0_Interrupt (0x1 << 4)
#define PWM1IR_PWMCAP1_Interrupt (0x1 << 5)
#define PWM1IR_PWMMR4_Interrupt  (0x1 << 8)
#define PWM1IR_PWMMR5_Interrupt  (0x1 << 9)
#define PWM1IR_PWMMR6_Interrupt  (0x1 << 10)

/* PWM Timer Control Register (PWM1TCR 0x4001 8004) */
#define PWM1TCR_Counter_Enable (0x1 << 0)
#define PWM1TCR_Counter_Reset  (0x1 << 1)
#define PWM1TCR_PWM_Enable     (0x1 << 3)

/* PWM Count Control Register (PWM1CTCR - 0x4001 8070) */
#define PWM1CTCR_Counter_Timer_Mode (0x3 << 0)
#define PWM1CTCR_Count_Input_Select (0x3 << 2)

/* PWM Match Control Register (PWM1MCR - 0x4001 8014) */
#define PWM1MCR_PWMMR0I (0x1 << 0)
#define PWM1MCR_PWMMR0R (0x1 << 1)
#define PWM1MCR_PWMMR0S (0x1 << 2)
#define PWM1MCR_PWMMR1I (0x1 << 3)
#define PWM1MCR_PWMMR1R (0x1 << 4)
#define PWM1MCR_PWMMR1S (0x1 << 5)
#define PWM1MCR_PWMMR2I (0x1 << 6)
#define PWM1MCR_PWMMR2R (0x1 << 7)
#define PWM1MCR_PWMMR2S (0x1 << 8)
#define PWM1MCR_PWMMR3I (0x1 << 9)
#define PWM1MCR_PWMMR3R (0x1 << 10)
#define PWM1MCR_PWMMR3S (0x1 << 11)
#define PWM1MCR_PWMMR4I (0x1 << 12)
#define PWM1MCR_PWMMR4R (0x1 << 13)
#define PWM1MCR_PWMMR4S (0x1 << 14)
#define PWM1MCR_PWMMR5I (0x1 << 15)
#define PWM1MCR_PWMMR5R (0x1 << 16)
#define PWM1MCR_PWMMR5S (0x1 << 17)
#define PWM1MCR_PWMMR6I (0x1 << 18)
#define PWM1MCR_PWMMR6R (0x1 << 19)
#define PWM1MCR_PWMMR6S (0x1 << 20)

/* PWM Capture Control Register (PWM1CCR - 0x4001 8028) */
#define PWM1CCR_Capture_on_CAPn0_rising_edge  (0x1 << 0)
#define PWM1CCR_Capture_on_CAPn0_falling_edge (0x1 << 1)
#define PWM1CCR_Interrupt_on_CAPn0_event      (0x1 << 2)
#define PWM1CCR_Capture_on_CAPn1_rising_edge  (0x1 << 3)
#define PWM1CCR_Capture_on_CAPn1_falling_edge (0x1 << 4)
#define PWM1CCR_Interrupt_on_CAPn1_event      (0x1 << 5)

/* PWM Control Register (PWM1PCR - 0x4001 804C) */
#define PWM1PCR_PWMSEL2 (0x1 << 2)
#define PWM1PCR_PWMSEL3 (0x1 << 3)
#define PWM1PCR_PWMSEL4 (0x1 << 4)
#define PWM1PCR_PWMSEL5 (0x1 << 5)
#define PWM1PCR_PWMSEL6 (0x1 << 6)
#define PWM1PCR_PWMENA1 (0x1 << 9)
#define PWM1PCR_PWMENA2 (0x1 << 10)
#define PWM1PCR_PWMENA3 (0x1 << 11)
#define PWM1PCR_PWMENA4 (0x1 << 12)
#define PWM1PCR_PWMENA5 (0x1 << 13)
#define PWM1PCR_PWMENA6 (0x1 << 14)

/* skipped PWM Latch Enable Register (PWM1LER - 0x4001 8050) */


/* MCPWM registers */

#define MCCON        LPC17_REG(0x400B8000) /* PWM Control read address */
#define MCCON_SET    LPC17_REG(0x400B8004) /* PWM Control set address */
#define MCCON_CLR    LPC17_REG(0x400B8008) /* PWM Control clear address */
#define MCCAPCON     LPC17_REG(0x400B800C) /* Capture Control read address */
#define MCCAPCON_SET LPC17_REG(0x400B8010) /* Capture Control set address */
#define MCCAPCON_CLR LPC17_REG(0x400B8014) /* Event Control clear address */
#define MCTC0        LPC17_REG(0x400B8018) /* Timer Counter register, channel 0 */
#define MCTC1        LPC17_REG(0x400B801C) /* Timer Counter register, channel 1 */
#define MCTC2        LPC17_REG(0x400B8020) /* Timer Counter register, channel 2 */
#define MCLIM0       LPC17_REG(0x400B8024) /* Limit register, channel 0 */
#define MCLIM1       LPC17_REG(0x400B8028) /* Limit register, channel 1 */
#define MCLIM2       LPC17_REG(0x400B802C) /* Limit register, channel 2 */
#define MCMAT0       LPC17_REG(0x400B8030) /* Match register, channel 0 */
#define MCMAT1       LPC17_REG(0x400B8034) /* Match register, channel 1 */
#define MCMAT2       LPC17_REG(0x400B8038) /* Match register, channel 2 */
#define MCDT         LPC17_REG(0x400B803C) /* Dead time register */
#define MCCP         LPC17_REG(0x400B8040) /* Commutation Pattern register */
#define MCCAP0       LPC17_REG(0x400B8044) /* Capture register, channel 0 */
#define MCCAP1       LPC17_REG(0x400B8048) /* Capture register, channel 1 */
#define MCCAP2       LPC17_REG(0x400B804C) /* Capture register, channel 2 */
#define MCINTEN      LPC17_REG(0x400B8050) /* Interrupt Enable read address */
#define MCINTEN_SET  LPC17_REG(0x400B8054) /* Interrupt Enable set address */
#define MCINTEN_CLR  LPC17_REG(0x400B8058) /* Interrupt Enable clear address */
#define MCCNTCON     LPC17_REG(0x400B805C) /* Count Control read address */
#define MCCNTCON_SET LPC17_REG(0x400B8060) /* Count Control set address */
#define MCCNTCON_CLR LPC17_REG(0x400B8064) /* Count Control clear address */
#define MCINTF       LPC17_REG(0x400B8068) /* Interrupt flags read address */
#define MCINTF_SET   LPC17_REG(0x400B806C) /* Interrupt flags set address */
#define MCINTF_CLR   LPC17_REG(0x400B8070) /* Interrupt flags clear address */
#define MCCAP_CLR    LPC17_REG(0x400B8074) /* Capture clear address */

/* MCPWM Control read address (MCCON - 0x400B 8000) */
#define MCCON_RUN0    (0x1 << 0)
#define MCCON_CENTER0 (0x1 << 1)
#define MCCON_POLA0   (0x1 << 2)
#define MCCON_DTE0    (0x1 << 3)
#define MCCON_DISUP0  (0x1 << 4)
#define MCCON_RUN1    (0x1 << 8)
#define MCCON_CENTER1 (0x1 << 9)
#define MCCON_POLA1   (0x1 << 10)
#define MCCON_DTE1    (0x1 << 11)
#define MCCON_DISUP1  (0x1 << 12)
#define MCCON_RUN2    (0x1 << 16)
#define MCCON_CENTER2 (0x1 << 17)
#define MCCON_POLA2   (0x1 << 18)
#define MCCON_DTE2    (0x1 << 19)
#define MCCON_DISUP2  (0x1 << 20)
#define MCCON_INVBDC  (0x1 << 29)
#define MCCON_ACMODE  (0x1 << 30)
#define MCCON_DCMODE  (0x1 << 31)

/*
 * skipped:
 *
 * MCPWM Control set address (MCCON_SET - 0x400B 8004)
 * MCPWM Control clear address (MCCON_CLR - 0x400B 8008)
 */

/* MCPWM Capture Control read address (MCCAPCON - 0x400B 800C) */
#define MCCAPCON_CAP0MCI0_RE (0x1 << 0)
#define MCCAPCON_CAP0MCI0_FE (0x1 << 1)
#define MCCAPCON_CAP0MCI1_RE (0x1 << 2)
#define MCCAPCON_CAP0MCI1_FE (0x1 << 3)
#define MCCAPCON_CAP0MCI2_RE (0x1 << 4)
#define MCCAPCON_CAP0MCI2_FE (0x1 << 5)
#define MCCAPCON_CAP1MCI0_RE (0x1 << 6)
#define MCCAPCON_CAP1MCI0_FE (0x1 << 7)
#define MCCAPCON_CAP1MCI1_RE (0x1 << 8)
#define MCCAPCON_CAP1MCI1_FE (0x1 << 9)
#define MCCAPCON_CAP1MCI2_RE (0x1 << 10)
#define MCCAPCON_CAP1MCI2_FE (0x1 << 11)
#define MCCAPCON_CAP2MCI0_RE (0x1 << 12)
#define MCCAPCON_CAP2MCI0_FE (0x1 << 13)
#define MCCAPCON_CAP2MCI1_RE (0x1 << 14)
#define MCCAPCON_CAP2MCI1_FE (0x1 << 15)
#define MCCAPCON_CAP2MCI2_RE (0x1 << 16)
#define MCCAPCON_CAP2MCI2_FE (0x1 << 17)
#define MCCAPCON_RT0         (0x1 << 18)
#define MCCAPCON_RT1         (0x1 << 19)
#define MCCAPCON_RT2         (0x1 << 20)
#define MCCAPCON_HNFCAP0     (0x1 << 21)
#define MCCAPCON_HNFCAP1     (0x1 << 22)
#define MCCAPCON_HNFCAP2     (0x1 << 23)

/*
 * skipped:
 *
 * MCPWM Capture Control set address (MCCAPCON_SET - 0x400B 8010)
 * MCPWM Capture Control clear address (MCCAPCON_CLR - 0x400B 8014)
 */

/* MCPWM Interrupt Enable read address (MCINTEN - 0x400B 8050) */
#define MCINTEN_ILIM0 (0x1 << 0)
#define MCINTEN_IMAT0 (0x1 << 1)
#define MCINTEN_ICAP0 (0x1 << 2)
#define MCINTEN_ILIM1 (0x1 << 4)
#define MCINTEN_IMAT1 (0x1 << 5)
#define MCINTEN_ICAP1 (0x1 << 6)
#define MCINTEN_ILIM2 (0x1 << 8)
#define MCINTEN_IMAT2 (0x1 << 9)
#define MCINTEN_ICAP2 (0x1 << 10)
#define MCINTEN_ABORT (0x1 << 15)

/* MCPWM Interrupt Enable set address (MCINTEN_SET - 0x400B 8054) */
#define MCINT_SET_ILIM0 (0x1 << 0)
#define MCINT_SET_IMAT0 (0x1 << 1)
#define MCINT_SET_ICAP0 (0x1 << 2)
#define MCINT_SET_ILIM1 (0x1 << 4)
#define MCINT_SET_IMAT1 (0x1 << 5)
#define MCINT_SET_ICAP1 (0x1 << 6)
#define MCINT_SET_ILIM2 (0x1 << 8)
#define MCINT_SET_IMAT2 (0x1 << 9)
#define MCINT_SET_ICAP2 (0x1 << 10)
#define MCINT_SET_ABORT (0x1 << 15)

/* MCPWM Interrupt Enable clear address (MCINTEN_CLR - 0x400B 8058) */
#define MCINT_CLR_ILIM0 (0x1 << 0)
#define MCINT_CLR_IMAT0 (0x1 << 1)
#define MCINT_CLR_ICAP0 (0x1 << 2)
#define MCINT_CLR_ILIM1 (0x1 << 4)
#define MCINT_CLR_IMAT1 (0x1 << 5)
#define MCINT_CLR_ICAP1 (0x1 << 6)
#define MCINT_CLR_ILIM2 (0x1 << 8)
#define MCINT_CLR_IMAT2 (0x1 << 9)
#define MCINT_CLR_ICAP2 (0x1 << 10)
#define MCINT_CLR_ABORT (0x1 << 15)

/* MCPWM Interrupt Flags read address (MCINTF - 0x400B 8068) */
#define MCINTF_ILIM0 (0x1 << 0)
#define MCINTF_IMAT0 (0x1 << 1)
#define MCINTF_ICAP0 (0x1 << 2)
#define MCINTF_ILIM1 (0x1 << 4)
#define MCINTF_IMAT1 (0x1 << 5)
#define MCINTF_ICAP1 (0x1 << 6)
#define MCINTF_ILIM2 (0x1 << 8)
#define MCINTF_IMAT2 (0x1 << 9)
#define MCINTF_ICAP2 (0x1 << 10)
#define MCINTF_ABORT (0x1 << 15)

/* MCPWM Interrupt Flags set address (MCINTF_SET - 0x400B 806C) */
#define MCINTF_SET_ILIM0 (0x1 << 0)
#define MCINTF_SET_IMAT0 (0x1 << 1)
#define MCINTF_SET_ICAP0 (0x1 << 2)
#define MCINTF_SET_ILIM1 (0x1 << 4)
#define MCINTF_SET_IMAT1 (0x1 << 5)
#define MCINTF_SET_ICAP1 (0x1 << 6)
#define MCINTF_SET_ILIM2 (0x1 << 8)
#define MCINTF_SET_IMAT2 (0x1 << 9)
#define MCINTF_SET_ICAP2 (0x1 << 10)
#define MCINTF_SET_ABORT (0x1 << 15)

/* MCPWM Interrupt Flags clear address (MCINTF_CLR - 0x400B 8070) */
#define MCINTF_CLR_ILIM0 (0x1 << 0)
#define MCINTF_CLR_IMAT0 (0x1 << 1)
#define MCINTF_CLR_ICAP0 (0x1 << 2)
#define MCINTF_CLR_ILIM1 (0x1 << 4)
#define MCINTF_CLR_IMAT1 (0x1 << 5)
#define MCINTF_CLR_ICAP1 (0x1 << 6)
#define MCINTF_CLR_ILIM2 (0x1 << 8)
#define MCINTF_CLR_IMAT2 (0x1 << 9)
#define MCINTF_CLR_ICAP2 (0x1 << 10)
#define MCINTF_CLR_ABORT (0x1 << 15)

/* MCPWM Count Control read address (MCCNTCON - 0x400B 805C) */
#define MCCNTCON_TC0MCI0_RE (0x1 << 0)
#define MCCNTCON_TC0MCI0_FE (0x1 << 1)
#define MCCNTCON_TC0MCI1_RE (0x1 << 2)
#define MCCNTCON_TC0MCI1_FE (0x1 << 3)
#define MCCNTCON_TC0MCI2_RE (0x1 << 4)
#define MCCNTCON_TC0MCI2_FE (0x1 << 5)
#define MCCNTCON_TC1MCI0_RE (0x1 << 6)
#define MCCNTCON_TC1MCI0_FE (0x1 << 7)
#define MCCNTCON_TC1MCI1_RE (0x1 << 8)
#define MCCNTCON_TC1MCI1_FE (0x1 << 9)
#define MCCNTCON_TC1MCI2_RE (0x1 << 10)
#define MCCNTCON_TC1MCI2_FE (0x1 << 11)
#define MCCNTCON_TC2MCI0_RE (0x1 << 12)
#define MCCNTCON_TC2MCI0_FE (0x1 << 13)
#define MCCNTCON_TC2MCI1_RE (0x1 << 14)
#define MCCNTCON_TC2MCI1_FE (0x1 << 15)
#define MCCNTCON_TC2MCI2_RE (0x1 << 16)
#define MCCNTCON_TC2MCI2_FE (0x1 << 17)
#define MCCNTCON_CNTR0      (0x1 << 29)
#define MCCNTCON_CNTR1      (0x1 << 30)
#define MCCNTCON_CNTR2      (0x1 << 31)

/*
 * skipped:
 *
 * MCPWM Count Control set address (MCCNTCON_SET - 0x400B 8060)
 * MCPWM Count Control clear address (MCCNTCON_CLR - 0x400B 8064)
 * MCPWM Timer/Counter 0-2 registers (MCTC0-2 - 0x400B 8018, 0x400B 801C, 0x400B 8020)
 * MCPWM Limit 0-2 registers (MCLIM0-2 - 0x400B 8024, 0x400B 8028, 0x400B 802C)
 * MCPWM Match 0-2 registers (MCMAT0-2 - 0x400B 8030, 0x400B 8034, 0x400B 8038)
 */

/* MCPWM Dead-time register (MCDT - 0x400B 803C) */
#define MCDT_DT0 (0x3FF << 0)
#define MCDT_DT1 (0x3FF << 10)
#define MCDT_DT2 (0x3FF << 20)

/* MCPWM Commutation Pattern register (MCCP - 0x400B 8040) */
#define MCCP_CCPA0 (0x1 << 0)
#define MCCP_CCPB0 (0x1 << 1)
#define MCCP_CCPA1 (0x1 << 2)
#define MCCP_CCPB1 (0x1 << 3)
#define MCCP_CCPA2 (0x1 << 4)
#define MCCP_CCPB2 (0x1 << 5)

/* skipped  MCPWM Capture read addresses (MCCAP0-2 - 0x400B 8044, 0x400B 8048, 0x400B 804C) */

/* MCPWM Capture clear address (MCCAP_CLR - 0x400B 8074) */
#define MCCAP_CLR_CAP_CLR0 (0x1 << 0)
#define MCCAP_CLR_CAP_CLR1 (0x1 << 1)
#define MCCAP_CLR_CAP_CLR2 (0x1 << 2)


/* QEI registers */

/* Control registers */
#define QEICON     LPC17_REG(0x400BC000) /* Control register */
#define QEICONF    LPC17_REG(0x400BC008) /* Configuration register */
#define QEISTAT    LPC17_REG(0x400BC004) /* Encoder status register */

/* Position, index, and timer registers */
#define QEIPOS     LPC17_REG(0x400BC00C) /* Position register */
#define QEIMAXPOS  LPC17_REG(0x400BC010) /* Maximum position register */
#define CMPOS0     LPC17_REG(0x400BC014) /* position compare register 0 */
#define CMPOS1     LPC17_REG(0x400BC018) /* position compare register 1 */
#define CMPOS2     LPC17_REG(0x400BC01C) /* position compare register 2 */
#define INXCNT     LPC17_REG(0x400BC020) /* Index count register */
#define INXCMP     LPC17_REG(0x400BC024) /* Index compare register */
#define QEILOAD    LPC17_REG(0x400BC028) /* Velocity timer reload register */
#define QEITIME    LPC17_REG(0x400BC02C) /* Velocity timer register */
#define QEIVEL     LPC17_REG(0x400BC030) /* Velocity counter register */
#define QEICAP     LPC17_REG(0x400BC034) /* Velocity capture register */
#define VELCOMP    LPC17_REG(0x400BC038) /* Velocity compare register */
#define FILTER     LPC17_REG(0x400BC03C) /* Digital filter register */

/* Interrupt registers */
#define QEIINTSTAT LPC17_REG(0x400BCFE0) /* Interrupt status register */
#define QEISET     LPC17_REG(0x400BCFEC) /* Interrupt status set register */
#define QEICLR     LPC17_REG(0x400BCFE8) /* Interrupt status clear register */
#define QEIIE      LPC17_REG(0x400BCFE4) /* Interrupt enable register */
#define QEIIES     LPC17_REG(0x400BCFDC) /* Interrupt enable set register */
#define QEIIEC     LPC17_REG(0x400BCFD8) /* Interrupt enable clear register */

/* QEI Control register (QEICON - 0x400B C000) */
#define QEICON_RESP  (0x1 << 0)
#define QEICON_RESPI (0x1 << 1)
#define QEICON_RESV  (0x1 << 2)
#define QEICON_RESI  (0x1 << 3)

/* QEI Configuration register (QEICONF - 0x400B C008) */
#define QEICONF_DIRINV  (0x1 << 0)
#define QEICONF_SIGMODE (0x1 << 1)
#define QEICONF_CAPMODE (0x1 << 2)
#define QEICONF_INVINX  (0x1 << 3)

/* QEI Status register (QEISTAT - 0x400B C004) */
#define QEISTAT_DIR (0x1 << 0)

/*
 * skipped:
 *
 * QEI Position register (QEIPOS - 0x400B C00C)
 * QEI Maximum Position register (QEIMAXPOS - 0x400B C010)
 * QEI Position Compare register 0 (CMPOS0 - 0x400B C014)
 * QEI Position Compare register 1 (CMPOS1 - 0x400B C018)
 * QEI Position Compare register 2 (CMPOS2 - 0x400B C01C)
 * QEI Index Count register (INXCNT - 0x400B C020)
 * QEI Index Compare register (INXCMP - 0x400B C024)
 * QEI Timer Reload register (QEILOAD - 0x400B C028)
 * QEI Timer register (QEITIME - 0x400B C02C)
 * QEI Velocity register (QEIVEL - 0x400B C030)
 * QEI Velocity Capture register (QEICAP - 0x400B C034)
 * QEI Velocity Compare register (VELCOMP - 0x400B C038)
 * QEI Digital Filter register (FILTER - 0x400B C03C)
 */

/* QEI Interrupt Status register (QEIINTSTAT) */
#define QEIINTSTAT_INX_Int     (0x1 << 0)
#define QEIINTSTAT_TIM_Int     (0x1 << 1)
#define QEIINTSTAT_VELC_Int    (0x1 << 2)
#define QEIINTSTAT_DIR_Int     (0x1 << 3)
#define QEIINTSTAT_ERR_Int     (0x1 << 4)
#define QEIINTSTAT_ENCLK_Int   (0x1 << 5)
#define QEIINTSTAT_POS0_Int    (0x1 << 6)
#define QEIINTSTAT_POS1_Int    (0x1 << 7)
#define QEIINTSTAT_POS2_Int    (0x1 << 8)
#define QEIINTSTAT_REV_Int     (0x1 << 9)
#define QEIINTSTAT_POS0REV_Int (0x1 << 10)
#define QEIINTSTAT_POS1REV_Int (0x1 << 11)
#define QEIINTSTAT_POS2REV_Int (0x1 << 12)

/* QEI Interrupt Set register (QEISET - 0x400B CFEC) */
#define QEISET_INX_Int     (0x1 << 0)
#define QEISET_TIM_Int     (0x1 << 1)
#define QEISET_VELC_Int    (0x1 << 2)
#define QEISET_DIR_Int     (0x1 << 3)
#define QEISET_ERR_Int     (0x1 << 4)
#define QEISET_ENCLK_Int   (0x1 << 5)
#define QEISET_POS0_Int    (0x1 << 6)
#define QEISET_POS1_Int    (0x1 << 7)
#define QEISET_POS2_Int    (0x1 << 8)
#define QEISET_REV_Int     (0x1 << 9)
#define QEISET_POS0REV_Int (0x1 << 10)
#define QEISET_POS1REV_Int (0x1 << 11)
#define QEISET_POS2REV_Int (0x1 << 12)

/* QEI Interrupt Clear register (QEICLR - 0x400B CFE8) */
#define QEICLR_INX_Int     (0x1 << 0)
#define QEICLR_TIM_Int     (0x1 << 1)
#define QEICLR_VELC_Int    (0x1 << 2)
#define QEICLR_DIR_Int     (0x1 << 3)
#define QEICLR_ERR_Int     (0x1 << 4)
#define QEICLR_ENCLK_Int   (0x1 << 5)
#define QEICLR_POS0_Int    (0x1 << 6)
#define QEICLR_POS1_Int    (0x1 << 7)
#define QEICLR_POS2_Int    (0x1 << 8)
#define QEICLR_REV_Int     (0x1 << 9)
#define QEICLR_POS0REV_Int (0x1 << 10)
#define QEICLR_POS1REV_Int (0x1 << 11)
#define QEICLR_POS2REV_Int (0x1 << 12)

/* QEI Interrupt Enable register (QEIIE - 0x400B CFE4) */
#define QEIIE_INX_Int     (0x1 << 0)
#define QEIIE_TIM_Int     (0x1 << 1)
#define QEIIE_VELC_Int    (0x1 << 2)
#define QEIIE_DIR_Int     (0x1 << 3)
#define QEIIE_ERR_Int     (0x1 << 4)
#define QEIIE_ENCLK_Int   (0x1 << 5)
#define QEIIE_POS0_Int    (0x1 << 6)
#define QEIIE_POS1_Int    (0x1 << 7)
#define QEIIE_POS2_Int    (0x1 << 8)
#define QEIIE_REV_Int     (0x1 << 9)
#define QEIIE_POS0REV_Int (0x1 << 10)
#define QEIIE_POS1REV_Int (0x1 << 11)
#define QEIIE_POS2REV_Int (0x1 << 12)

/* QEI Interrupt Enable Set register (QEIIES - 0x400B CFDC) */
#define QEIIES_INX_Int     (0x1 << 0)
#define QEIIES_TIM_Int     (0x1 << 1)
#define QEIIES_VELC_Int    (0x1 << 2)
#define QEIIES_DIR_Int     (0x1 << 3)
#define QEIIES_ERR_Int     (0x1 << 4)
#define QEIIES_ENCLK_Int   (0x1 << 5)
#define QEIIES_POS0_Int    (0x1 << 6)
#define QEIIES_POS1_Int    (0x1 << 7)
#define QEIIES_POS2_Int    (0x1 << 8)
#define QEIIES_REV_Int     (0x1 << 9)
#define QEIIES_POS0REV_Int (0x1 << 10)
#define QEIIES_POS1REV_Int (0x1 << 11)
#define QEIIES_POS2REV_Int (0x1 << 12)

/* QEI Interrupt Enable Clear register (QEIIEC - 0x400B CFD8) */
#define QEIIEC_INX_Int     (0x1 << 0)
#define QEIIEC_TIM_Int     (0x1 << 1)
#define QEIIEC_VELC_Int    (0x1 << 2)
#define QEIIEC_DIR_Int     (0x1 << 3)
#define QEIIEC_ERR_Int     (0x1 << 4)
#define QEIIEC_ENCLK_Int   (0x1 << 5)
#define QEIIEC_POS0_Int    (0x1 << 6)
#define QEIIEC_POS1_Int    (0x1 << 7)
#define QEIIEC_POS2_Int    (0x1 << 8)
#define QEIIEC_REV_Int     (0x1 << 9)
#define QEIIEC_POS0REV_Int (0x1 << 10)
#define QEIIEC_POS1REV_Int (0x1 << 11)
#define QEIIEC_POS2REV_Int (0x1 << 12)


/* real-time clock (RTC) registers */

/* Miscellaneous registers */
#define RTC_ILR         LPC17_REG(0x40024000) /* Interrupt Location Register */
#define RTC_CCR         LPC17_REG(0x40024008) /* Clock Control Register */
#define RTC_CIIR        LPC17_REG(0x4002400C) /* Counter Increment Interrupt Register */
#define RTC_AMR         LPC17_REG(0x40024010) /* Alarm Mask Register */
#define RTC_AUX     LPC17_REG(0x4002405C) /* RTC Auxiliary control register */
#define RTC_AUXEN   LPC17_REG(0x40024058) /* RTC Auxiliary Enable register */

/* Consolidated time registers */
#define RTC_CTIME0      LPC17_REG(0x40024014) /* Consolidated Time Register 0 */
#define RTC_CTIME1      LPC17_REG(0x40024018) /* Consolidated Time Register 1 */
#define RTC_CTIME2      LPC17_REG(0x4002401C) /* Consolidated Time Register 2 */

/* Time counter registers */
#define RTC_SEC         LPC17_REG(0x40024020) /* Seconds Counter */
#define RTC_MIN         LPC17_REG(0x40024024) /* Minutes Register */
#define RTC_HOUR        LPC17_REG(0x40024028) /* Hours Register */
#define RTC_DOM         LPC17_REG(0x4002402C) /* Day of Month Register */
#define RTC_DOW         LPC17_REG(0x40024030) /* Day of Week Register */
#define RTC_DOY         LPC17_REG(0x40024034) /* Day of Year Register */
#define RTC_MONTH       LPC17_REG(0x40024038) /* Months Register */
#define RTC_YEAR        LPC17_REG(0x4002403C) /* Years Register */
#define RTC_CALIBRATION LPC17_REG(0x40024040) /* Calibration Value Register */

/* General purpose registers */
#define RTC_GPREG0      LPC17_REG(0x40024044) /* General Purpose Register 0 */
#define RTC_GPREG1      LPC17_REG(0x40024048) /* General Purpose Register 1 */
#define RTC_GPREG2      LPC17_REG(0x4002404C) /* General Purpose Register 2 */
#define RTC_GPREG3      LPC17_REG(0x40024050) /* General Purpose Register 3 */
#define RTC_GPREG4      LPC17_REG(0x40024054) /* General Purpose Register 4 */

/* Alarm register group */
#define RTC_ALSEC       LPC17_REG(0x40024060) /* Alarm value for Seconds */
#define RTC_ALMIN       LPC17_REG(0x40024064) /* Alarm value for Minutes */
#define RTC_ALHOUR      LPC17_REG(0x40024068) /* Alarm value for Hours */
#define RTC_ALDOM       LPC17_REG(0x4002406C) /* Alarm value for Day of Month */
#define RTC_ALDOW       LPC17_REG(0x40024070) /* Alarm value for Day of Week */
#define RTC_ALDOY       LPC17_REG(0x40024074) /* Alarm value for Day of Year */
#define RTC_ALMON       LPC17_REG(0x40024078) /* Alarm value for Months */
#define RTC_ALYEAR      LPC17_REG(0x4002407C) /* Alarm value for Year */

/* Interrupt Location Register (ILR - 0x4002 4000) */
#define RTC_ILR_RTCCIF (0x1 << 0)
#define RTC_ILR_RTCALF (0x1 << 1)

/* Clock Control Register (CCR - 0x4002 4008) */
#define RTC_CCR_CLKEN  (0x1 << 0)
#define RTC_CCR_CTCRST (0x1 << 1)
#define RTC_CCR_CCALEN (0x1 << 4)

/* Counter Increment Interrupt Register (CIIR - 0x4002 400C) */
#define RTC_CIIR_IMSEC  (0x1 << 0)
#define RTC_CIIR_IMMIN  (0x1 << 1)
#define RTC_CIIR_IMHOUR (0x1 << 2)
#define RTC_CIIR_IMDOM  (0x1 << 3)
#define RTC_CIIR_IMDOW  (0x1 << 4)
#define RTC_CIIR_IMDOY  (0x1 << 5)
#define RTC_CIIR_IMMON  (0x1 << 6)
#define RTC_CIIR_IMYEAR (0x1 << 7)

/* Alarm Mask Register (AMR - 0x4002 4010) */
#define RTC_AMR_AMRSEC  (0x1 << 0)
#define RTC_AMR_AMRMIN  (0x1 << 1)
#define RTC_AMR_AMRHOUR (0x1 << 2)
#define RTC_AMR_AMRDOM  (0x1 << 3)
#define RTC_AMR_AMRDOW  (0x1 << 4)
#define RTC_AMR_AMRDOY  (0x1 << 5)
#define RTC_AMR_AMRMON  (0x1 << 6)
#define RTC_AMR_AMRYEAR (0x1 << 7)

/* RTC Auxiliary control register (RTC_AUX - 0x4002 405C) */
#define RTC_AUX_RTC_OSCF (0x1 << 4)

/* RTC Auxiliary Enable register (RTC_AUXEN - 0x4002 4058) */
#define RTC_AUXEN_RTC_OSCFEN (0x1 << 4)

/* Consolidated Time Register 0 (CTIME0 - 0x4002 4014) */
#define RTC_CTIME0_Seconds     (0x3F << 0)
#define RTC_CTIME0_Minutes     (0x3F << 8)
#define RTC_CTIME0_Hours       (0x1F << 16)
#define RTC_CTIME0_Day_Of_Week (0x7 << 24)

/* Consolidated Time Register 1 (CTIME1 - 0x4002 4018) */
#define RTC_CTIME1_Day_of_Month (0x1F << 0)
#define RTC_CTIME1_Month        (0xF << 8)
#define RTC_CTIME1_Year         (0xFFF << 0)

/* Consolidated Time Register 2 (CTIME2 - 0x4002 401C) */
#define RTC_CTIME2_Day_of_Year (0xFFF << 0)

/* skipped Time Counter registers (SEC, MIN, etc.) */

/* Calibration register (CALIBRATION - address 0x4002 4040) */
#define RTC_CALIBRATION_CALVAL (0x1FFFF << 0)
#define RTC_CALIBRATION_CALDIR (0x1 << 17)

/*
 * skipped:
 *
 * General purpose registers 0 to 4 (GPREG0 to GPREG4 - addresses 0x4002 4044 to 0x4002 4054)
 * Alarm register group (ALSEC, ALMIN, etc.)
 */


/* watchdog timer (WDT) registers */

#define WDMOD    LPC17_REG(0x40000000) /* Watchdog mode register */
#define WDTC     LPC17_REG(0x40000004) /* Watchdog timer constant register */
#define WDFEED   LPC17_REG(0x40000008) /* Watchdog feed sequence register */
#define WDTV     LPC17_REG(0x4000000C) /* Watchdog timer value register */
#define WDCLKSEL LPC17_REG(0x40000010) /* Watchdog clock source selection register */

/* Watchdog Mode register (WDMOD - 0x4000 0000) */
#define WDMOD_WDEN    (0x1 << 0)
#define WDMOD_WDRESET (0x1 << 1)
#define WDMOD_WDTOF   (0x1 << 2)
#define WDMOD_WDINT   (0x1 << 3)

/*
 * skipped:
 *
 * Watchdog Timer Constant register (WDTC - 0x4000 0004)
 * Watchdog Timer Value register (WDTV - 0x4000 000C)
 */

/* Watchdog Feed register (WDFEED - 0x4000 0008) */
#define WDFEED_SEQUENCE WDFEED = 0xAA; WDFEED = 0x55

/* Watchdog Timer Clock Source Selection register (WDCLKSEL - 0x4000 0010) */
#define WDCLKSEL_WDSEL  (0x3 << 0)
#define WDCLKSEL_WDLOCK (0x1 << 31)


/* ADC registers */

#define AD0CR    LPC17_REG(0x40034000) /* ADCR    A/D Control Register */
#define AD0GDR   LPC17_REG(0x40034004) /* ADGDR   A/D Global Data Register */
#define AD0INTEN LPC17_REG(0x4003400C) /* ADINTEN A/D Interrupt Enable Register */
#define AD0DR0   LPC17_REG(0x40034010) /* ADDR0   A/D Channel 0 Data Register */
#define AD0DR1   LPC17_REG(0x40034014) /* ADDR1   A/D Channel 1 Data Register */
#define AD0DR2   LPC17_REG(0x40034018) /* ADDR2   A/D Channel 2 Data Register */
#define AD0DR3   LPC17_REG(0x4003401C) /* ADDR3   A/D Channel 3 Data Register */
#define AD0DR4   LPC17_REG(0x40034020) /* ADDR4   A/D Channel 4 Data Register */
#define AD0DR5   LPC17_REG(0x40034024) /* ADDR5   A/D Channel 5 Data Register */
#define AD0DR6   LPC17_REG(0x40034028) /* ADDR6   A/D Channel 6 Data Register */
#define AD0DR7   LPC17_REG(0x4003402C) /* ADDR7   A/D Channel 7 Data Register */
#define AD0STAT  LPC17_REG(0x40034030) /* ADSTAT  A/D Status Register */
#define AD0TRM   LPC17_REG(0x40034034) /* ADTRM   ADC trim register */

/* A/D Control Register (AD0CR - 0x4003 4000) */
#define AD0CR_SEL    (0xFF << 0)
#define AD0CR_CLKDIV (0xFF << 8)
#define AD0CR_BURST  (0x1 << 16)
#define AD0CR_PDN    (0x1 << 21)
#define AD0CR_START  (0x7 << 24)
#define AD0CR_EDGE   (0x1 << 27)

/* A/D Global Data Register (AD0GDR - 0x4003 4004) */
#define AD0GDR_RESULT  (0xFFF << 4)
#define AD0GDR_CHN     (0x7 << 24)
#define AD0GDR_OVERRUN (0x1 << 30)
#define AD0GDR_DONE    (0x1 << 31)

/* A/D Interrupt Enable register (AD0INTEN - 0x4003 400C) */
#define AD0INTEN_ADINTEN0 (0x1 << 0)
#define AD0INTEN_ADINTEN1 (0x1 << 1)
#define AD0INTEN_ADINTEN2 (0x1 << 2)
#define AD0INTEN_ADINTEN3 (0x1 << 3)
#define AD0INTEN_ADINTEN4 (0x1 << 4)
#define AD0INTEN_ADINTEN5 (0x1 << 5)
#define AD0INTEN_ADINTEN6 (0x1 << 6)
#define AD0INTEN_ADINTEN7 (0x1 << 7)
#define AD0INTEN_ADGINTEN (0x1 << 8)

/* A/D Data Registers (AD0DR0 to AD0DR7 - 0x4003 4010 to 0x4003 402C) */
#define AD0DRn_RESULT  (0xFFF << 4)
#define AD0DRn_OVERRUN (0x1 << 30)
#define AD0DRn_DONE    (0x1 << 31)

/* A/D Status register (ADSTAT - 0x4003 4030) */
#define ADSTAT_DONE0    (0x1 << 0)
#define ADSTAT_DONE1    (0x1 << 1)
#define ADSTAT_DONE2    (0x1 << 2)
#define ADSTAT_DONE3    (0x1 << 3)
#define ADSTAT_DONE4    (0x1 << 4)
#define ADSTAT_DONE5    (0x1 << 5)
#define ADSTAT_DONE6    (0x1 << 6)
#define ADSTAT_DONE7    (0x1 << 7)
#define ADSTAT_OVERRUN0 (0x1 << 8)
#define ADSTAT_OVERRUN1 (0x1 << 9)
#define ADSTAT_OVERRUN2 (0x1 << 10)
#define ADSTAT_OVERRUN3 (0x1 << 11)
#define ADSTAT_OVERRUN4 (0x1 << 12)
#define ADSTAT_OVERRUN5 (0x1 << 13)
#define ADSTAT_OVERRUN6 (0x1 << 14)
#define ADSTAT_OVERRUN7 (0x1 << 15)
#define ADSTAT_ADINT    (0x1 << 16)

/* A/D Trim register (ADTRIM - 0x4003 4034) */
#define ADTRIM_ADCOFFS (0xF << 4)
#define ADTRIM_TRIM    (0xF << 8)


/* DAC registers */

#define DACR      LPC17_REG(0x4008C000) /* D/A Converter Register */
#define DACCTRL   LPC17_REG(0x4008C004) /* DAC Control register */
#define DACCNTVAL LPC17_REG(0x4008C008) /* DAC Counter Value register */

/* D/A Converter Register (DACR - 0x4008 C000) */
#define DACR_VALUE (0x3FF << 6)
#define DACR_BIAS  (0x1 << 16)

/* D/A Converter Control register (DACCTRL - 0x4008 C004) */
#define DACCTRL_INT_DMA_REQ (0x1 << 0)
#define DACCTRL_DBLBUF_ENA  (0x1 << 1)
#define DACCTRL_CNT_ENA     (0x1 << 2)
#define DACCTRL_DMA_ENA     (0x1 << 3)

/* skipped D/A Converter Counter Value register (DACCNTVAL - 0x4008 C008) */


/* GPDMA registers */

/* General registers */
#define DMACIntStat       LPC17_REG(0x50004000) /* DMA Interrupt Status Register */
#define DMACIntTCStat     LPC17_REG(0x50004004) /* DMA Interrupt Terminal Count Request Status Register */
#define DMACIntTCClear    LPC17_REG(0x50004008) /* DMA Interrupt Terminal Count Request Clear Register */
#define DMACIntErrStat    LPC17_REG(0x5000400C) /* DMA Interrupt Error Status Register */
#define DMACIntErrClr     LPC17_REG(0x50004010) /* DMA Interrupt Error Clear Register */
#define DMACRawIntTCStat  LPC17_REG(0x50004014) /* DMA Raw Interrupt Terminal Count Status Register */
#define DMACRawIntErrStat LPC17_REG(0x50004018) /* DMA Raw Error Interrupt Status Register */
#define DMACEnbldChns     LPC17_REG(0x5000401C) /* DMA Enabled Channel Register */
#define DMACSoftBReq      LPC17_REG(0x50004020) /* DMA Software Burst Request Register */
#define DMACSoftSReq      LPC17_REG(0x50004024) /* DMA Software Single Request Register */
#define DMACSoftLBReq     LPC17_REG(0x50004028) /* DMA Software Last Burst Request Register */
#define DMACSoftLSReq     LPC17_REG(0x5000402C) /* DMA Software Last Single Request Register */
#define DMACConfig        LPC17_REG(0x50004030) /* DMA Configuration Register */
#define DMACSync          LPC17_REG(0x50004034) /* DMA Synchronization Register */
#define DMAREQSEL         LPC17_REG(0x400FC1C4) /* Selects between UART and timer DMA requests */

/* Channel 0 registers */
#define DMACC0SrcAddr     LPC17_REG(0x50004100) /* DMA Channel 0 Source Address Register */
#define DMACC0DestAddr    LPC17_REG(0x50004104) /* DMA Channel 0 Destination Address Register */
#define DMACC0LLI         LPC17_REG(0x50004108) /* DMA Channel 0 Linked List Item Register */
#define DMACC0Control     LPC17_REG(0x5000410C) /* DMA Channel 0 Control Register */
#define DMACC0Config      LPC17_REG(0x50004110) /* DMA Channel 0 Configuration Register */

/* Channel 1 registers */
#define DMACC1SrcAddr     LPC17_REG(0x50004120) /* DMA Channel 1 Source Address Register */
#define DMACC1DestAddr    LPC17_REG(0x50004124) /* DMA Channel 1 Destination Address Register */
#define DMACC1LLI         LPC17_REG(0x50004128) /* DMA Channel 1 Linked List Item Register */
#define DMACC1Control     LPC17_REG(0x5000412C) /* DMA Channel 1 Control Register */
#define DMACC1Config      LPC17_REG(0x50004130) /* DMA Channel 1 Configuration Register */

/* Channel 2 registers */
#define DMACC2SrcAddr     LPC17_REG(0x50004140) /* DMA Channel 2 Source Address Register */
#define DMACC2DestAddr    LPC17_REG(0x50004144) /* DMA Channel 2 Destination Address Register */
#define DMACC2LLI         LPC17_REG(0x50004148) /* DMA Channel 2 Linked List Item Register */
#define DMACC2Control     LPC17_REG(0x5000414C) /* DMA Channel 2 Control Register */
#define DMACC2Config      LPC17_REG(0x50004150) /* DMA Channel 2 Configuration Register */

/* Channel 3 registers */
#define DMACC3SrcAddr     LPC17_REG(0x50004160) /* DMA Channel 3 Source Address Register */
#define DMACC3DestAddr    LPC17_REG(0x50004164) /* DMA Channel 3 Destination Address Register */
#define DMACC3LLI         LPC17_REG(0x50004168) /* DMA Channel 3 Linked List Item Register */
#define DMACC3Control     LPC17_REG(0x5000416C) /* DMA Channel 3 Control Register */
#define DMACC3Config      LPC17_REG(0x50004170) /* DMA Channel 3 Configuration Register */

/* Channel 4 registers */
#define DMACC4SrcAddr     LPC17_REG(0x50004180) /* DMA Channel 4 Source Address Register */
#define DMACC4DestAddr    LPC17_REG(0x50004184) /* DMA Channel 4 Destination Address Register */
#define DMACC4LLI         LPC17_REG(0x50004188) /* DMA Channel 4 Linked List Item Register */
#define DMACC4Control     LPC17_REG(0x5000418C) /* DMA Channel 4 Control Register */
#define DMACC4Config      LPC17_REG(0x50004190) /* DMA Channel 4 Configuration Register */

/* Channel 5 registers */
#define DMACC5SrcAddr     LPC17_REG(0x500041A0) /* DMA Channel 5 Source Address Register */
#define DMACC5DestAddr    LPC17_REG(0x500041A4) /* DMA Channel 5 Destination Address Register */
#define DMACC5LLI         LPC17_REG(0x500041A8) /* DMA Channel 5 Linked List Item Register */
#define DMACC5Control     LPC17_REG(0x500041AC) /* DMA Channel 5 Control Register */
#define DMACC5Config      LPC17_REG(0x500041B0) /* DMA Channel 5 Configuration Register */

/* Channel 6 registers */
#define DMACC6SrcAddr     LPC17_REG(0x500041C0) /* DMA Channel 6 Source Address Register */
#define DMACC6DestAddr    LPC17_REG(0x500041C4) /* DMA Channel 6 Destination Address Register */
#define DMACC6LLI         LPC17_REG(0x500041C8) /* DMA Channel 6 Linked List Item Register */
#define DMACC6Control     LPC17_REG(0x500041CC) /* DMA Channel 6 Control Register */
#define DMACC6Config      LPC17_REG(0x500041D0) /* DMA Channel 6 Configuration Register */

/* Channel 7 registers */
#define DMACC7SrcAddr     LPC17_REG(0x500041E0) /* DMA Channel 7 Source Address Register */
#define DMACC7DestAddr    LPC17_REG(0x500041E4) /* DMA Channel 7 Destination Address Register */
#define DMACC7LLI         LPC17_REG(0x500041E8) /* DMA Channel 7 Linked List Item Register */
#define DMACC7Control     LPC17_REG(0x500041EC) /* DMA Channel 7 Control Register */
#define DMACC7Config      LPC17_REG(0x500041F0) /* DMA Channel 7 Configuration Register */

/*
 * skipped:
 *
 * DMA Interrupt Status register (DMACIntStat - 0x5000 4000)
 * DMA Interrupt Terminal Count Request Status register (DMACIntTCStat - 0x5000 4004)
 * DMA Interrupt Terminal Count Request Clear register (DMACIntTCClear - 0x5000 4008)
 * DMA Interrupt Error Status register (DMACIntErrStat - 0x5000 400C)
 * DMA Interrupt Error Clear register (DMACIntErrClr - 0x5000 4010)
 * DMA Raw Interrupt Terminal Count Status register (DMACRawIntTCStat - 0x5000 4014)
 * DMA Raw Interrupt Terminal Count Status register (DMACRawIntTCStat - 0x5000 4014)
 * DMA Enabled Channel register (DMACEnbldChns - 0x5000 401C)
 * DMA Software Burst Request register (DMACSoftBReq - 0x5000 4020)
 * DMA Software Single Request register (DMACSoftSReq - 0x5000 4024)
 * DMA Software Last Burst Request register (DMACSoftLBReq - 0x5000 4028)
 * DMA Software Last Single Request register (DMACSoftLSReq - 0x5000 402C)
 */

/* DMA Configuration register (DMACConfig - 0x5000 4030) */
#define DMACConfig_E (0x1 << 0)
#define DMACConfig_M (0x1 << 1)

/* skipped DMA Synchronization register (DMACSync - 0x5000 4034) */

/* DMA Request Select register (DMAReqSel - 0x400F C1C4) */
#define DMAReqSel_DMASEL08 (0x1 << 0)
#define DMAReqSel_DMASEL09 (0x1 << 1)
#define DMAReqSel_DMASEL10 (0x1 << 2)
#define DMAReqSel_DMASEL11 (0x1 << 3)
#define DMAReqSel_DMASEL12 (0x1 << 4)
#define DMAReqSel_DMASEL13 (0x1 << 5)
#define DMAReqSel_DMASEL14 (0x1 << 6)
#define DMAReqSel_DMASEL15 (0x1 << 7)

/*
 * skipped:
 *
 * DMA Channel Source Address registers (DMACCxSrcAddr - 0x5000 41x0)
 * DMA Channel Destination Address registers (DMACCxDestAddr - 0x5000 41x4)
 * DMA Channel Linked List Item registers (DMACCxLLI - 0x5000 41x8)
 */

/* DMA channel control registers (DMACCxControl - 0x5000 41xC) */
#define DMACCxControl_TransferSize (0xFFF << 0)
#define DMACCxControl_SBSize       (0x7 << 12)
#define DMACCxControl_DBSize       (0x7 << 15)
#define DMACCxControl_SWidth       (0x7 << 18)
#define DMACCxControl_DWidth       (0x7 << 21)
#define DMACCxControl_SI           (0x1 << 26)
#define DMACCxControl_DI           (0x1 << 27)
#define DMACCxControl_Prot1        (0x1 << 28)
#define DMACCxControl_Prot2        (0x1 << 29)
#define DMACCxControl_Prot3        (0x1 << 30)
#define DMACCxControl_I            (0x1 << 31)

/* DMA Channel Configuration registers (DMACCxConfig - 0x5000 41x0) */
#define DMACCxConfig_E              (0x1 << 0)
#define DMACCxConfig_SrcPeripheral  (0x1F << 1)
#define DMACCxConfig_DestPeripheral (0x1F << 6)
#define DMACCxConfig_TransferType   (0x7 << 11)
#define DMACCxConfig_IE             (0x1 << 14)
#define DMACCxConfig_ITC            (0x1 << 15)
#define DMACCxConfig_L              (0x1 << 16)
#define DMACCxConfig_A              (0x1 << 17)
#define DMACCxConfig_H              (0x1 << 18)


/* FMC registers */

#define FMSSTART  LPC17_REG(0x40084020) /* Signature start address register */
#define FMSSTOP   LPC17_REG(0x40084024) /* Signature stop-address register */
#define FMSW0     LPC17_REG(0x4008402C) /* 128-bit signature Word 0 */
#define FMSW1     LPC17_REG(0x40084030) /* 128-bit signature Word 1 */
#define FMSW2     LPC17_REG(0x40084034) /* 128-bit signature Word 2 */
#define FMSW3     LPC17_REG(0x40084038) /* 128-bit signature Word 3 */
#define FMSTAT    LPC17_REG(0x40084FE0) /* Signature generation status register */
#define FMSTATCLR LPC17_REG(0x40084FE8) /* Signature generation status clear register */

/* skipped Flash Module Signature Start register (FMSSTART - 0x4008 4020) */

/* Flash Module Signature Stop register (FMSSTOP - 0x4008 4024) */
#define FMSSTOP_STOP      (0x1FFFF << 0)
#define FMSSTOP_SIG_START (0x1 << 17)

/*
 * skipped:
 *
 * FMSW0 register bit description (FMSW0, address: 0x4008 402C)
 * FMSW1 register bit description (FMSW1, address: 0x4008 4030)
 * FMSW2 register bit description (FMSW2, address: 0x4008 4034)
 * FMSW3 register bit description (FMSW3, address: 0x4008 4038)
 */

/* Flash module Status register (FMSTAT - 0x4008 4FE0) */
#define FMSTAT_SIG_DONE (0x1 << 2)

/* Flash Module Status Clear register (FMSTATCLR - 0x0x4008 4FE8) */
#define FMSTATCLR_SIG_DONE_CLR (0x1 << 2)

#endif /* __LPC17_H */
