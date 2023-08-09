/*
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

 /**
 * @ingroup sys_rbpf_syscalls
 * @{
 * @file
 *
 * @brief   Syscall helper functions for rBPF
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include "rbpf.h"
#include "rbpf/shared/syscalls.h"
#include "rbpf/internal/syscall.h"

rbpf_call_t rbpf_get_external_call(uint32_t num)
{
    switch (num) {
        case RBPF_SYSCALL_RANDOM_UINT32:
            return rbpf_syscall_get_random_uint32;
        case RBPF_SYSCALL_RANDOM_BUF:
            return rbpf_syscall_get_random_bytes;
        case RBPF_SYSCALL_GCOAP_REQ_INIT:
            return rbpf_syscall_coap_init_req;
        case RBPF_SYSCALL_GCOAP_REQ_SEND:
            return rbpf_syscall_gcoap_req_send;
        case RBPF_SYSCALL_COAP_OPT_ADD_URI:
            return rbpf_syscall_coap_opt_add_uri;
        case RBPF_SYSCALL_COAP_OPT_ADD_FORMAT:
            return rbpf_syscall_coap_opt_add_format;
        case RBPF_SYSCALL_COAP_OPT_FINISH:
            return rbpf_syscall_coap_opt_finish;
        case RBPF_SYSCALL_COAP_HDR_SET_TYPE:
            return rbpf_syscall_coap_hdr_set_type;
        default:
            return NULL;
    }
}

static int _check_mem(void *ctx, const uint8_t *buf, size_t len, virt_syscall_mem_perm_t permissions)
{
    rbpf_application_t *rbpf = (rbpf_application_t*)ctx;
    return rbpf_mem_allowed(rbpf, buf, len, permissions) ? 0 : -1;
}

static int _add_mem_region(void *ctx, const void *mem, size_t len, virt_syscall_mem_perm_t permissions)
{
    rbpf_application_t *rbpf = (rbpf_application_t*)ctx;
    for (size_t i = 0; i < RBPF_EXTRA_REGIONS_NUM; i++) {
        if (rbpf->extra_regions[i].start == NULL) {
            rbpf_memory_region_init(&rbpf->extra_regions[i], mem, len, permissions);
            rbpf_add_region(rbpf, &rbpf->extra_regions[i]);
            return 0;
        }
    }
    return -1;
}

static intptr_t _alloc_obj_handle(void *ctx, size_t len)
{
    rbpf_application_t *rbpf = (rbpf_application_t*)ctx;
    int handle = rbpf_mem_pool_calloc_handle(&rbpf->pool, len, 0);
    return handle;
}

static void *_obj_from_handle(void *ctx, intptr_t handle)
{
    rbpf_application_t *rbpf = (rbpf_application_t*)ctx;
    return rbpf_mem_pool_obj_by_handle(&rbpf->pool, handle);
}

const virt_syscall_driver_t rbpf_syscall_ctx_driver = {
    .check_mem = _check_mem,
    .add_mem_region = _add_mem_region,
    .alloc_obj_handle = _alloc_obj_handle,
    .obj_from_handle = _obj_from_handle,
};
