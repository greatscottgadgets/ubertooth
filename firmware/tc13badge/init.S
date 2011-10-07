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

.include "r8c2l.inc"

	.section ".vec", "a"
	.3byte	0	; UND
	.byte	0xff
	.3byte	0	; overflow
	.byte	0xff
	.3byte	0	; BRK, see also ivect
	.byte	0xff

	.section ".resetvec", "a"
	.global	_reset
_reset:
	.3byte	_start
	.byte	0xff		; watchdog settings.

	.text
	.global _start
_start:
	fclr	U	/* Interrupt stack gets the big main stack.  */
	ldc	#__stack,sp
; We're going to use a combined stack, because we're low on memory.
;	fset	U	/* User stack gets the little one. */
;	ldc	#__istack,sp

	ldc	#%lo16(_ivects),intbl
	ldc	#%hi16(_ivects),intbh

; Copy ROM data to RAM
	mov.b	#%hi8(__romdatastart),r1h
	mov.w	#%lo16(__romdatastart),a0
	mov.w	#__datastart,a1
	mov.w	#__romdatacopysize,r3
	shl.w	#-1,r3
	smovf.w

	mov.w	#__bssstart,a1
	mov.w	#__bsssize,r3
	shl.w	#-1,r3
	mov.w	#0,r0
	sstr.w

; Run the main application

	jmp.w	_main

	.global __reset
__reset:
	.global __exit
__exit:
	bset	$prc1, prcr
	bset	$pm03, pm0
	jmp.b	__exit

	.bss
	.global	__impure_ptr
__impure_ptr:
	.space	10
	.global	mem0
mem0:	.space	1
	.global	mem1
mem1:	.space	1
	.global	mem2
mem2:	.space	1
	.global	mem3
mem3:	.space	1
	.global	mem4
mem4:	.space	1
	.global	mem5
mem5:	.space	1
	.global	mem6
mem6:	.space	1
	.global	mem7
mem7:	.space	1
	.global	mem8
mem8:	.space	1
	.global	mem9
mem9:	.space	1
	.global	mem10
mem10:	.space	1
	.global	mem11
mem11:	.space	1
	.global	mem12
mem12:	.space	1
	.global	mem13
mem13:	.space	1
	.global	mem14
mem14:	.space	1
	.global	mem15
mem15:	.space	1

