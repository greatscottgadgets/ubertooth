#ifndef __PKT_POOL_H
#define __PKT_POOL_H
#include <stdint.h>
#include <ubtbr/cfg.h>
#include <ubtbr/debug.h>

void mem_pool_init(void);
void *mem_pool_alloc(unsigned size);
void mem_pool_free(void *p);

#endif
