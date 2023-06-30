/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_virt_syscall virt_syscall
 * @ingroup     sys
 * @brief       Virtual Machine syscall interface
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef VIRT_SYSCALL_H
#define VIRT_SYSCALL_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VIRT_SYSCALL_OK = 0,
    VIRT_SYSCALL_ERR_MEM = -1,
    VIRT_SYSCALL_ERR_CLASS = -2,
    VIRT_SYSCALL_ERR_TYPE = -3,
};

/**
 * @brief syscall call type
 *
 * Classifies the type of operation on the host system.
 *
 * Extend where needed
 */
typedef enum {
    VIRT_SYSCALL_TYPE_NONE,
    VIRT_SYSCALL_TYPE_READ,
    VIRT_SYSCALL_TYPE_WRITE,
    VIRT_SYSCALL_TYPE_EXEC,
} virt_syscall_type_t;

/**
 * @brief syscall memory permissions needed on the VM memory area
 */
typedef enum {
    VIRT_SYSCALL_MEM_PERM_NONE = 0x00,  /**< NOOP */
    VIRT_SYSCALL_MEM_PERM_READ = 0x01,  /**< Read permissions required */
    VIRT_SYSCALL_MEM_PERM_WRITE = 0x02, /**< Write permissions required */
    VIRT_SYSCALL_MEM_PERM_EXEC = 0x04,  /**< Execute permissions required */
    VIRT_SYSCALL_MEM_PERM_OPAQUE = 0x08,  /**< Opaque access, access only allowed through syscalls */
} virt_syscall_mem_perm_t;

typedef struct {
    int (*add_mem_region)(void *ctx, const void *addr, size_t len, virt_syscall_mem_perm_t permissions);
    int (*call_allowed)(void *ctx, uintptr_t call, uint32_t class, virt_syscall_type_t type);
    int (*check_mem)(void *ctx, const uint8_t *buf, size_t len, virt_syscall_mem_perm_t permissions);
} virt_syscall_driver_t;

typedef struct {
    const virt_syscall_driver_t *driver;
    void *ctx;
} virt_syscall_ctx_t;

static inline int virt_syscall_add_mem_region(const virt_syscall_ctx_t *virt, const void *mem,
        size_t byte_len, virt_syscall_mem_perm_t permissions)
{
    return virt->driver->add_mem_region(virt->ctx, mem, byte_len, permissions);
}

static inline int virt_syscall_check_mem(const virt_syscall_ctx_t *virt, const void *mem,
        size_t byte_len, virt_syscall_mem_perm_t permissions)
{
    return virt->driver->check_mem(virt->ctx, mem, byte_len, permissions);
}

static inline int virt_syscall_call_allowed(const virt_syscall_ctx_t *virt, uintptr_t call,
        uint32_t class, virt_syscall_type_t type)
{
    return virt->driver->call_allowed(virt->ctx, call, class, type);
}
#ifdef __cplusplus
}
#endif

#endif /* VIRT_SYSCALL_H */
/** @} */
