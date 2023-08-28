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
 * This module provides a virtual-machine agnostic call backend. This module provides the bindings
 * to different modules in RIOT and exposes those for use by a virtual machine or scripting
 * environment such as rBPF and WAMR. The implementation here can be used both by VMs using virtual
 * memory and by VMs directly using the host OS' memory mapping. The virt_syscall module handles all
 * the complexities of the calls and the VM only needs an implementation for the calls that
 * translate the VM calling conventions to the C calling conventions used by the virt_syscall module.
 *
 * The virtual machines using this implementation must supply a context struct and 'driver'. The VM
 * must supply a number of functions via this 'driver' struct to this module:
 * - `add_mem_region`: The VM must allow access to the supplied memory region, this should be
 *   treated as flat byte buffer and can be used to provide access to either a struct or a buffer
 *   from the OS.
 * - `call_allowed`: This can be used in the VM as policy engine to grant or reject access to
 *   specific call types.
 * - `check_mem`: Verify whether access to a memory region is allowed according to the VM's policy.
 * - `alloc_obj_handle`: Allocate a buffer in the VM's memory with an object handle to store structs
 *   from the OS in. Access from within the VM must not be allowed as the struct is stored in the
 *   host architecture.
 * - `obj_from_handle`: Retrieve a struct or buffer from the VM by its handle.
 *
 * This module handles all the complexities of the calls. In particular the memory handling is all
 * handled here and the VMs only have to provide a number of calls in the driver struct to verify
 * access to memory and to allocate (for the VM) opaque objects.
 *
 * The allocation of objects to provide functionality to the VM is done via handles. The
 * objects themselves are allocated by the VM, but not accessible from inside the VM application.
 * This way no translation is required for these objects from the host to the VM architecture. The
 * VM only receives a handle to the object that is passed every call. The downside of this is that
 * every VM needs at least a slim allocator implementation and every access to struct members goes
 * through a call. As most VM implementations already provide an allocator to the VM applications
 * themselves, this is not considered a real issue. The allocator used for these handles only needs
 * to exist for the duration of the VM application.
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
    VIRT_SYSCALL_ERR_INVALID_ARG = -4,
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

/**
 * @brief Driver struct for virtual machines
 */
typedef struct {
    /**
     * @brief Request the VM to allow access to a memory region from the host.
     *
     * Can be used to grant access to memory buffers or to grant access to pointers outside the VM
     * through system calls. For example to a @ref saul_reg_t struct.
     *
     * @param   ctx         The VM provided context
     * @param   addr        Start address of the memory area
     * @param   len         Length in bytes of the memory area
     * @param   permission  Permissions granted to the memory area, see @ref virt_syscall_mem_perm_t.
     *
     * @returns zero on success, negative on error.
     */
    int (*add_mem_region)(void *ctx, const void *addr, size_t len, virt_syscall_mem_perm_t permissions);

    /**
     * @brief Check with the VM whether a call from the VM is allowed.
     *
     * Can be used to implement a policy engine in the VM. The arguments to this call provide hints
     * to the nature of the call
     *
     * @param   ctx         The VM provided context
     * @param   call        pointer to the specific call
     * @param   class       Class of the call, usually refers to a module.
     * @param   type        Type of the call, usually refers to the type of access of the call.
     *
     * @returns zero on call allowed, negative on error.
     */
    int (*call_allowed)(void *ctx, uintptr_t call, uint32_t class, virt_syscall_type_t type);

    /**
     * @brief Check whether access to a memory area is allowed by the VM's policy.
     *
     * @param   ctx         The VM provided context
     * @param   addr        Start address of the memory area
     * @param   len         Length in bytes of the memory area
     * @param   permission  Permission required on the memory area, see @ref virt_syscall_mem_perm_t.
     *
     * @returns zero on access allowed, negative on error.
     */
    int (*check_mem)(void *ctx, const uint8_t *buf, size_t len, virt_syscall_mem_perm_t permissions);

    /**
     * @brief Allocate a buffer with handle.
     *
     * @param   ctx         The VM provided context
     * @param   len         Length in bytes of the memory area
     *
     * @returns The object handle
     */
    intptr_t (*alloc_obj_handle)(void *ctx, size_t len);

    /**
     * @brief Return an object based on the handle
     *
     * @param   ctx         The VM provided context
     * @param   handle      The handle to the object.
     *
     * @returns The pointer to the object.
     */
    void *(*obj_from_handle)(void *ctx, intptr_t handle);
} virt_syscall_driver_t;

/**
 * @brief Context structs for VMs to supply via the calls.
 */
typedef struct {
    const virt_syscall_driver_t *driver; /**< VM-specific driver struct */
    void *ctx; /** Opaque context that can be used by the VM, not touched by this module */
} virt_syscall_ctx_t;

/**
 * @brief Request the VM to allow access to a memory region from the host.
 *
 * @param   ctx         The VM provided context
 * @param   addr        Start address of the memory area
 * @param   len         Length in bytes of the memory area
 * @param   permission  Permissions granted to the memory area, see @ref virt_syscall_mem_perm_t.
 *
 * @returns zero on success, negative on error.
 */
static inline int virt_syscall_add_mem_region(const virt_syscall_ctx_t *virt, const void *mem,
        size_t byte_len, virt_syscall_mem_perm_t permissions)
{
    return virt->driver->add_mem_region(virt->ctx, mem, byte_len, permissions);
}

/**
 * @brief Check with the VM whether a call from the VM is allowed.
 *
 * @param   ctx         The VM provided context
 * @param   call        pointer to the specific call
 * @param   class       Class of the call, usually refers to a module.
 * @param   type        Type of the call, usually refers to the type of access of the call.
 *
 * @returns zero on call allowed, negative on error.
 */
static inline int virt_syscall_call_allowed(const virt_syscall_ctx_t *virt, uintptr_t call,
        uint32_t class, virt_syscall_type_t type)
{
    return virt->driver->call_allowed(virt->ctx, call, class, type);
}

/**
 * @brief Check whether access to a memory area is allowed by the VM's policy.
 *
 * @param   ctx         The VM provided context
 * @param   addr        Start address of the memory area
 * @param   len         Length in bytes of the memory area
 * @param   permission  Permission required on the memory area, see @ref virt_syscall_mem_perm_t.
 *
 * @returns zero on access allowed, negative on error.
 */
static inline int virt_syscall_check_mem(const virt_syscall_ctx_t *virt, const void *mem,
        size_t byte_len, virt_syscall_mem_perm_t permissions)
{
    return virt->driver->check_mem(virt->ctx, mem, byte_len, permissions);
}

/**
 * @brief Allocate a buffer with handle.
 *
 * @param   ctx         The VM provided context
 * @param   len         Length in bytes of the memory area
 *
 * @returns The object handle
 */
static inline intptr_t virt_syscall_alloc_obj_handle(const virt_syscall_ctx_t *virt, size_t len)
{
    return virt->driver->alloc_obj_handle(virt->ctx, len);
}

/**
 * @brief Return an object based on the handle
 *
 * @param   ctx         The VM provided context
 * @param   handle      The handle to the object.
 *
 * @returns The pointer to the object.
 */
static inline void *virt_syscall_obj_from_handle(const virt_syscall_ctx_t *virt, intptr_t handle)
{
    return virt->driver->obj_from_handle(virt->ctx, handle);
}
#ifdef __cplusplus
}
#endif

#endif /* VIRT_SYSCALL_H */
/** @} */
