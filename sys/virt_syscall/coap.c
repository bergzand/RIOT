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

#ifdef MODULE_SOCK_UDP

#include "net/sock/util.h"
#include "net/nanocoap.h"
#include "net/gcoap.h"


/* returns the handle to the packet */
intptr_t virt_syscall_gcoap_req_init(const virt_syscall_ctx_t *virt, uint8_t *buf, size_t len, unsigned code)
{
    int buf_check = virt_syscall_check_mem(virt, buf, len, VIRT_SYSCALL_MEM_PERM_WRITE);
    if (buf_check < 0) {
        return buf_check;
    }

    /* Alloc coap pkt */
    intptr_t objhandle= virt_syscall_alloc_obj_handle(virt, sizeof(coap_pkt_t));
    if (objhandle < 0) {
        return objhandle;
    }

    coap_pkt_t *pkt = virt_syscall_obj_from_handle(virt, objhandle);

    int res = gcoap_req_init_path_buffer(pkt, buf, len, code, NULL, 0);
    if (res < 0)
    {
        return res;
    }
    return objhandle;
}

int virt_syscall_coap_opt_add_uri(const virt_syscall_ctx_t *virt, intptr_t handle, const char *path, size_t path_len, int *res)
{
    coap_pkt_t *pkt = virt_syscall_obj_from_handle(virt, handle);
    if (!pkt) {
        return -1; /* TODO: error code */
    }
    int buf_check = virt_syscall_check_mem(virt, path, path_len, VIRT_SYSCALL_MEM_PERM_READ);
    if (buf_check < 0) {
        return buf_check;
    }

    bool zero_term = path[path_len - 1] == '\0';
    if (!zero_term) {
        return VIRT_SYSCALL_ERR_INVALID_ARG;
    }

    *res = coap_opt_add_uri_path(pkt, path);
    return 0;
}

int virt_syscall_coap_hdr_set_type(const virt_syscall_ctx_t *virt, intptr_t handle, unsigned msg_type)
{
    coap_pkt_t *pkt = virt_syscall_obj_from_handle(virt, handle);
    if (!pkt) {
        return -1; /* TODO: error code */
    }

    coap_hdr_set_type(pkt->hdr, msg_type);
    return 0;
}

int virt_syscall_coap_opt_add_format(const virt_syscall_ctx_t *virt, intptr_t handle, unsigned format)
{
    coap_pkt_t *pkt = virt_syscall_obj_from_handle(virt, handle);
    if (!pkt) {
        return -1; /* TODO: error code */
    }

    coap_opt_add_format(pkt, format);
    return 0;
}

int virt_syscall_coap_opt_finish(const virt_syscall_ctx_t *virt, intptr_t handle, bool payload)
{
    coap_pkt_t *pkt = virt_syscall_obj_from_handle(virt, handle);
    if (!pkt) {
        return -1; /* TODO: error code */
    }
    coap_opt_finish(pkt, payload ? COAP_OPT_FINISH_PAYLOAD : COAP_OPT_FINISH_NONE);
    return 0;
}

int virt_syscall_gcoap_req_send(const virt_syscall_ctx_t *virt, intptr_t handle, size_t len, const char *dest_str, size_t dest_len, ssize_t *res)
{
    coap_pkt_t *pkt = virt_syscall_obj_from_handle(virt, handle);
    if (!pkt) {
        return -1; /* TODO: error code */
    }
    int dest_check = virt_syscall_check_mem(virt, dest_str, dest_len, VIRT_SYSCALL_MEM_PERM_READ);
    if (dest_check < 0) {
        return dest_check;
    }

    bool zero_term = dest_str[dest_len - 1] == '\0';
    if (!zero_term) {
        return VIRT_SYSCALL_ERR_INVALID_ARG;
    }
    sock_udp_ep_t remote;
    if (sock_udp_name2ep(&remote, dest_str) != 0) {
        return 0;
    }
    if (remote.port == 0) {
        remote.port = CONFIG_GCOAP_PORT;
    }

    const uint8_t *buf = (const uint8_t *)pkt->hdr;
    *res = gcoap_req_send(buf, len, &remote, NULL, NULL);
    return 0;
}
#endif
