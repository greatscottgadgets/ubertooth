/*
  Copyright (C) 2011 Michael Ossmann <mike@ossmann.com>
  Copyright (C) 2010 DJ Delorie <dj@redhat.com>

  This file is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 3, or (at your option) any later
  version.

  This file is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License
  along with this file; see the file COPYING3.  If not see
  <http://www.gnu.org/licenses/>.
*/

	.text

	.global	_default_isr
_default_isr:
	jmp.b	_default_isr

	.global	_ivects
_ivects:
	;; 0
	.long	_brk_handler       ; brk
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	_timer_rc_handler  ; timer_rc
	;; 8
	.long	_timer_rd0_handler ; timer_rd0
	.long	_timer_rd1_handler ; timer_rd1
	.long	0
	.long	_s2tx_handler      ; s2tx
	.long	_s2rx_handler      ; s2rx
	.long	_key_handler       ; key
	.long	_adc_handler       ; adc
	.long	0
	;; 16
	.long	0
	.long	_s0tx_handler      ; s0tx
	.long	_s0rx_handler      ; s0rx
	.long	0
	.long	0
	.long	0
	.long	_timer_ra_handler  ; timer_ra
	.long	0
	;; 24
	.long	_timer_rb_handler  ; timer_rb
	.long	_int1_handler      ; int1
	.long	_int3_handler      ; int3
	.long	0
	.long	0
	.long	_int0_handler      ; int0
	.long	0
	.long	0
	;; the rest are software interrupts only, which we don't use.
