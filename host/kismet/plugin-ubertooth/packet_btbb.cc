/* -*- c++ -*- */
/*
 * Copyright 2010, 2011 Michael Ossmann
 * Copyright 2009, 2010 Mike Kershaw
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

#include <packetchain.h>
#include <packetsource.h>
#include <endian_magic.h>

#include "packet_btbb.h"

// From kismet_ubertooth
extern int pack_comp_btbb;

static int debugno = 0;

int kis_btbb_dissector(CHAINCALL_PARMS) {
	int offset = 0;

	btbb_packinfo *pi = NULL;

	if (in_pack->error)
		return 0;

	kis_datachunk *chunk =
		(kis_datachunk *) in_pack->fetch(_PCM(PACK_COMP_LINKFRAME));

	if (chunk == NULL)
		return 0;

	if (chunk->dlt != KDLT_BTBB)
		return 0;

	debugno++;

	if (chunk->length < 14) {
		_MSG("Short Bluetooth baseband frame!", MSGFLAG_ERROR);
		in_pack->error = 1;
		return 0;
	}

	pi = new btbb_packinfo();

	pi->type = btbb_type_id;
	pi->nap = (chunk->data[6] << 8)
			| chunk->data[7];
	pi->uap = chunk->data[8];
	pi->lap = (chunk->data[9] << 16)
			| (chunk->data[10] << 8)
			| chunk->data[11];

	//printf("Bluetooth Baseband Packet %d\n", debugno);

	in_pack->insert(pack_comp_btbb, pi);

	return 1;
}
