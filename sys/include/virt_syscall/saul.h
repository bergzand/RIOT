/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_virt_syscall_calls_random virt_syscall random interface
 * @ingroup     sys_virt_syscall_calls
 * @brief       Virtual Machine syscall interface
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef VIRT_SYSCALL_SAUL_H
#define VIRT_SYSCALL_SAUL_H

#include <stdint.h>
#include <stdlib.h>

#include "phydat.h"
#include "virt_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

int virt_syscall_saul_reg_find_nth(const virt_syscall_ctx_t *virt, int pos, void **ref);
int virt_syscall_saul_reg_find_type(const virt_syscall_ctx_t *virt, uint8_t type, void **ref);

int virt_syscall_saul_reg_read(const virt_syscall_ctx_t *virt, void *ref, phydat_t *data, int *call_res);
int virt_syscall_saul_reg_write(const virt_syscall_ctx_t *virt, void *ref, phydat_t *data, int *call_res);

#ifdef __cplusplus
}
#endif

#endif /* VIRT_SYSCALL_SAUL_H */
/** @} */

