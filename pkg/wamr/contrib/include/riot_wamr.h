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
 * @defgroup    pkg_wamr_user_saul WAMR SAUL bindings
 * @ingroup     pkg_wamr_user
 * @brief       WAMR user SAUL api
 * @{
 *
 * @file
 * @brief Interface definitions for WAMR SAUL functions
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef RIOT_WAMR_H
#define RIOT_WAMR_H

#include "macros/xtstr.h"
#include "xfa.h"
#include "wasm_export.h"
#include "clist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WAMR_NATIVE_SYMBOL(symbol_name, sign) \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
    XFA(wamr_native_symbols, 0) NativeSymbol xfa_wamr_ ## symbol_name = { \
        .symbol = XTSTR(symbol_name), \
        .func_ptr = (void*)wamr_ ## symbol_name, \
        .signature = sign, \
    }

typedef struct wamr_obj_handle {
    clist_node_t node;
    void *ptr;
    size_t size;
    unsigned type;
    unsigned num;
} wamr_obj_handle_t;

typedef struct {
    clist_node_t list;
    unsigned handle_num;
} wamr_obj_pool_t;

typedef struct {
    wamr_obj_pool_t obj_pool;
} riot_wamr_ctx_t;


wamr_obj_handle_t *wamr_obj_calloc_handle(wamr_obj_pool_t *pool, size_t obj_bytes, unsigned type);
void *wamr_obj_by_handle(wamr_obj_pool_t *pool, unsigned handle);
#ifdef __cplusplus
}
#endif
#endif /* RIOT_WAMR_H */

/** @} */

