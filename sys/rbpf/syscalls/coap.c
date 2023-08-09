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
 * @brief   Bindings for gcoap for rBPF
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include "rbpf.h"
#include "rbpf/internal/syscall.h"
#include "virt_syscall.h"
#include "virt_syscall/coap.h"
#include "virt_syscall/calls.h"


int rbpf_syscall_coap_init_req(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    uint8_t *buf = rbpf_syscall_arg_as_ptr(regs, 1);
    size_t buf_len = rbpf_syscall_arg_as_uint(regs, 2);
    unsigned code = rbpf_syscall_arg_as_uint(regs, 3);
    intptr_t handle = virt_syscall_gcoap_req_init(&ctx, buf, buf_len, code);
    rbpf_syscall_set_return(regs, (int64_t)handle);
    return 0;
}

int rbpf_syscall_coap_opt_add_uri(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    intptr_t handle = rbpf_syscall_arg_as_uint(regs, 1);
    const char *path = rbpf_syscall_arg_as_ptr(regs, 2);
    size_t path_len = rbpf_syscall_arg_as_uint(regs, 3);
    ssize_t call_res = 0;
    int res = virt_syscall_coap_opt_add_uri(&ctx, handle, path, path_len, &call_res);
    rbpf_syscall_set_return(regs, call_res);
    return res;
}

int rbpf_syscall_coap_hdr_set_type(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    intptr_t handle = rbpf_syscall_arg_as_uint(regs, 1);
    unsigned type = rbpf_syscall_arg_as_uint(regs, 2);
    int res = virt_syscall_coap_hdr_set_type(&ctx, handle, type);
    return res;
}

int rbpf_syscall_coap_opt_add_format(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    intptr_t handle = rbpf_syscall_arg_as_uint(regs, 1);
    unsigned format = rbpf_syscall_arg_as_uint(regs, 2);
    int res = virt_syscall_coap_opt_add_format(&ctx, handle, format);
    return res;
}

int rbpf_syscall_coap_opt_finish(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    intptr_t handle = rbpf_syscall_arg_as_uint(regs, 1);
    bool payload = rbpf_syscall_arg_as_uint(regs, 2);
    int res = virt_syscall_coap_opt_finish(&ctx, handle, payload);
    return res;
}

int rbpf_syscall_gcoap_req_send(rbpf_application_t *rbpf, uint64_t *regs)
{
    virt_syscall_ctx_t ctx = rbpf_syscall_ctx(rbpf);
    intptr_t handle = rbpf_syscall_arg_as_uint(regs, 1);
    size_t pkt_len = rbpf_syscall_arg_as_uint(regs, 2);
    const char *dest = rbpf_syscall_arg_as_ptr(regs, 3);
    size_t dest_len = rbpf_syscall_arg_as_uint(regs, 4);
    int call_res = 0;
    int res = virt_syscall_gcoap_req_send(&ctx, handle, pkt_len, dest, dest_len, &call_res);
    rbpf_syscall_set_return(regs, call_res);
    return res;
}
