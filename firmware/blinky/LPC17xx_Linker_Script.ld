/* Linker script for Cortex-M3
 *
 * Version:CodeSourcery Sourcery G++ Lite 2007q3-53
 * BugURL:https://support.codesourcery.com/GNUToolchain/
 *
 *  Copyright 2007 CodeSourcery.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply. */

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(_start)
SEARCH_DIR(.)
GROUP(-lgcc -lc -lcs3 -lcs3unhosted -lcs3micro)

MEMORY
{
  rom (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  ram (rwx) : ORIGIN = 0x10000000, LENGTH =  32K
}

EXTERN(Reset_Handler)
ENTRY(Reset_Handler)

SECTIONS
{
	.text :
	{
		KEEP(*(.irq_handler_table))
		*(.text*)
		*(.rodata*)

	} > rom


	.ARM.extab : 
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > rom

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > rom
	__exidx_end = .;

	_etext = .;

	.data : AT (__exidx_end)
	{
		_data = .;
		*(vtable)
		*(.data*)
		_edata = .;
	} > ram

	/* zero initialized data */
	.bss :
	{
		_bss = .;
		*(.bss*)
		*(COMMON)
		_ebss = .;
	} > ram

	/* Where we put the heap with cr_clib */
	.cr_heap :
	{
		end = .;
		_pvHeapStart = .;
	} > ram

	_StackTop = (0x10000000 + 0x8000) - 16;

}
