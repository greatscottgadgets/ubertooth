/* -*- c++ -*- */
/*
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
#include "config.h"

#include <endian_magic.h>

#include "packet_btbb.h"

const char *btbb_type_str[] = {
	"NULL",
	"POLL",
	"FHS",
	"DM1",
	"DH1/2-DH1",
	"HV1",
	"HV2/2-EV3",
	"HV3/EV3/3-EV3",
	"DV/3-DH1",
	"AUX1",
	"DM3/2-DH3",
	"DH3/3-DH3",
	"EV4/2-EV5",
	"EV5/3-EV5",
	"DM5/2-DH5",
	"DH5/3-DH5"
};
