#ifndef _L1_TDMA_SCHED_H
#define _L1_TDMA_SCHED_H

#include <stdint.h>

/* TDMA scheduler */

/* The idea of this scheduler is that we have a circular buffer of buckets,
 * where each bucket corresponds to one future TDMA frame [interrupt]. Each
 * bucket contains of a list of callbacks which are executed when the bucket
 * index reaches that particular bucket. */

#define TDMASCHED_NUM_FRAMES	16
#define TDMASCHED_NUM_CB	4

#define TDMA_IFLG_TPU		(1<<0)
#define TDMA_IFLG_DSP		(1<<1)

typedef int tdma_sched_cb(uint8_t p1, uint8_t p2, uint16_t p3);

/* A single item in a TDMA scheduler bucket */
struct tdma_sched_item {
	tdma_sched_cb *cb;
	uint8_t p1;
	uint8_t p2;
	uint16_t p3;
	int16_t prio;
	uint16_t flags;		/* TDMA_IFLG_xxx */
};

/* A bucket inside the TDMA scheduler */
struct tdma_sched_bucket {
	struct tdma_sched_item item[TDMASCHED_NUM_CB];
	uint8_t num_items;
};

/* The scheduler itself, consisting of buckets and a current index */
struct tdma_scheduler {
	struct tdma_sched_bucket bucket[TDMASCHED_NUM_FRAMES];
	uint8_t cur_bucket;
};

/* Schedule an item at 'frame_offset' TDMA frames in the future */
int tdma_schedule(uint8_t frame_offset, tdma_sched_cb *cb,
                  uint8_t p1, uint8_t p2, uint16_t p3, int16_t prio);

/* Schedule a set of items starting from 'frame_offset' TDMA frames in the future */
int tdma_schedule_set(uint8_t frame_offset, const struct tdma_sched_item *item_set, uint16_t p3);

/* Scan current frame scheduled items for flags */
uint16_t tdma_sched_flag_scan(void);

/* Execute pre-scheduled events for current frame */
int tdma_sched_execute(void);

/* Advance TDMA scheduler to the next bucket */
void tdma_sched_advance(void);

/* reset the scheduler; erase all scheduled items */
void tdma_sched_reset(void);

/* debug function: print number of entries of all TDMA buckets */
void tdma_sched_dump(void);


extern int tdma_end_set(uint8_t p1, uint8_t p2, uint16_t p3);
#define SCHED_ITEM(x, p, y, z)		{ .cb = x, .p1 = y, .p2 = z, .prio = p, .flags = 0 }
#define SCHED_ITEM_DT(x, p, y, z)	{ .cb = x, .p1 = y, .p2 = z, .prio = p, \
					  .flags = TDMA_IFLG_TPU | TDMA_IFLG_DSP }
#define SCHED_END_FRAME()		{ .cb = NULL, .p1 = 0, .p2 = 0 }
#define SCHED_END_SET()			{ .cb = &tdma_end_set, .p1 = 0, .p2 = 0 }

#endif /* _L1_TDMA_SCHED_H */
