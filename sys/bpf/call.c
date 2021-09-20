/*
 * Copyright (C) 2020 Inria
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bpf.h"
#include "bpf/instruction.h"
#include "bpf/store.h"
#include "bpf/shared.h"
#include "bpf/call.h"
#include "xtimer.h"

#ifdef MODULE_GCOAP
#include "net/gcoap.h"
#include "net/nanocoap.h"
#endif
#include "saul.h"
#include "saul_reg.h"
#include "fmt.h"

#ifdef MODULE_ZTIMER
#include "ztimer.h"
#endif

uint32_t _reg(const uint64_t *regmap, size_t num)
{
    return regmap[num];
}

bpf_call_t bpf_call_get(uint32_t num)
{
    switch(num) {
        case BPF_FUNC_BPF_PRINTF:
            return &bpf_vm_printf;
        case BPF_FUNC_BPF_MEMCPY:
            return &bpf_vm_memcpy;
        case BPF_FUNC_BPF_STORE_LOCAL:
            return &bpf_vm_store_local;
        case BPF_FUNC_BPF_STORE_GLOBAL:
            return &bpf_vm_store_global;
        case BPF_FUNC_BPF_FETCH_LOCAL:
            return &bpf_vm_fetch_local;
        case BPF_FUNC_BPF_FETCH_GLOBAL:
            return &bpf_vm_fetch_global;
#ifdef MODULE_XTIMER
        case BPF_FUNC_BPF_NOW_MS:
            return &bpf_vm_now_ms;
#endif
#ifdef MODULE_SAUL_REG
        case BPF_FUNC_BPF_SAUL_REG_FIND_NTH:
            return &bpf_vm_saul_reg_find_nth;
        case BPF_FUNC_BPF_SAUL_REG_FIND_TYPE:
            return &bpf_vm_saul_reg_find_type;
        case BPF_FUNC_BPF_SAUL_REG_READ:
            return &bpf_vm_saul_reg_read;
#endif
#ifdef MODULE_GCOAP
        case BPF_FUNC_BPF_GCOAP_RESP_INIT:
            return &bpf_vm_gcoap_resp_init;
        case BPF_FUNC_BPF_COAP_OPT_FINISH:
            return &bpf_vm_coap_opt_finish;
        case BPF_FUNC_BPF_COAP_ADD_FORMAT:
            return &bpf_vm_coap_add_format;
        case BPF_FUNC_BPF_COAP_GET_PDU:
            return &bpf_vm_coap_get_pdu;
#endif
#ifdef MODULE_FMT
        case BPF_FUNC_BPF_FMT_S16_DFP:
            return &bpf_vm_fmt_s16_dfp;
        case BPF_FUNC_BPF_FMT_U32_DEC:
            return &bpf_vm_fmt_u32_dec;
#endif
#ifdef MODULE_ZTIMER
        case BPF_FUNC_BPF_ZTIMER_NOW:
            return &bpf_vm_ztimer_now;
        case BPF_FUNC_BPF_ZTIMER_PERIODIC_WAKEUP:
            return &bpf_vm_ztimer_periodic_wakeup;
#endif
        default:
            return NULL;
    }
}

uint32_t bpf_vm_printf(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    return printf((char*)(uintptr_t)_reg(regmap, 1),
            _reg(regmap, 2),
            _reg(regmap, 3),
            _reg(regmap, 4),
            _reg(regmap, 5));
}

uint32_t bpf_vm_store_local(bpf_t *bpf, const uint64_t *regmap)
{
    uint32_t key = _reg(regmap, 1);
    uint32_t value = _reg(regmap, 2);
    return (uint32_t)bpf_store_update_local(bpf, key, value);
}

uint32_t bpf_vm_store_global(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    uint32_t key = _reg(regmap, 1);
    uint32_t value = _reg(regmap, 2);
    return (uint32_t)bpf_store_update_global(key, value);
}

uint32_t bpf_vm_fetch_local(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    uint32_t key = _reg(regmap, 1);
    uint32_t *value = (uint32_t*)(uintptr_t)_reg(regmap, 2);
    if (bpf_store_allowed(bpf, value, sizeof(uint32_t)) < 0) {
        return -1;
    }
    return (uint32_t)bpf_store_fetch_local(bpf, key, value);
}

uint32_t bpf_vm_fetch_global(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    uint32_t key = _reg(regmap, 1);
    uint32_t *value = (uint32_t*)(uintptr_t)_reg(regmap, 2);
    if (bpf_store_allowed(bpf, (void*)value, sizeof(uint32_t)) < 0) {
        return -1;
    }
    return (uint32_t)bpf_store_fetch_global(key, (uint32_t*)(uintptr_t)value);
}

uint32_t bpf_vm_memcpy(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    void *dest = (void *)(uintptr_t)_reg(regmap, 1);
    const void *src = (const void *)(uintptr_t)_reg(regmap, 2);
    size_t size = _reg(regmap, 3);

    if (bpf_store_allowed(bpf, dest, size) < 0) {
        return -1;
    }
    if (bpf_load_allowed(bpf, src, size) < 0) {
        return -1;
    }

    return (uintptr_t) memcpy(dest, src, size);
}

#ifdef MODULE_XTIMER
uint32_t bpf_vm_now_ms(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    (void)regmap;
    return xtimer_now_usec64()/US_PER_MS;
}
#endif

#ifdef MODULE_SAUL_REG
uint32_t bpf_vm_saul_reg_find_nth(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    int pos = (int)_reg(regmap, 1);
    saul_reg_t *reg = saul_reg_find_nth(pos);
    return (uint32_t)(intptr_t)reg;
}

uint32_t bpf_vm_saul_reg_find_type(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    uint8_t type = _reg(regmap, 1);

    saul_reg_t *reg = saul_reg_find_type(type);
    return (uint32_t)(intptr_t)reg;
}

uint32_t bpf_vm_saul_reg_read(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    saul_reg_t *dev = (saul_reg_t*)(intptr_t)_reg(regmap, 1);
    phydat_t *data = (phydat_t*)(intptr_t)_reg(regmap, 2);

    if (bpf_store_allowed(bpf, data, sizeof(phydat_t)) < 0) {
        return -1;
    }

    int res = saul_reg_read(dev, data);
    return (uint32_t)res;
}
#endif

#ifdef MODULE_GCOAP
uint32_t bpf_vm_gcoap_resp_init(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    bpf_coap_ctx_t *coap_ctx = (bpf_coap_ctx_t *)(intptr_t)_reg(regmap, 1);
    unsigned resp_code = (unsigned)_reg(regmap, 2);

    gcoap_resp_init(coap_ctx->pkt, coap_ctx->buf, coap_ctx->buf_len, resp_code);
    return 0;
}

uint32_t bpf_vm_coap_add_format(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    bpf_coap_ctx_t *coap_ctx = (bpf_coap_ctx_t *)(intptr_t)_reg(regmap, 1);
    uint16_t format = _reg(regmap, 2);

    ssize_t res = coap_opt_add_format(coap_ctx->pkt, format);
    return (uint32_t)res;
}

uint32_t bpf_vm_coap_opt_finish(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    bpf_coap_ctx_t *coap_ctx = (bpf_coap_ctx_t *)(intptr_t)_reg(regmap, 1);
    uint16_t flags = (uint16_t)_reg(regmap, 2);

    ssize_t res = coap_opt_finish(coap_ctx->pkt, flags);
    return (uint32_t)res;
}

uint32_t bpf_vm_coap_get_pdu(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    bpf_coap_ctx_t *coap_ctx = (bpf_coap_ctx_t *)(intptr_t)_reg(regmap, 1);
    return (uint32_t)(intptr_t)((coap_pkt_t*)coap_ctx->pkt)->payload;
}
#endif

#ifdef MODULE_FMT
uint32_t bpf_vm_fmt_s16_dfp(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    char *out = (char*)(intptr_t)_reg(regmap, 1);
    int16_t val = _reg(regmap, 2);
    int digits = _reg(regmap, 3);

    /* First get the resulting size without writing */
    size_t res = fmt_s16_dfp(NULL, val, digits);

    /* Check if it is allowed to write there */
    if (bpf_store_allowed(bpf, out, res) < 0) {
        return -1;
    }

    res = fmt_s16_dfp(out, val, digits);
    return (uint32_t)res;
}

