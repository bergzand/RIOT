/*
 * Copyright (C) 2020 Inria
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef BPF_CALL_H
#define BPF_CALL_H

#include <stdint.h>
#include "bpf/shared.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FUNC_BINDING(name)  uint32_t name(bpf_t* bpf, const uint64_t *regmap);

typedef uint32_t (*bpf_call_t)(bpf_t *bpf, const uint64_t *regmap);

FUNC_BINDING(bpf_vm_printf);
FUNC_BINDING(bpf_vm_store_local);
FUNC_BINDING(bpf_vm_store_global);
FUNC_BINDING(bpf_vm_fetch_local);
FUNC_BINDING(bpf_vm_fetch_global);

FUNC_BINDING(bpf_vm_memcpy);

FUNC_BINDING(bpf_vm_now_ms);

FUNC_BINDING(bpf_vm_saul_reg_find_nth);
FUNC_BINDING(bpf_vm_saul_reg_find_type);
FUNC_BINDING(bpf_vm_saul_reg_read);

FUNC_BINDING(bpf_vm_gcoap_resp_init);
FUNC_BINDING(bpf_vm_coap_opt_finish);

FUNC_BINDING(bpf_vm_fmt_s16_dfp);
FUNC_BINDING(bpf_vm_fmt_u32_dec);
FUNC_BINDING(bpf_vm_coap_add_format);
FUNC_BINDING(bpf_vm_coap_get_pdu);

FUNC_BINDING(bpf_vm_ztimer_now);
FUNC_BINDING(bpf_vm_ztimer_periodic_wakeup);

#ifdef __cplusplus
}
#endif
#endif /* BPF_CALL_H */

