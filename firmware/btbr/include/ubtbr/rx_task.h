#ifndef __RX_TASK_H
#define __RX_TASK_H
#include <ubertooth_interface.h>
#include <ubtbr/bb_msg.h>

#define RX_F_PAYLOAD 	0
#define RX_F_RAW	1

typedef int (*rx_task_cb_t)(msg_t *msg, void *arg, int time_offset);
void rx_task_schedule(unsigned delay, rx_task_cb_t cb, void*cbarg, unsigned flags);
void rx_task_reset(void);
#endif
