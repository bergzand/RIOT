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
#include "virt_syscall/saul.h"
#include "virt_syscall/calls.h"

int rbpf_syscall_saul_find_nth(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    size_t nth = rbpf_syscall_arg_as_uint(regs, 1);
    void *ref;
    virt_syscall_saul_reg_find_nth(&ctx, nth, &ref);
    rbpf_syscall_set_return_ptr(regs, ref);
    return 0;
}

int rbpf_syscall_saul_find_type(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    uint8_t type = rbpf_syscall_arg_as_uint(regs, 1);
    void *ref;
    virt_syscall_saul_reg_find_type(&ctx, type, &ref);
    rbpf_syscall_set_return_ptr(regs, ref);
    return 0;
}

int rbpf_syscall_saul_read(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    void *reg = rbpf_syscall_arg_as_ptr(regs, 1);

    void *phydat = rbpf_syscall_arg_as_ptr(regs, 2);

    int call_res;
    virt_syscall_saul_reg_read(&ctx, reg, phydat, &call_res);
    rbpf_syscall_set_return(regs, call_res);
    return 0;
}

int rbpf_syscall_saul_write(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    void *reg = rbpf_syscall_arg_as_ptr(regs, 1);

    void *phydat = rbpf_syscall_arg_as_ptr(regs, 2);

    int call_res;
    virt_syscall_saul_reg_write(&ctx, reg, phydat, &call_res);
    rbpf_syscall_set_return(regs, call_res);
    return 0;
}
