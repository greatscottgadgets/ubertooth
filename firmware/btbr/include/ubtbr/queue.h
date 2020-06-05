#ifndef __QUEUE_H
#define __QUEUE_H
#include <ubertooth_interface.h>
#include <ubtbr/bb_msg.h>

// must be power of two
#define MSG_QUEUE_SIZE 8

typedef struct msg_queue_s {
	int head;
	int tail;
	msg_t *msg[MSG_QUEUE_SIZE];
} msg_queue_t;

static inline int msg_queue_empty(msg_queue_t *q)
{
	return q->head == q->tail;
}

static inline int msg_queue_full(msg_queue_t *q)
{
	return (q->tail+1)%MSG_QUEUE_SIZE == q->head;
}

void msg_queue_init(msg_queue_t *q);
msg_t* msg_dequeue(msg_queue_t *q);
int msg_enqueue(msg_queue_t *q, msg_t *p);
#endif
