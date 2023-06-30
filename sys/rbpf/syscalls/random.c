/*
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

 /**
 * @ingroup sys_rbpf_syscalls_random
 * @{
 * @file
 *
 * @brief   Bindings for random bytes for rBPF
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include "rbpf.h"
#include "rbpf/internal/syscall.h"
#include "virt_syscall.h"
#include "virt_syscall/calls.h"

int rbpf_syscall_get_random_uint32(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    uint32_t val = 0;

    int res = virt_syscall_get_random_uint32(&ctx, &val);

    rbpf_syscall_set_return(regs, val);
    return res;
}

int rbpf_syscall_get_random_bytes(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    void *buf = rbpf_syscall_arg_as_ptr(regs, 1);
    size_t num_bytes = rbpf_syscall_arg_as_uint(regs, 2);

    return virt_syscall_get_random_bytes(&ctx, buf, num_bytes);
}