uint32_t bpf_vm_fmt_u32_dec(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;

    char *out = (char*)(intptr_t)_reg(regmap, 1);
    uint32_t val = _reg(regmap, 2);

    size_t res = fmt_u32_dec(NULL, val);
    if (bpf_store_allowed(bpf, out, res) < 0) {
        return -1;
    }

    res = fmt_u32_dec(out, val);
    return (uint32_t)res;
}
#endif

#ifdef MODULE_ZTIMER
uint32_t bpf_vm_ztimer_now(bpf_t *bpf, const uint64_t *regmap)
{
    (void)bpf;
    (void)regmap;
    return ztimer_now(ZTIMER_USEC);
}
uint32_t bpf_vm_ztimer_periodic_wakeup(bpf_t *bpf, const uint64_t *regmap)
                                       uint32_t period,
                                       uint32_t a3, uint32_t a4, uint32_t a5)
{
    (void)bpf;
    uint32_t *last = (uint32_t*)(intptr_t)_reg(regmap, 1);
    uint32_t period = _reg(regmap, 2);

    if (bpf_loadstore_allowed(bpf, last, sizeof(uint32_t)) < 0) {
        return -1;
    }

    ztimer_periodic_wakeup(ZTIMER_USEC, last, period);
    return 0;
}
#endif
