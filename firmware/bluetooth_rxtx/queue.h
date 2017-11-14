/*
 * Copyright 2017 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#define FIFOSIZE 10

typedef struct _queue_t {
	void *data[FIFOSIZE];
	unsigned head, tail;
} queue_t;

void queue_init(queue_t *f);
int queue_insert(queue_t *f, void *x);
int queue_remove(queue_t *f, void **x);

#endif /* __QUEUE_H__ */
