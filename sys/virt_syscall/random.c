/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_virt_syscall
 * @{
 *
 * @file
 * @brief       virt_syscall random implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include "virt_syscall.h"
#include "random.h"

int virt_syscall_get_random_bytes(const virt_syscall_ctx_t *virt, void *bytes, size_t len)
{
    int res = virt_syscall_check_mem(virt, bytes, len, VIRT_SYSCALL_MEM_PERM_WRITE);
    if (res < 0) {
        return res;
    }

    random_bytes(bytes, len);
    return VIRT_SYSCALL_OK;
}

int virt_syscall_get_random_uint32(const virt_syscall_ctx_t *virt, uint32_t *result)
{
    (void)virt;
    *result = random_uint32();
    return VIRT_SYSCALL_OK;
}
