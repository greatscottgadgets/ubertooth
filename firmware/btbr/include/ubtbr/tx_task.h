#ifndef __TX_TASK_H
#define __TX_TASK_H

typedef void (*tx_task_cb_t)(void *arg);
void tx_task_schedule(unsigned delay, tx_task_cb_t cb, void*cbarg, bbhdr_t *hdr, uint8_t *data);
void tx_task_reset(void);

#endif
