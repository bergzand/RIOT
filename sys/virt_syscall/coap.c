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
 * @brief       virt_syscall coap implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include "virt_syscall.h"
#include "net/coap.h"
#include "net/nanocoap.h"
#include "net/gcoap.h"
#include "virt_syscall/coap_structs.h"

static int _verify_vm_coap_opts(const vm_coap_pkt_t *vm_pkt)
{
    if (vm_pkt->options_len > VM_COAP_OPTS_MAX) {
        return VIRT_SYSCALL_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < vm_pkt->options_len; i++) {
        if (vm_pkt->options[i].offset >= vm_pkt->payload_offset) {
            return VIRT_SYSCALL_ERR_INVALID_ARG;
        }
    }
    return 0;
}

static int _verify_vm_coap_pkt(const virt_syscall_ctx_t *virt, const vm_coap_pkt_t *vm_pkt)
{
    int mem_check = virt_syscall_check_mem(virt, vm_pkt->pkt,
            vm_pkt->payload_offset + vm_pkt->payload_len, VIRT_SYSCALL_MEM_PERM_WRITE);
    if (mem_check < 0) {
        return mem_check;
    }

    int opt_check = _verify_vm_coap_opts(vm_pkt);
    if (opt_check < 0) {
        return opt_check;
    }
    return 0;
}

static void _transform_to_coap_pkt(const vm_coap_pkt_t *vm_pkt, coap_pkt_t *coap_pkt)
{
    coap_pkt->hdr = (coap_hdr_t*)vm_pkt->pkt;
    coap_pkt->payload = vm_pkt->pkt + vm_pkt->payload_offset;
    coap_pkt->snips = NULL;
    coap_pkt->payload_len = vm_pkt->payload_len;
    coap_pkt->options_len = vm_pkt->options_len;
    memset(coap_pkt->options, 0, CONFIG_NANOCOAP_NOPTS_MAX * sizeof(coap_optpos_t));
    static_assert(sizeof(coap_optpos_t) == sizeof(vm_coap_optpos_t), "CoAP options struct sizes must be equal");
    memcpy(coap_pkt->options, vm_pkt->options, vm_pkt->options_len * sizeof(coap_optpos_t));
}

static void _transform_to_vm_pkt(vm_coap_pkt_t *vm_pkt, const coap_pkt_t *coap_pkt)
{
    vm_pkt->pkt = (uint8_t*)coap_pkt->hdr;
    vm_pkt->payload_offset = coap_pkt->payload - (uint8_t*)coap_pkt->hdr;
    vm_pkt->payload_len = coap_pkt->payload_len;
    vm_pkt->options_len = coap_pkt->options_len;
    memset(vm_pkt->options, 0, VM_COAP_OPTS_MAX * sizeof(coap_optpos_t));
    memcpy(vm_pkt->options, coap_pkt->options, coap_pkt->options_len * sizeof(coap_optpos_t));
}

static int _to_coap_pkt(const virt_syscall_ctx_t *virt, vm_coap_pkt_t *vm_pkt, coap_pkt_t *coap_pkt)
{
    int verify = _verify_vm_coap_pkt(virt, vm_pkt);
    if (verify < 0) {
        return verify;
    }
    _transform_to_coap_pkt(vm_pkt, coap_pkt);
    return 0;
}

int virt_syscall_gcoap_req_init(const virt_syscall_ctx_t *virt, vm_coap_pkt_t *vm_pkt, uint8_t *buf, size_t len, unsigned code, int *res)
{
    coap_pkt_t pkt;
    int buf_check = virt_syscall_check_mem(virt, buf, len, VIRT_SYSCALL_MEM_PERM_WRITE);
    if (buf_check < 0) {
        return buf_check;
    }

    *res = gcoap_req_init_path_buffer(&pkt, buf, len, code, NULL, 0);
    _transform_to_vm_pkt(vm_pkt, &pkt);

    return 0;
}

int virt_syscall_coap_opt_add_uri(const virt_syscall_ctx_t *virt, vm_coap_pkt_t *vm_pkt, const char *path, size_t path_len, int *res)
{
    coap_pkt_t coap_pkt;
    int verify = _to_coap_pkt(virt, vm_pkt, &coap_pkt);
    if (verify < 0) {
        return verify;
    }

    int buf_check = virt_syscall_check_mem(virt, path, path_len, VIRT_SYSCALL_MEM_PERM_READ);
    if (buf_check < 0) {
        return buf_check;
    }

    bool zero_term = path[path_len - 1] == '\0';
    if (!zero_term) {
        return VIRT_SYSCALL_ERR_INVALID_ARG;
    }

    *res = coap_opt_add_uri_path(&coap_pkt, path);

    _transform_to_vm_pkt(vm_pkt, &coap_pkt);

    return 0;
}
