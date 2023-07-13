/*
 * Copyright (C) 2023 Freie Universität Berlin
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_rbpf_allocator rBPF small virtual machine allocator
 * @ingroup     sys_rbpf
 * @brief       Small memory allocator for rBPF VM applications
 */

#ifndef RBPF_MEM_POOL_H
#define RBPF_MEM_POOL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "clist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_RBPF_MEM_POOL_BYTES 256

typedef struct rbpf_mem_pool_handle {
    clist_node_t node;
    void *ptr;
    size_t num_bytes;
    unsigned type;
    unsigned num;
} rbpf_mem_pool_handle_t;

/**
 * @brief Memory pool struct
 */
typedef struct {
    unsigned handle_num;
    clist_node_t list;
    size_t last;    /**< Last offset allocated in pool */
    uint8_t pool[CONFIG_RBPF_MEM_POOL_BYTES]; /**< Pool itself */
} rbpf_mem_pool_t;

int rbpf_mem_pool_calloc_handle(rbpf_mem_pool_t *pool, size_t obj_bytes, unsigned type);
rbpf_mem_pool_handle_t *rbpf_mem_pool_find_handle(rbpf_mem_pool_t *pool, unsigned handle);
void *rbpf_mem_pool_obj_by_handle(rbpf_mem_pool_t *pool, unsigned handle);
#ifdef __cplusplus
}
#endif
#endif /* RBPF_MEM_POOL_H*/

/** @} */
