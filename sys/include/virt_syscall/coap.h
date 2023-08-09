/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_virt_syscall_calls_coap virt_syscall CoAP interface
 * @ingroup     sys_virt_syscall_calls
 * @brief       Virtual Machine syscall interface
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef VIRT_SYSCALL_COAP_H
#define VIRT_SYSCALL_COAP_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "virt_syscall.h"


#ifdef __cplusplus
extern "C" {
#endif

intptr_t virt_syscall_gcoap_req_init(const virt_syscall_ctx_t *virt, uint8_t *buf, size_t len, unsigned code);
intptr_t virt_syscall_coap_opt_add_uri(const virt_syscall_ctx_t *virt, intptr_t handle, const char *path, size_t path_len, ssize_t *res);
int virt_syscall_coap_hdr_set_type(const virt_syscall_ctx_t *virt, intptr_t handle, unsigned msg_type);
int virt_syscall_coap_opt_add_format(const virt_syscall_ctx_t *virt, intptr_t handle, unsigned format);
int virt_syscall_coap_opt_finish(const virt_syscall_ctx_t *virt, intptr_t handle, bool payload);
int virt_syscall_gcoap_req_send(const virt_syscall_ctx_t *virt, intptr_t handle, size_t len, const char *dest_str, size_t dest_len, ssize_t *res);
#ifdef __cplusplus
}
#endif

#endif /* VIRT_SYSCALL_COAP_H */
/** @} */


