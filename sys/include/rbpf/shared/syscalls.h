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
 * @defgroup    sys_rbpf rBPF user helper function numbers
 * @ingroup     sys
 * @brief       rBPF users helper function numbers
 * @{
 *
 * @file
 * @brief Shared syscall number values between VM and engine.
 *
 * This file contains mostly defines for different virtual machine system calls for rBPF.
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef SHARED_SYSCALLS_H
#define SHARED_SYSCALLS_H

#ifdef __cplusplus
extern "C" {
#endif


#define RBPF_SYSCALL_GROUP_SYS      0x10000 /**< rBPF system related calls */
#define RBPF_SYSCALL_GROUP_RANDOM   0x20000 /**< 'random' module calls */
#define RBPF_SYSCALL_GROUP_SAUL     0x30000 /**< saul module calls */

/**
 * @brief Tail call into another rBPF function
 */
#define RBPF_SYSCALL_TAIL_CALL      (RBPF_SYSCALL_GROUP_SYS | 1)

/**
 * @brief Get a random uint32 value from the host
 */
#define RBPF_SYSCALL_RANDOM_UINT32  (RBPF_SYSCALL_GROUP_RANDOM | 1)

/**
 * @brief Fill a buffer with random bytes
 */
#define RBPF_SYSCALL_RANDOM_BUF     (RBPF_SYSCALL_GROUP_RANDOM | 2)


#define RBPF_SYSCALL_SAUL_FIND_NTH (RBPF_SYSCALL_GROUP_SAUL | 1)
#define RBPF_SYSCALL_SAUL_FIND_TYPE (RBPF_SYSCALL_GROUP_SAUL | 2)
#define RBPF_SYSCALL_SAUL_READ      (RBPF_SYSCALL_GROUP_SAUL | 3)
#define RBPF_SYSCALL_SAUL_WRITE     (RBPF_SYSCALL_GROUP_SAUL | 4)

#ifdef __cplusplus
}
#endif
#endif /* SHARED_SYSCALLS_H */

/** @} */
