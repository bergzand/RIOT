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

/**
 * @brief rBPF mem pool object handle
 */
typedef struct rbpf_mem_pool_handle {
    clist_node_t node;  /**< Next handle in list */
    void *ptr;          /**< Pointer to the actual object */
    size_t alloc_size;  /**< Allocated size */
    size_t req_size;    /**< Requested object size */
    unsigned type;      /**< Type of object */
    unsigned num;       /**< Handle number */
} rbpf_mem_pool_handle_t;

/**
 * @brief Memory pool struct
 */
typedef struct {
    clist_node_t list; /**< First object in list */
    unsigned handle_num;    /**< Highest handle number allocated */
    size_t last;    /**< Last offset allocated in pool */
    uint8_t pool[CONFIG_RBPF_MEM_POOL_BYTES]; /**< Pool itself */
} rbpf_mem_pool_t;

/**
 * @brief Calloc function to allocate object with handle in the pool
 *
 * @param pool      Allocator pool to allocate from
 * @param obj_bytes Number of bytes to allocate for the object
 * @param type      Type of the object to allocate
 *
 * @return  Negative on error
 * @return  Handle on ok
 */
int rbpf_mem_pool_calloc_handle(rbpf_mem_pool_t *pool, size_t obj_bytes, unsigned type);

/**
 * @brief Find object handle by numeric handle
 *
 * @param pool   Pool to look into
 * @param handle Handle value to look for
 *
 * @return  pointer to the handle
 * @return  NULL on error or handle not found
 */
rbpf_mem_pool_handle_t *rbpf_mem_pool_find_handle(rbpf_mem_pool_t *pool, unsigned handle);

/**
 * @brief Find object by numeric handle
 *
 * @param pool   Pool to look into
 * @param handle Handle value to look for
 *
 * @return  pointer to the object
 * @return  NULL on error or handle not found
 */
void *rbpf_mem_pool_obj_by_handle(rbpf_mem_pool_t *pool, unsigned handle);
#ifdef __cplusplus
}
#endif
#endif /* RBPF_MEM_POOL_H*/

/** @} */
