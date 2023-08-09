/*
 * Copyright (C) 2023 Freie Universität Berlin
 * Copyright (C) 2023 Inria
 * Copyright (C) 2023 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_rbpf rBPF user helper functions
 * @ingroup     sys
 * @brief       rBPF users helper function syscalls
 * @{
 *
 * @file
 * @brief Interface definitions for rBPF syscalls
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef USER_RBPF_HELPERS_H
#define USER_RBPF_HELPERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "rbpf/shared/syscalls.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void handle_t;

/* TODO: autogenerate these */
//static uint32_t (*bpf_tail_call)(void *ctx, struct bpf_map *map, uint32_t index) = (void*)RBPF_SYSCALL_TAIL_CALL;

/**
 * @brief Retrieve a uint32_t with random value from the host
 *
 * @returns a random value
 */
static uint32_t (*bpf_get_prandom_u32)(void) = (void*)RBPF_SYSCALL_RANDOM_UINT32;

/**
 * @brief Fill a buffer with random bytes
 *
 * @param buf   Buffer to fill
 * @param len   Length in bytes of the buffer
 */
static void (*bpf_get_prandom_buf)(void *buf, size_t len) = (void*)RBPF_SYSCALL_RANDOM_BUF;

#if 0
static void* (*saul_reg_find_nth)(int pos) = (void*)RBPF_SYSCALL_SAUL_FIND_NTH;
static void* (*saul_reg_find_type)(uint8_t type) = (void*)RBPF_SYSCALL_SAUL_FIND_TYPE;
static int (*saul_reg_read)(handle_t *ref, phydat_t *data) = (void*)RBPF_SYSCALL_SAUL_READ;
static int (*saul_reg_write)(handle_t *ref, phydat_t *data) = (void*)RBPF_SYSCALL_SAUL_WRITE;
#endif


static int (*gcoap_req_init)(uint8_t *buf, size_t len, unsigned code) = (void*)RBPF_SYSCALL_GCOAP_REQ_INIT;
static int (*coap_opt_add_uri)(int handle, const char *path, size_t path_len) = (void*)RBPF_SYSCALL_COAP_OPT_ADD_URI;
static int (*coap_hdr_set_type)(int handle, unsigned msg_type) = (void*)RBPF_SYSCALL_COAP_HDR_SET_TYPE;
static int (*coap_opt_add_format)(int handle, unsigned format) = (void*)RBPF_SYSCALL_COAP_OPT_ADD_FORMAT;
static int (*coap_opt_finish)(int handle, bool payload) = (void*)RBPF_SYSCALL_COAP_OPT_FINISH;
static int (*gcoap_req_send)(int handle, size_t len, const char *dest_str, size_t dest_len) = (void*)RBPF_SYSCALL_GCOAP_REQ_SEND;
#ifdef __cplusplus
}
#endif
#endif /* USERS_RBPF_HELPERS_H*/

/** @} */
