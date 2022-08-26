/*
 * Copyright (C) 2022 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_coreconf CORECONF CoAP management interface
 * @ingroup     net
 * @brief       CORECONF definitions
 * @{
 *
 * @file
 * @brief   CORECONF definitions
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 *
 * @experimental
 */
#ifndef NET_CORECONF_H
#define NET_CORECONF_H

#include "macros/xtstr.h"
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "net/gcoap.h"
#include "nanocbor/nanocbor.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CORECONF_STORE_SUBTREE
#define CORECONF_STORE_SUBTREE "/c"
#endif

/**
 * @brief Maximum allowed length of the k query parameter
 */
#ifndef CORECONF_COAP_K_LEN
#define CORECONF_COAP_K_LEN 16
#endif

/**
 * @brief CoAP format code used in the response. Spec says
 * application/yang-data+cbor; id=sid, but decoding with aiocoap is automatic
 * when using application/cbor
 */
#ifndef CORECONF_COAP_FORMAT
#define CORECONF_COAP_FORMAT   COAP_FORMAT_CBOR
#endif

typedef struct coreconf_node coreconf_node_t;

typedef struct {
    coap_block_slicer_t slicer;
    size_t sliced_length;
    uint8_t *buf;
} coreconf_slicer_helper_t;

typedef struct {
    coap_pkt_t *pdu;
    coreconf_slicer_helper_t slicer;
    nanocbor_encoder_t encoder;
    char k_param[CORECONF_COAP_K_LEN];
    size_t num_k_args;
} coreconf_encoder_t;

typedef int (*coreconf_node_read_handler_t)(coreconf_encoder_t *encoder, const coreconf_node_t *node);
typedef int (*coreconf_node_write_handler_t)(coreconf_encoder_t *encoder, const coreconf_node_t *node, uint8_t *pdu, size_t len);

struct coreconf_node {
    uint64_t num;
    uint8_t methods;
    coreconf_node_read_handler_t read;
    coreconf_node_write_handler_t write;
};

static inline nanocbor_encoder_t *coreconf_encoder_cbor(coreconf_encoder_t *enc)
{
    return &enc->encoder;
}

static inline bool coreconf_k_param_empty(const coreconf_encoder_t *enc)
{
    return enc->k_param[0] == '\0';
}

/**
 * @brief   Define a CORECONF endpoint
 *
 * This macro is a helper for defining a CORECONF endpoint and adding it to the
 * CORECONF XFA (cross file array).
 *
 * @experimental This should be considered experimental API, subject to change
 *               without notice!
 *
 * Example:
 *
 * ```.c
 * #include "coreconf.h"
 *   // ...
 * }
 * CORECONF_NODE(endpoint, COAP_GET | COAP_PUT, _handler, NULL);
 * ```
 */
#define CORECONF_NODE(num, methods, read, write) \
    XFA_CONST(coreconf_node_xfa, num)                                          \
    coreconf_node_t coreconf_node_ ## num\
        = { num , methods, read, write}


void coreconf_fmt_sid(coreconf_encoder_t *enc, uint64_t parent_sid, uint64_t sid);
#ifdef __cplusplus
}
#endif

#endif /* NET_CORECONF_H */
/** @} */
