/*
 * Copyright 2011 Michael Ossmann
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

#include "tc13badge.h"
#include "cc2400.h"
#include "bitmaps.h"

void delay(uint16_t duration);
void sleep(void);
void pulse_leds(uint8_t led);
void timer_ra_init(void);
void timer_ra_stop(void);
void timer_ra_start(void);
void r8c_init(void);
void specan_init(void);
void led_specan(void);
void post(void);
void main(void);
int logo_update(void);
int icon_update(void);
void pov_logo(void);
void pov_icon(void);
void easter_egg(void);

uint8_t tests_completed = 0;
int easter_threshold = -169;
uint8_t easter_is_coming = 0;
volatile int timesup = 0;
volatile int button_pressed = 0;
uint8_t pwm[13];

/* interrupt service routine for INT3 (button SW1) */
void __attribute__((interrupt)) int3_handler(void)
{
	button_pressed++;
}

/* interrupt service routine for INT0 (R8C_CTL) */
void __attribute__((interrupt)) int0_handler(void)
{
}

/* interrupt service routine for Timer RA */
void __attribute__((interrupt)) timer_ra_handler(void)
{
	timesup++;
}

void delay(uint16_t duration)
{
	int i, j;
	for (i = duration; i; i--)
		for (j = 13; j; j--)
			asm("nop");
}

void sleep(void)
{
	all_leds_off();
	uart_off();
	cc2400_off();
	button_pressed = 0;
	stop_until_button();
	cc2400_init();
	uart_init();
	cc2400_clock_start();
	pulse_leds(13);
}

void pulse_leds(uint8_t led)
{
	int i, j;

	for (i = 455; i; i--) {
		for (j = 0; j < 13; j++) {
			set_led(j, (j < led));
			delay(1);
			set_led(j, 0);
		}
	}
}

// note: vect = 22
void timer_ra_init(void)
{
	DISABLE_INTERRUPTS;
	tracr.tstart = 0;
	while(tracr.tcstf);
	traic = 0;       // disable timer RA interrupts
	tracr.tstop = 1; // initialize timer RA registers
	trapre = 7;
	tra = 13;
	traioc.b = 0x00;
	tramr.b = 0x20;  // timer mode counting fOCO
	traic = 3;       // set timer RA interrupt priority
	FOUR_NOPS;
	ENABLE_INTERRUPTS;
}

void timer_ra_stop(void)
{
	tracr.tstart = 0;
	while(tracr.tcstf);
}

void timer_ra_start(void)
{
	timer_ra_stop();
	timesup = 0;
	tracr.tstart = 1;
	while(!tracr.tcstf);
}

void r8c_init(void)
{
	DISABLE_INTERRUPTS;
	prcr.prc0 = 1;        // disable control register protection
	fra2 = 0x00;          // divide high speed clock by 2 (to 20 MHz)
	fra0.b0 = 1;          // turn on high speed oscillator
	delay(13);            // wait for high speed oscillator to stabilize
	fra0.b1 = 1;          // switch to high speed oscillator
	ocd.ocd2 = 1;         // use internal oscillator for system clock
	cm1.b = cm1.b & 0x3f; // no CPU clock division
	cm0.b6 = 0;           // enable CPU clock division setting
	cm0.b2 = 1;           // peripheral clock stops in WAIT mode
	prcr.prc0 = 0;        // enable control register protection
	trbmr.tckcut = 1;     // cut Timer RB count source for power consumption
	adcon1.vcut = 0;      // cut VREF connection for power consumption
	ENABLE_INTERRUPTS;
}

void specan_init(void)
{
	int i;

	uart_init();
	cc2400_set(MANAND,  0x7fbf); // don't power down oscillator
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	cc2400_set(RSSI,    0x00F0); // fastest RSSI setting
	uart_off();

	for (i = 0; i < 13; i++)
		pwm[i] = 0;
}

