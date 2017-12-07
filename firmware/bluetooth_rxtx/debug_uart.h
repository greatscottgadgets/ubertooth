/*
 * Copyright 2017 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#ifndef __DEBUG_UART_H__
#define __DEBUG_UART_H__

void debug_uart_init(int flow_control);
void debug_write(char *fmt);
void debug_printf(char *fmt, ...);

#endif
