/*
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "xfa.h"
#include "virt_syscall.h"
#include "riot_wamr.h"
#include "wasm_export.h"

XFA_INIT(NativeSymbol, wamr_native_symbols);

static int _check_mem(void *ctx, const uint8_t *buf, size_t len, virt_syscall_mem_perm_t permissions)
{
    (void)ctx;
    (void)permissions;
    (void)buf;
    (void)len;
    return 0; /* Check already done by WAMR */
}

static int _add_mem_region(void *ctx, const void *addr, size_t len, virt_syscall_mem_perm_t permissions)
{
    (void)ctx;
    (void)addr;
    (void)len;
    (void)permissions;
    return 0;
}

static intptr_t _alloc_obj_handle(void *ctx, size_t len)
{
    wasm_exec_env_t exec_env = (wasm_exec_env_t)ctx;
    riot_wamr_ctx_t *riot_wamr = wasm_runtime_get_user_data(exec_env);
    return (intptr_t)wamr_obj_calloc_handle(&riot_wamr->obj_pool, len, 0);
}

static void *_obj_from_handle(void *ctx, intptr_t handle)
{
    wasm_exec_env_t exec_env = (wasm_exec_env_t)ctx;
    riot_wamr_ctx_t *riot_wamr = wasm_runtime_get_user_data(exec_env);
    return wamr_obj_by_handle(&riot_wamr->obj_pool, handle);
}

const virt_syscall_driver_t wamr_syscall_ctx_driver = {
    .check_mem = _check_mem,
    .add_mem_region = _add_mem_region,
    .alloc_obj_handle = _alloc_obj_handle,
    .obj_from_handle = _obj_from_handle,
};