inline void sweep(uint8_t low, uint8_t high, uint16_t base, uint8_t* store)
{
	const char floor = -30;
	char rssi;
	uint16_t channel;

	uint8_t i;

	for (i = low; i < high; i++) {
		channel = base + (i*5);
		//while ((cc2400_status() & FS_LOCK));
		cc2400_set(FSDIV, channel - 1); // tune 1 MHz below because of 1 MHz IF
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));
			//cc2400_strobe(SFSON);
		cc2400_strobe(SRX);
		delay(1);
		rssi = cc2400_get(RSSI) >> 8;
		cc2400_strobe(SRFOFF);
		if (rssi > floor)
			store[i] = MAX(store[i], (rssi - floor) >> 1);

		if (channel == 2472) {
			easter_threshold = rssi;
		} else if ((channel == 2470) || (channel == 2474)) {
			if ((rssi + 3) < easter_threshold) {
				if (easter_is_coming < 169) {
					easter_is_coming++;
				}
			} else if (easter_is_coming > 1) {
				easter_is_coming -= 2;
			}
		}
	}
}

/* LED based spectrum analysis */
void led_specan(void)
{
	uint8_t port1, port2, port3;
	uint8_t i, j, k;
	uint8_t radio_on = 0;
	uint16_t loop = 169;
	const uint16_t base = 2410; // lowest channel measured

	i = 0;
	j = 0;

	while (loop--) {
		if (i < 12) {
			port1 = 1;
			port2 = 0;
			port3 = 0;
			if (pwm[0] > i) port3 |= (1 << 5);
			if (pwm[1] > (12 - i)) port3 |= (1 << 4);
			if (pwm[2] > i) port1 |= (1 << 1);
			if (pwm[3] > (12 - i)) port1 |= (1 << 2);
			if (pwm[4] > i) port1 |= (1 << 7);
			if (pwm[5] > (12 - i)) port2 |= (1 << 0);
			if (pwm[6] > i) port2 |= (1 << 2);
			if (pwm[7] > (12 - i)) port2 |= (1 << 1);
			if (pwm[8] > i) port2 |= (1 << 3);
			if (pwm[9] > (12 - i)) port2 |= (1 << 4);
			if (pwm[10] > i) port2 |= (1 << 5);
			if (pwm[11] > (12 - i)) port2 |= (1 << 6);
			if (pwm[12] > i) port2 |= (1 << 7);
			p1.b = port1;
			p2.b = port2;
			p3.b = port3;
			// above is just a faster version of this:
			//for (k = 0; k < 13; k++)
				//set_led(k, (pwm[k] > i));

			/* delay in low power WAIT mode */
			prcr.b = 0x09;    // disable control register protection
			vca2.b0 = 1;      // enable voltage detector low power mode
			fra0.b1 = 0;      // switch to low speed oscillator
			fra0.b0 = 0;      // turn off high speed oscillator
			tracr.tstart = 1; // start timer
			WAIT;
			FOUR_NOPS;
			fra0.b0 = 1;      // turn on high speed oscillator
			fra0.b1 = 1;      // switch to high speed oscillator
			vca2.b0 = 0;      // disable voltage detector low power mode
			prcr.b = 0x00;    // enable control register protection
			timer_ra_stop();  // stop timer

		} else {
			p1.b = 1;
			p2.b = 0;
			p3.b = 0;
			// above is a faster all_leds_off()

			/* measure signal strength on 13 channels */
			uart_init();
			cc2400_strobe(SXOSCON);
			while (!(cc2400_status() & XOSC16M_STABLE));
			sweep(0, 13, base+j, &pwm[0]);
			while ((cc2400_status() & FS_LOCK));
			cc2400_strobe(SXOSCOFF);
			uart_off();
		}

		i = (i+1) % 13;
		if (i == 0) {
			j = (j+1) % 5;
			/* fade out */
			for (k = 0; k < 13; k++)
				if (pwm[k] > 0)
					pwm[k]--;
		}
	}
}

