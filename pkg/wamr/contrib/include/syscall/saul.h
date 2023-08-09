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

#ifndef WAMR_USER_SAUL_H
#define WAMR_USER_SAUL_H

#include "wasm_export.h"

#ifdef __cplusplus
extern "C" {
#endif

uintptr_t saul_reg_find_nth(wasm_exec_env_t env, int pos);
uintptr_t saul_reg_find_type(wasm_exec_env_t env, uint8_t type);
int saul_reg_read(wasm_exec_env_t env, uintptr_t ref, phydat_t *data);
int saul_reg_write(wasm_exec_env_t env, uintptr_t ref, phydat_t *data);

#ifdef __cplusplus
}
#endif
#endif /* WAMR_USER_SAUL_H */

/** @} */

