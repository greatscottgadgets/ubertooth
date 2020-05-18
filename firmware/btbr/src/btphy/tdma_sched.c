/* TDMA Scheduler Implementation */

/* (C) 2010 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdint.h>
//#include <stdio.h>
#include <string.h>

#include <ubtbr/cfg.h>
#include <ubtbr/defines.h>
#include <ubtbr/debug.h>

#include <ubtbr/tdma_sched.h>

static struct tdma_scheduler tdma_sched;

/* dummy function to mark end of set */
int tdma_end_set(__unused uint8_t p1, __unused uint8_t p2,
		 __unused uint16_t p3)
{
	return 0;
}

static uint8_t wrap_bucket(uint8_t offset)
{
	uint16_t bucket;

	bucket = (tdma_sched.cur_bucket + offset)
					% ARRAY_SIZE(tdma_sched.bucket);

	return bucket;
}

/* Schedule an item at 'frame_offset' TDMA frames in the future */
int tdma_schedule(uint8_t frame_offset, tdma_sched_cb *cb,
                  uint8_t p1, uint8_t p2, uint16_t p3, int16_t prio)
{
	struct tdma_scheduler *sched = &tdma_sched;
	uint8_t bucket_nr = wrap_bucket(frame_offset);
	struct tdma_sched_bucket *bucket = &sched->bucket[bucket_nr];
	struct tdma_sched_item *sched_item;

	if (bucket->num_items >= ARRAY_SIZE(bucket->item)) {
		cprintf("tdma_schedule bucket overflow\n");
		return -1;
	}

	sched_item = &bucket->item[bucket->num_items++];

	sched_item->cb = cb;
	sched_item->p1 = p1;
	sched_item->p2 = p2;
	sched_item->p3 = p3;
	sched_item->prio = prio;

	return 0;
}

/* Schedule a set of items starting from 'frame_offset' TDMA frames in the future */
int tdma_schedule_set(uint8_t frame_offset, const struct tdma_sched_item *item_set, uint16_t p3)
{
	struct tdma_scheduler *sched = &tdma_sched;
	uint8_t bucket_nr = wrap_bucket(frame_offset);
	int i, j;

	for (i = 0, j = 0; 1; i++) {
		const struct tdma_sched_item *sched_item = &item_set[i];
		struct tdma_sched_bucket *bucket = &sched->bucket[bucket_nr];

		if (sched_item->cb == &tdma_end_set) {
			/* end of scheduler set, return */
			break;
		}

		if (sched_item->cb == NULL) {
			/* advance to next bucket (== TDMA frame) */
			bucket_nr = wrap_bucket(++frame_offset);
			j++;
			continue;
		}
		/* check for bucket overflow */
		if (bucket->num_items >= ARRAY_SIZE(bucket->item)) {
			cprintf("tdma_schedule bucket overflow\n");
			return -1;
		}
		/* copy the item from the set into the current bucket item position */
		memcpy(&bucket->item[bucket->num_items], sched_item, sizeof(*sched_item));
		bucket->item[bucket->num_items].p3 = p3;
		bucket->num_items++;
	}

	return j;
}

/* Advance TDMA scheduler to the next bucket */
void tdma_sched_advance(void)
{
	struct tdma_scheduler *sched = &tdma_sched;
	uint8_t next_bucket;

	/* advance to the next bucket */
	next_bucket = wrap_bucket(1);
	sched->cur_bucket = next_bucket;
}

/* Scan current frame scheduled items for flags */
uint16_t tdma_sched_flag_scan(void)
{
	struct tdma_scheduler *sched = &tdma_sched;
	struct tdma_sched_bucket *bucket;
	int i;
	uint16_t flags = 0;

	/* determine current bucket */
	bucket = &sched->bucket[sched->cur_bucket];

	/* iterate over items in this bucket and call callback function */
	for (i=0; i<bucket->num_items; i++) {
		struct tdma_sched_item *item = &bucket->item[i];
		flags |= item->flags;
	}

	return flags;
}

/* Sort a bucket entries by priority */
static void _tdma_sched_bucket_sort(struct tdma_sched_bucket *bucket, int *seq)
{
	int i, j, k;
	struct tdma_sched_item *item_i, *item_j;

	/* initial sequence */
		/* we need all the items because some call back may schedule
		 * new call backs 'on the fly' */
	for (i=0; i<TDMASCHED_NUM_CB; i++)
		seq[i] = i;

	/* iterate over items in this bucket and sort them */
	for (i=0; i<bucket->num_items; i++)
	{
		item_i = &bucket->item[seq[i]];

		for (j=i+1; j<bucket->num_items; j++)
		{
			item_j = &bucket->item[seq[j]];

			if (item_i->prio > item_j->prio)
			{
				item_i = item_j;
				k      = seq[i];
				seq[i] = seq[j];
				seq[j] = k;
			}
		}
	}
}

/* Execute pre-scheduled events for current frame */
int tdma_sched_execute(void)
{
	struct tdma_scheduler *sched = &tdma_sched;
	struct tdma_sched_bucket *bucket;
	int i, num_events = 0;
	int seq[TDMASCHED_NUM_CB];

	/* determine current bucket */
	bucket = &sched->bucket[sched->cur_bucket];

	/* get sequence in priority order */
	_tdma_sched_bucket_sort(bucket, seq);

	/* iterate over items in this bucket and call callback function */
#ifdef USE_TDMA_DEBUG
	//if (bucket->num_items) 	cprintf (">");
#endif
	for (i = 0; i < bucket->num_items; i++) {
		struct tdma_sched_item *item = &bucket->item[seq[i]];
		int rc;

		num_events++;

		rc = item->cb(item->p1, item->p2, item->p3);
		if (rc < 0) {
			//cprintf("Error %d during processing of item %u of bucket %u\n",
			//	rc, i, sched->cur_bucket);
			return rc;
		}
		/* if the cb() we just called has scheduled more items for the
		 * current TDMA, bucket->num_items will have increased and we
		 * will simply continue to execute them as intended. Priorities
		 * won't work though ! */
	}

	/* clear/reset the bucket */
	bucket->num_items = 0;

	/* return number of items that we called */
	return num_events;
}

void tdma_sched_reset(void)
{
	struct tdma_scheduler *sched = &tdma_sched;
	unsigned int bucket_nr;

	for (bucket_nr = 0; bucket_nr < ARRAY_SIZE(sched->bucket); bucket_nr++) {
		struct tdma_sched_bucket *bucket = &sched->bucket[bucket_nr];
		/* current bucket will be reset by iteration code above! */
		if (bucket_nr != sched->cur_bucket)
			bucket->num_items = 0;
	}

	/* Don't reset cur_bucket, as it would upset the bucket iteration code
	 * in tdma_sched_execute() */
}

void tdma_sched_dump(void)
{
	unsigned int i;

	cprintf("\n(%2u)", tdma_sched.cur_bucket);
	for (i = 0; i < ARRAY_SIZE(tdma_sched.bucket); i++) {
		int bucket_nr = wrap_bucket(i);
		struct tdma_sched_bucket *bucket = &tdma_sched.bucket[bucket_nr];
		cprintf("%u:", bucket->num_items);
	}
	cputc('\n');
}
