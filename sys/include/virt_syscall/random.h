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

#ifndef VIRT_SYSCALL_RANDOM_H
#define VIRT_SYSCALL_RANDOM_H

#include <stdint.h>
#include <stdlib.h>

#include "virt_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

int virt_syscall_get_random_bytes(const virt_syscall_ctx_t *virt, void *bytes, size_t len);
int virt_syscall_get_random_uint32(const virt_syscall_ctx_t *virt, uint32_t *result);

#ifdef __cplusplus
}
#endif

#endif /* VIRT_SYSCALL_RANDOM_H */
/** @} */
