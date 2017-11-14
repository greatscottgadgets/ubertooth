/*
 * Copyright 2017 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#include "queue.h"

// queue implementation is based heavily on Koopman's "Better Embedded
// Systems Software" section 20.3.3.1 pg 209

void queue_init(queue_t *f) {
	f->head = 0;
	f->tail = 0;
}

// insert
int queue_insert(queue_t *f, void *x) {
	unsigned newtail;
	// access next free element
	newtail = f->tail + 1;

	// wrap around to beginning if needed
	if (newtail >= FIFOSIZE) { newtail = 0; }

	// if head and tail are equal, queue is full
	if (newtail == f->head) { return 0; }

	// write data before updating pointer
	f->data[newtail] = x;
	f->tail = newtail;

	return 1;
}

// TODO remove
int queue_remove(queue_t *f, void **x) {
	unsigned newhead;

	if (f->head == f->tail) { return 0; }

	newhead = f->head + 1;

	if (newhead >= FIFOSIZE) { newhead = 0; }

	*x = f->data[newhead];
	f->head = newhead;

	return 1;
}
