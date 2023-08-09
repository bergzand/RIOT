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
#include "wasm_export.h"
#include "riot_wamr.h"
#include "clist.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static void _setup_handle(wamr_obj_pool_t *pool, wamr_obj_handle_t *handle, void *obj, size_t obj_bytes, unsigned type)
{
    handle->ptr = obj;
    handle->size = obj_bytes;
    handle->type = type;
    handle->num = pool->handle_num;
    pool->handle_num++;
}

wamr_obj_handle_t *wamr_obj_calloc_handle(wamr_obj_pool_t *pool, size_t obj_bytes, unsigned type)
{
    wamr_obj_handle_t *handle = wasm_runtime_malloc(sizeof(wamr_obj_handle_t));
    if (!handle) {
        return NULL;
    }

    void *obj = wasm_runtime_malloc(obj_bytes);
    if (!obj) {
        wasm_runtime_free(handle);
    }
    memset(obj, 0, obj_bytes);

    _setup_handle(pool, handle, obj, obj_bytes, type);
    return handle;
}

static int _find_handle(clist_node_t *node, void *arg)
{
    unsigned num = *(unsigned*)arg;
    wamr_obj_handle_t *handle = container_of(node, wamr_obj_handle_t, node);
    return handle->num == num;
}

wamr_obj_handle_t *wamr_obj_pool_find_handle(wamr_obj_pool_t *pool, unsigned handle)
{
    clist_node_t *node = clist_foreach(&pool->list, _find_handle, &handle);
    if (node) {
        return container_of(node, wamr_obj_handle_t, node);
    }
    return NULL;
}

void *wamr_obj_by_handle(wamr_obj_pool_t *pool, unsigned handle)
{
    wamr_obj_handle_t *phandle = wamr_obj_pool_find_handle(pool, handle);
    if (!phandle) {
        return NULL;
    }
    return phandle->ptr;
}

