/*
 * Copyright (C) 2021 Inria
 * Copyright (C) 2021 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef RBPF_INTERNAL_SYSCALL_H
#define RBPF_INTERNAL_SYSCALL_H

#include <stdint.h>
#include <stdlib.h>
#include "assert.h"
#include "rbpf.h"
#include "virt_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const virt_syscall_driver_t rbpf_syscall_ctx_driver;

static inline void rbpf_syscall_set_return(uint64_t *regs, uint64_t value)
{
    regs[0] = value;
}

static inline void rbpf_syscall_set_return_ptr(uint64_t *regs, void *value)
{
    rbpf_syscall_set_return(regs, (uintptr_t)value);
}

static inline void *rbpf_syscall_arg_as_ptr(uint64_t *regs, size_t arg_num)
{
    return (void*)(uintptr_t)regs[arg_num];
}

static inline uint64_t rbpf_syscall_arg_as_uint(uint64_t *regs, size_t arg_num)
{
    return regs[arg_num];
}

static inline int rbpf_syscall_check_mem(void *ctx, const uint8_t *buf, size_t len, virt_syscall_mem_perm_t permissions)
{
    static_assert(VIRT_SYSCALL_MEM_PERM_READ == RBPF_MEM_REGION_READ, "virt and rbpf defines must match");
    static_assert(VIRT_SYSCALL_MEM_PERM_WRITE == RBPF_MEM_REGION_WRITE, "virt and rbpf defines must match");
    return rbpf_mem_allowed(ctx, (void*)buf, len, permissions) ? 0 : VIRT_SYSCALL_ERR_MEM;
}

static inline virt_syscall_ctx_t rbpf_syscall_ctx(rbpf_application_t *rbpf)
{
    return (virt_syscall_ctx_t){
        .driver = &rbpf_syscall_ctx_driver,
        .ctx = rbpf,
    };
}

rbpf_call_t rbpf_get_external_call(uint32_t num);
int rbpf_syscall_get_random_uint32(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_get_random_bytes(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_saul_find_nth(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_saul_find_type(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_saul_read(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_saul_write(rbpf_application_t *rbpf, uint64_t *regs);

int rbpf_syscall_coap_init_req(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_coap_opt_add_uri(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_coap_opt_add_format(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_coap_opt_finish(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_coap_hdr_set_type(rbpf_application_t *rbpf, uint64_t *regs);
int rbpf_syscall_gcoap_req_send(rbpf_application_t *rbpf, uint64_t *regs);
#ifdef __cplusplus
}
#endif
#endif /* RBPF_INTERNAL_SYSCALL_H */

