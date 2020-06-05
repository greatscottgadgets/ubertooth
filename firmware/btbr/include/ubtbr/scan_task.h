#ifndef __SCAN_TASK_H
#define __SCAN_TASK_H

typedef void (*scan_task_cb_t)(int sw_detected, void *arg);

void scan_task_schedule(unsigned delay, scan_task_cb_t cb, void*cbarg);

#endif
