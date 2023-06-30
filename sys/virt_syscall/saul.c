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
 * @brief       virt_syscall saul implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include "virt_syscall.h"
#include "saul_reg.h"

int virt_syscall_saul_reg_find_nth(const virt_syscall_ctx_t *virt, int pos, void **ref)
{
    saul_reg_t *dev = saul_reg_find_nth(pos);
    *ref = dev;
    if (dev) {
        virt_syscall_add_mem_region(virt, dev, sizeof(saul_reg_t), VIRT_SYSCALL_MEM_PERM_OPAQUE);
    }
    return 0;
}

int virt_syscall_saul_reg_find_type(const virt_syscall_ctx_t *virt, uint8_t type, void **ref)
{
    saul_reg_t *dev = saul_reg_find_type(type);
    *ref = dev;
    if (dev) {
        virt_syscall_add_mem_region(virt, dev, sizeof(saul_reg_t), VIRT_SYSCALL_MEM_PERM_OPAQUE);
    }
    return 0;
}

int virt_syscall_saul_reg_read(const virt_syscall_ctx_t *virt, void *ref, phydat_t *data, int *call_res)
{
    saul_reg_t *dev = (saul_reg_t*)ref;
    int res = virt_syscall_check_mem(virt, dev, sizeof(saul_reg_t), VIRT_SYSCALL_MEM_PERM_OPAQUE);
    if (res < 0) {
        return res;
    }
    res = virt_syscall_check_mem(virt, data, sizeof(phydat_t), VIRT_SYSCALL_MEM_PERM_WRITE);
    *call_res = saul_reg_read(dev, data);
    return 0;
}

int virt_syscall_saul_reg_write(const virt_syscall_ctx_t *virt, void *ref, phydat_t *data, int *call_res)
{
    saul_reg_t *dev = (saul_reg_t*)ref;
    int res = virt_syscall_check_mem(virt, dev, sizeof(saul_reg_t), VIRT_SYSCALL_MEM_PERM_OPAQUE);
    if (res < 0) {
        return res;
    }
    res = virt_syscall_check_mem(virt, data, sizeof(phydat_t), VIRT_SYSCALL_MEM_PERM_READ);
    *call_res = saul_reg_write(dev, data);
    return 0;
}
