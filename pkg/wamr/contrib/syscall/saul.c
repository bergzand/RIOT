/*
 * Copyright (C) 2023 Freie Universität Berlin
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
#include <math.h>

#include "riot_wamr.h"
#include "wasm_export.h"
#include "syscall.h"
#include "virt_syscall.h"
#include "virt_syscall/saul.h"

uintptr_t wamr_saul_reg_find_nth(wasm_exec_env_t env, int32_t pos)
{
    (void)env;
    void *ref;
    virt_syscall_ctx_t ctx = wamr_syscall_ctx(env);
    virt_syscall_saul_reg_find_nth(&ctx, pos, &ref);
    return (uintptr_t)ref;
}

uintptr_t wamr_saul_reg_find_type(wasm_exec_env_t env, uint32_t type)
{
    (void)env;
    void *ref;
    virt_syscall_ctx_t ctx = wamr_syscall_ctx(env);
    virt_syscall_saul_reg_find_type(&ctx, type, &ref);
    return (uintptr_t)ref;
}

float wamr_saul_reg_read(wasm_exec_env_t env, uintptr_t ref)
{
    int res = 0;
    phydat_t val;
    virt_syscall_ctx_t ctx = wamr_syscall_ctx(env);
    virt_syscall_saul_reg_read(&ctx, (void*)ref, &val, &res);
    return val.val[0] * pow(10, val.scale);
}

int wamr_saul_reg_write(wasm_exec_env_t env, uintptr_t ref, float val)
{
    int res = 0;
    (void)val;
    virt_syscall_ctx_t ctx = wamr_syscall_ctx(env);
    virt_syscall_saul_reg_write(&ctx, (void*)ref, NULL, &res);
    return res;
}

XFA_USE(NativeSymbol, wamr_native_symbols);
WAMR_NATIVE_SYMBOL(saul_reg_find_nth, "(i)i");
WAMR_NATIVE_SYMBOL(saul_reg_find_type, "(i)i");
WAMR_NATIVE_SYMBOL(saul_reg_read, "(i)f");
WAMR_NATIVE_SYMBOL(saul_reg_write, "(if)i");
