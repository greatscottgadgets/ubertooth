#ifndef __BTCTL_H
#define __BTCTL_H
// FIXME: this is a mess: msg.h must be included after defining DIE & btctl_mem_alloc/free
#include <ubtbr/debug.h>
#include <ubtbr/mem_pool.h>
#define btctl_mem_alloc mem_pool_alloc
#define btctl_mem_free	mem_pool_free
#include <ubtbr/msg.h>

#endif
