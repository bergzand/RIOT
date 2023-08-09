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
 * @defgroup    pkg_wamr_syscall WAMR syscall interface
 * @ingroup     pkg_wamr
 * @brief       WAMR syscalls for RIOT
 * @{
 *
 * @file
 * @brief Interface definitions for WAMR syscalls for RIOT
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef WAMR_SYSCALL_H
#define WAMR_SYSCALL_H

#include "wasm_export.h"
#include "virt_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extern declaration for the WAMR syscall driver
 */
extern const virt_syscall_driver_t wamr_syscall_ctx_driver;

static inline virt_syscall_ctx_t wamr_syscall_ctx(wasm_exec_env_t env)
{
    return (virt_syscall_ctx_t){
        .driver = &wamr_syscall_ctx_driver,
        .ctx = env,
    };
}

#ifdef __cplusplus
}
#endif
#endif /* WAMR_SYSCALL_H */

/** @} */


