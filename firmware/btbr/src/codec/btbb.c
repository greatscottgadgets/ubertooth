/* -*- c -*- */
/*
 * Copyright 2007 - 2013 Dominic Spill, Michael Ossmann, Will Code
 *
 * This file is part of libbtbb
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
 * along with libbtbb; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <stdint.h>

/* default codeword modified for PN sequence and barker code */
#define DEFAULT_CODEWORD 0xb0000002c7820e7eULL
/*
 * generator matrix for sync word (64,30) linear block code
 * based on polynomial 0260534236651
 * thanks to http://www.ee.unb.ca/cgi-bin/tervo/polygen.pl
 * modified for barker code
 */
static const uint64_t sw_matrix[24] = {
        0xfe000002a0d1c014ULL, 0x01000003f0b9201fULL, 0x008000033ae40edbULL, 0x004000035fca99b9ULL,
        0x002000036d5dd208ULL, 0x00100001b6aee904ULL, 0x00080000db577482ULL, 0x000400006dabba41ULL,
        0x00020002f46d43f4ULL, 0x000100017a36a1faULL, 0x00008000bd1b50fdULL, 0x000040029c3536aaULL,
        0x000020014e1a9b55ULL, 0x0000100265b5d37eULL, 0x0000080132dae9bfULL, 0x000004025bd5ea0bULL,
        0x00000203ef526bd1ULL, 0x000001033511ab3cULL, 0x000000819a88d59eULL, 0x00000040cd446acfULL,
        0x00000022a41aabb3ULL, 0x0000001390b5cb0dULL, 0x0000000b0ae27b52ULL, 0x0000000585713da9ULL};

/* Generate Sync Word from an LAP */
uint64_t btbb_gen_syncword(const uint32_t LAP)
{
        int i;
        uint64_t codeword = DEFAULT_CODEWORD;

        /* the sync word generated is in host order, not air order */
        for (i = 0; i < 24; i++)
                if (LAP & (0x800000 >> i))
                        codeword ^= sw_matrix[i];

        return codeword;
}
