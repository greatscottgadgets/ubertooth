/*
 * R5F212L4: R8C/2L with 16k program flash, 2k data flash, 1.5k RAM
 *
 * adapted from one or two of DJ Delorie's linker scripts
 */

OUTPUT_FORMAT("elf32-m32c", "elf32-m32c", "elf32-m32c")
OUTPUT_ARCH(m32c)
ENTRY(_start)

MEMORY
{
	RAM (w) : ORIGIN = 0x400, LENGTH = 0x600
	DROM (r) : ORIGIN = 0x2400, LENGTH = 0x800
	ROM (r) : ORIGIN = 0xC000, LENGTH = 0x3f24
	VEC (r) : ORIGIN = 0xffdc, LENGTH = 32
	RESETVEC (r) : ORIGIN = 0xfffc, LENGTH = 4
}

SECTIONS
{
	.text : {
		*(.text)
		PROVIDE (__etext = .);
		. = ALIGN(2);
		*(.rodata)
		. = ALIGN(2);
		__romdatastart = .;
	} > ROM =0

	.data : {
		__datastart = .;
		. = ALIGN(2);
		*(.data)
		. = ALIGN(2);
		_edata = .;
	} > RAM AT>ROM
	PROVIDE (__romdatacopysize =SIZEOF(.data));

	.bss : {
		. = ALIGN(2);
		PROVIDE (__bssstart = .);
		*(.bss)
		*(COMMON)
		. = ALIGN(2);
		PROVIDE (__bssend = .);
	} > RAM
	__bsssize = SIZEOF(.bss);

	.stack (0x7fe) :
	{
		PROVIDE (__stack = .);
		*(.stack)
	}

	.drom : {
		*(.drom)
	} > DROM
	.vec : {
		*(.vec)
	} > VEC
	.resetvec : {
		*(.resetvec)
	} > RESETVEC

	PROVIDE (_brk_handler = 0);
	PROVIDE (_timer_rc_handler = 0);
	PROVIDE (_timer_rd0_handler = 0);
	PROVIDE (_timer_rd1_handler = 0);
	PROVIDE (_s2tx_handler = 0);
	PROVIDE (_s2rx_handler = 0);
	PROVIDE (_key_handler = 0);
	PROVIDE (_adc_handler = 0);
	PROVIDE (_s0tx_handler = 0);
	PROVIDE (_s0rx_handler = 0);
	PROVIDE (_timer_ra_handler = 0);
	PROVIDE (_timer_rb_handler = 0);
	PROVIDE (_int1_handler = 0);
	PROVIDE (_int3_handler = 0);
	PROVIDE (_int0_handler = 0);
}
