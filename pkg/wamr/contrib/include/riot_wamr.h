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
 * @defgroup    pkg_wamr_riot WAMR RIOT helpers for bindings
 * @ingroup     pkg_wamr_user
 * @brief       WAMR user SAUL api
 * @{
 *
 * @file
 * @brief Interface definitions for WAMR
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

/**
 * @brief Helper macro to define WAMR binding in the env table
 *
 * The function must have a name formatted as wamr_symbol_name.
 *
 * @param symbol_name   symbol name to define
 * @param sign          signature of the function according to wamr syntax
 */
#define WAMR_NATIVE_SYMBOL(symbol_name, sign) \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
    XFA(wamr_native_symbols, 0) NativeSymbol xfa_wamr_ ## symbol_name = { \
        .symbol = XTSTR(symbol_name), \
        .func_ptr = (void*)wamr_ ## symbol_name, \
        .signature = sign, \
    }

/**
 * @brief Object handle structure for WAMR objects for the RIOT host.
 */
typedef struct {
    clist_node_t node;  /**< Next element in the list */
    void *ptr;          /**< Pointer to the actual object */
    size_t size;        /**< Size of the object */
    unsigned type;      /**< Type of object */
    unsigned num;       /**< Handle number */
} wamr_obj_handle_t;

/**
 * @brief WAMR pool structure.
 */
typedef struct {
    clist_node_t list;      /**< Pool list structure */
    unsigned handle_num;    /**< Number of handles allocated */
} wamr_obj_pool_t;

/**
 * @brief WAMR context for RIOT
 */
typedef struct {
    wamr_obj_pool_t obj_pool; /**< Memory pool for RIOT objects */
} riot_wamr_ctx_t;

/**
 * @brief Calloc function for allocating objects in WAMR by handle
 *
 * @param   pool        Pool to allocate from
 * @param   obj_bytes   Number of bytes to allocate for the object
 * @param   type        Object type
 *
 * @return  Pointer to the object handle
 */
wamr_obj_handle_t *wamr_obj_calloc_handle(wamr_obj_pool_t *pool, size_t obj_bytes, unsigned type);

/**
 * @brief Retrieve an object by handle
 *
 * @param   pool    Pool to retrieve from
 * @param   handle  Handle to retrieve
 *
 * @return  pointer to the object
 */
void *wamr_obj_by_handle(wamr_obj_pool_t *pool, unsigned handle);
#ifdef __cplusplus
}
#endif
#endif /* RIOT_WAMR_H */

/** @} */

