/*
 * Copyright (C) 2020 Inria
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "assert.h"

#include "container.h"
#include "rbpf/mem_pool.h"
#include "clist.h"

static size_t _align4(size_t num_bytes)
{
    return (num_bytes + 3) & ~0x03;
}

static void *_mem_pool_calloc(rbpf_mem_pool_t *pool, size_t size, size_t num)
{
    size_t num_bytes = size * num;
    if (pool->last + num_bytes > CONFIG_RBPF_MEM_POOL_BYTES) {
        return NULL;
    }
    void *ptr = pool->pool + pool->last;
    pool->last += num_bytes;
    return ptr;
}

static void _setup_handle(rbpf_mem_pool_t *pool, rbpf_mem_pool_handle_t *handle, void *obj, size_t obj_bytes, unsigned type)
{
    handle->ptr = obj;
    handle->num_bytes = obj_bytes;
    handle->type = type;
    handle->num = pool->handle_num;
    pool->handle_num++;
}

int rbpf_mem_pool_calloc_handle(rbpf_mem_pool_t *pool, size_t obj_bytes, unsigned type)
{
    size_t allocated_size = _align4(obj_bytes);
    /* Allocate new handle */
    rbpf_mem_pool_handle_t *handle = _mem_pool_calloc(pool, sizeof(rbpf_mem_pool_handle_t), 1);
    if (!handle) {
        return -1;
    }

    /* Allocate object content */
    void *obj = _mem_pool_calloc(pool, allocated_size, 1);
    if (!obj) {
        /* leaks the handle, cleanup is a todo */
        return -1;
    }

    _setup_handle(pool, handle, obj, allocated_size, type);

    /* Push handle to the end of the list */
    clist_rpush(&pool->list, &handle->node);
    return 0;
}

static int _find_handle(clist_node_t *node, void *arg)
{
    unsigned num = *(unsigned*)arg;
    rbpf_mem_pool_handle_t *handle = container_of(node, rbpf_mem_pool_handle_t, node);
    return handle->num == num;
}

rbpf_mem_pool_handle_t *rbpf_mem_pool_find_handle(rbpf_mem_pool_t *pool, unsigned handle)
{
    clist_node_t *node = clist_foreach(&pool->list, _find_handle, &handle);
    if (node) {
        return container_of(node, rbpf_mem_pool_handle_t, node);
    }
    return NULL;
}

void *rbpf_mem_pool_obj_by_handle(rbpf_mem_pool_t *pool, unsigned handle)
{
    rbpf_mem_pool_handle_t *phandle = rbpf_mem_pool_find_handle(pool, handle);
    if (!phandle) {
        return NULL;
    }
    return phandle->ptr;
}
