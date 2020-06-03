/* Message queue
 *
 * Copyright 2020 Etienne Helluy-Lafont, Univ. Lille, CNRS.
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
#include <stdlib.h>
#include <ubtbr/defines.h>
#include <ubtbr/debug.h>
#include <ubtbr/queue.h>

msg_t *msg_dequeue(msg_queue_t*q)
{
	msg_t *p = NULL;

	if (!msg_queue_empty(q))
	{
		p = q->msg[q->head];
		q->head = (q->head+1)%MSG_QUEUE_SIZE;
	}

	return p;
}

int msg_enqueue(msg_queue_t *q, msg_t *p)
{
	if (msg_queue_full(q))
		return -1;

	q->msg[q->tail] = p;
	q->tail = (q->tail+1)%MSG_QUEUE_SIZE;

	return 0;
}

void msg_queue_init(msg_queue_t *q)
{
	q->head = q->tail = 0;
}