/* power-on self-test */
void post(void)
{
	int i = 13;

	cc2400_off();
	init_leds();
	pulse_leds(++tests_completed); // LED1

	init_button();
	pulse_leds(++tests_completed); // LED2

	int3_init();
	pulse_leds(++tests_completed); // LED3

	timer_ra_init();
	pulse_leds(++tests_completed); // LED4

	prcr.prc3 = 1;
	vca2.b5 = 1;
	prcr.prc3 = 0;
	while(!vca2.b5);
	pulse_leds(++tests_completed); // LED5

	cc2400_init();
	pulse_leds(++tests_completed); // LED6

	uart_init();
	pulse_leds(++tests_completed); // LED7

	cc2400_set(MAIN, 0x0000);
	while (cc2400_get(MAIN) != 0x0000) delay(13);
	pulse_leds(++tests_completed); // LED8

	cc2400_set(MAIN, 0x8000);
	while (cc2400_get(MAIN) != 0x8000) delay(13);
	pulse_leds(++tests_completed); // LED9

	cc2400_register_test();
	pulse_leds(++tests_completed); // LED10

	cc2400_clock_start();
	pulse_leds(++tests_completed); // LED11

	/* only run the range test if requested by user pressing the button */
	if (button_pressed) {
		if (cc2400_range_test()) {
			++tests_completed; // LED13
		}
		pulse_leds(++tests_completed); // LED12
	}
	button_pressed = 0;

	/* give the user a second to see the final test results */
	while ((!button_pressed) && (i--))
		pulse_leds(tests_completed);
}

void main(void)
{
	int i;

	tests_completed = 0;
	timesup = 0;
	button_pressed = 0;
	easter_threshold = 0;
	easter_is_coming = 0;

	r8c_init();
	r8c_takeover_init();
	post();

	if ((!button_pressed) && R8C_CTL)
		sleep();

	while (1) {
		specan_init();
		delay(28561);
		button_pressed = 0;
		for (i = 2197; i; i--) {
			if (!R8C_CTL) {
				r8c_takeover();
				while (!R8C_CTL)
					stop_until_button();
				r8c_take_back();
				specan_init();
				button_pressed = 0;
				easter_is_coming = 0;
				i = 2197;
			}
			led_specan();
			if (easter_is_coming == 169) {
				easter_is_coming = 0;
				easter_egg();
				break;
			}
			if (button_pressed) {
				delay(28561);
				break;
			}
		}
		if (R8C_CTL)
			sleep();
	}
}

int logo_update(void)
{
	static int i = 0;
	static int j = 0;
	static int m = 0;
	int k;

	for (k = 0; k < 13; k++) {
		set_led(k, (((logo[i][k] ^ key[m]) >> j) & 0x01));
		m = (m + 1) % KEY_LEN;
	}

	j = (j + 1) % 8;
	i = (i + 1) % LOGO_LEN;

	return i;
}

int icon_update(void)
{
	static int i = 0;
	static int j = 0;
	static int m = 0;
	int k;

	for (k = 0; k < 13; k++) {
		set_led(k, (((icon[i][k] ^ key[m]) >> j) & 0x01));
		m = (m + 1) % KEY_LEN;
	}

	j = (j + 1) % 8;
	i = (i + 1) % ICON_LEN;
	if (i == 0)
		m = 0;

	return i;
}

void pov_logo(void)
{
	delay(2197);

	while (logo_update())
		delay(13);
	all_leds_off();

	delay(28561);
}

void pov_icon(void)
{
	delay(2197);

	while (icon_update())
		delay(13);
	all_leds_off();

	delay(28561);
}

void easter_egg(void)
{
	uart_off();
	cc2400_off();

	pov_logo();
	stop_until_button();
	pov_icon();
	stop_until_button();
	pov_logo();
	stop_until_button();
	pov_icon();
	stop_until_button();
	pov_logo();
	stop_until_button();
	pov_icon();
	stop_until_button();
	pov_logo();
	stop_until_button();
	pov_icon();
	stop_until_button();
	pov_logo();
	stop_until_button();
	pov_icon();
	stop_until_button();
	pov_logo();
	stop_until_button();
	pov_icon();
	stop_until_button();
	pov_logo();
	stop_until_button();

	cc2400_init();
	uart_init();
	cc2400_clock_start();
	button_pressed = 0;
}
