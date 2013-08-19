/*
 * Copyright 2013
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

#include <stdio.h>

/*
 * fp = file to output to
 * r = register number
 * v = value in the register
 * verbose = if non-zero, output reserved fields
 */
void cc2400_decode(FILE *fp, int r, unsigned short v, int verbose);

/*
 * name = register name
 *
 * Returns register number or -1 if not found
 */
int cc2400_name2reg(char *name);

/*
 * r = register number
 *
 * Returns register name
 */
char *cc2400_reg2name(int r);
