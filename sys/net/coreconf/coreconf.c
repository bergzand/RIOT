/*
 * Copyright (C) 2022, Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_coreconf
 * @{
 * @file
 * @brief       CORECONF implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#include "kernel_defines.h"
#include "net/coreconf.h"
#include "net/nanocoap.h"
#include "net/gcoap.h"
#include "xfa.h"
#include "nanocbor/nanocbor.h"
#include "base64.h"
#include "byteorder.h"

XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa);


static ssize_t _encode_links(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context);
static int _request_matcher(gcoap_listener_t *listener, const coap_resource_t
        **resource, coap_pkt_t *pdu);

static ssize_t _coreconf_read_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *context);
static ssize_t _coreconf_write_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *context);

static const char _link_params[] = ";ct=\"core.c.dn\"";

/* todo: removeme */
uint8_t *coap_iterate_option(coap_pkt_t *pkt, uint8_t **optpos,
                             int *opt_len, int first);

static const coap_resource_t _read_resource = {
    NULL,
    0,
    _coreconf_read_handler,
    NULL
};

static const coap_resource_t _write_resource = {
    NULL,
    0,
    _coreconf_write_handler,
    NULL
};

static gcoap_listener_t _listener = {
    NULL,
    0,
    GCOAP_SOCKET_TYPE_UNDEF,
    _encode_links,
    NULL,
    _request_matcher
};

static void _sid2b64(uint64_t sid, uint8_t b64_buf[12])
{
    uint8_t val_buf[9] = { 0 };
    network_uint64_t big_num = byteorder_htonll(sid);

    memcpy(&val_buf[1], &big_num, sizeof(uint64_t));

    size_t tmp_len = 12;
    base64url_encode(val_buf, sizeof(val_buf), b64_buf, &tmp_len);
    assert(tmp_len == 12);
}

static size_t _b64offset(const uint8_t *b64_buf, size_t len)
{
    size_t b64offset = 0;
    for (; b64offset < len; b64offset++) {
        if (b64_buf[b64offset] != 'A') {
            break;
        }
    }
    return b64offset;
}

static uint64_t _b642sid(char *b64, size_t len)
{
    uint8_t val_buf[9] = { 0 };
    uint8_t b64buf[12] = "AAAAAAAAAAAA";
    network_uint64_t big_num = { 0 };
    size_t offset = sizeof(b64buf) - len;

    memcpy(&b64buf[offset], b64, len);

    size_t tmp = sizeof(val_buf);
    base64_decode(b64buf, sizeof(b64buf), val_buf, &tmp);
    memcpy(&big_num, &val_buf[1], sizeof(big_num));
    return byteorder_ntohll(big_num);
}

static uint64_t _pdu2sid(coap_pkt_t *pdu)
{
    char url_buffer[strlen(CORECONF_STORE_SUBTREE) + 13];

    size_t len = strlen(CORECONF_STORE_SUBTREE);

    if (coap_opt_get_string(pdu, COAP_OPT_URI_PATH, (uint8_t*)url_buffer,
                               sizeof(url_buffer), '/') <= 0) {
        /* The Uri-Path options are longer than
         * CONFIG_NANOCOAP_URI_MAX, and thus do not match anything
         * that could be found by this handler. */
        return GCOAP_RESOURCE_NO_PATH;
    }

    int res = strncmp(url_buffer, CORECONF_STORE_SUBTREE, strlen(CORECONF_STORE_SUBTREE));
    if (res != 0 || url_buffer[len] != '/') {
        return 0;
    }

    char *sid_start = url_buffer + len + 1;

    uint64_t sid = _b642sid(sid_start, strlen(sid_start));

    return sid;
}

static const coreconf_node_t *_find_coreconf_node(uint64_t sid)
{
    for (size_t i = 0; i < XFA_LEN(coreconf_node_t, coreconf_node_xfa); i++) {
        const coreconf_node_t *node = (const coreconf_node_t*)&coreconf_node_xfa[i];
        if (node->num == sid) {
            return node;
        }
        else if (node->num > sid) {
            break;
        }
    }
    return NULL;
}

static int _request_matcher(gcoap_listener_t *listener, const coap_resource_t
        **resource, coap_pkt_t *pdu)
{
    (void)listener;

    unsigned code = coap_get_code_detail(pdu);

    uint64_t sid = _pdu2sid(pdu);
    if (sid == 0) {
        return GCOAP_RESOURCE_NO_PATH;
    }

    int ret = GCOAP_RESOURCE_NO_PATH;

    const coreconf_node_t *node = _find_coreconf_node(sid);

    if (node) {
        if (code == COAP_METHOD_GET && node->read) {
            *resource = &_read_resource;
            ret = GCOAP_RESOURCE_FOUND;
        }
        else if ((code == COAP_METHOD_PUT || code == COAP_METHOD_POST) && node->write) {
            *resource = &_write_resource;
            ret = GCOAP_RESOURCE_FOUND;
        }
        else {
            ret = GCOAP_RESOURCE_WRONG_METHOD;
        }
    }

    /* check if the url is valid and return generic */
    return ret;
}

static ssize_t _encode_links(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context)
{
    (void)maxlen;
    uint8_t base64_buf[12] = { 0 };

    size_t exp_len = 0;
    size_t offset = 0;
    /* iterate over the sids present */
    size_t idx = index_of((coap_resource_t*)NULL, resource);
    const coreconf_node_t *node = (const coreconf_node_t*)&coreconf_node_xfa[idx];

    _sid2b64(node->num, base64_buf);

    size_t b64offset = _b64offset(base64_buf, sizeof(base64_buf));
    size_t b64len = sizeof(base64_buf) - b64offset;

    /* </c/NUM> */
    exp_len += 5 + b64len + strlen(_link_params);

    if (!(context->flags & COAP_LINK_FLAG_INIT_RESLIST)) {
        /* account for possibly leading comma */
        exp_len += 1;
    }

    if (exp_len > maxlen) {
        return -1;
    }

    if (buf) {
        if (!(context->flags & COAP_LINK_FLAG_INIT_RESLIST)) {
            buf[offset++] = ',';
        }
        buf[offset++] = '<';
        buf[offset++] = '/';
        buf[offset++] = 'c';
        buf[offset++] = '/';
        memcpy(&buf[offset], &base64_buf[b64offset], b64len);
        offset += b64len;
        buf[offset++] = '>';
        memcpy(&buf[offset], _link_params, strlen(_link_params));
        offset += strlen(_link_params);
    }

    return exp_len;
}

static bool _nanocbor_fits(nanocbor_encoder_t *enc, void *ctx, size_t len)
{
    (void)enc;
    (void)ctx;
    (void)len;
    return true; /* Always more space on the block2 */
}

static void _nanocbor_append(nanocbor_encoder_t *enc, void *ctx, const uint8_t *buf, size_t len)
{
    (void)enc;
    coreconf_slicer_helper_t *slice_helper = ctx;
    slice_helper->sliced_length +=
        coap_blockwise_put_bytes(&slice_helper->slicer,
                                 slice_helper->buf + slice_helper->sliced_length,
                                 buf, len);
}

int coreconf_fmt_sid(coreconf_encoder_t *enc, uint64_t parent_sid, uint64_t sid)
{
    /* BUG: uint64 to int64 conversion */
    int64_t fmt_sid = sid - parent_sid;

    nanocbor_fmt_int(coreconf_encoder_cbor(enc), fmt_sid);

    const coreconf_node_t *node = _find_coreconf_node(sid);
    assert(node);

    ssize_t res = node->read(enc, node);
    if (res < 0) {
        return res;
    }
    return 0;
}

static void _init_encoder(coreconf_encoder_t *encoder)
{
    encoder->slicer.sliced_length = 0;
    memset(encoder->k_param, 0, CORECONF_COAP_K_LEN);
}

static int _copy_k_param(coap_pkt_t *pdu, coreconf_encoder_t *enc)
{
    uint8_t *opt_pos = coap_find_option(pdu, COAP_OPT_URI_QUERY);
    if (!opt_pos) {
        enc->k_param[0] = '\0';
        return 0;
    }

    uint8_t *query_start = NULL;
    do {
        int opt_len;
        query_start = coap_iterate_option(pdu, &opt_pos, &opt_len,
                                         (query_start == NULL));
        /* valid k option */
        if (query_start && opt_len > 2 && query_start[0] == 'k' && query_start[1] == '=') {
            if (opt_len > (CORECONF_COAP_K_LEN + 1)) {
                return -ENOSPC;
            }
            /* copy only the value */
            memcpy(enc->k_param, &query_start[2], opt_len - 2);
            break;
        }
    } while(opt_pos);

    char *k_arg = enc->k_param;

    enc->num_k_args = (*k_arg == '\0') ? 0 : 1;

    while (*k_arg != '\0') {
        if (*k_arg == ',') {
            *k_arg = '\0';
            enc->num_k_args++;
        }
        k_arg++;
    }

    return 0;
}

static ssize_t _coreconf_read_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *context)
{
    (void)context;
    coreconf_encoder_t encoder;

    _init_encoder(&encoder);
    uint64_t sid = _pdu2sid(pdu);
    int res = _copy_k_param(pdu, &encoder);
    if (res < 0) {
        return res;
    }

    coap_block2_init(pdu, &encoder.slicer.slicer);
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

    coap_opt_add_format(pdu, CORECONF_COAP_FORMAT); /* Yang data + cbor */
    coap_opt_add_block2(pdu, &encoder.slicer.slicer, 1);
    ssize_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    encoder.slicer.buf = buf + resp_len;

    nanocbor_encoder_stream_init(coreconf_encoder_cbor(&encoder), &encoder.slicer,
                                 _nanocbor_append, _nanocbor_fits);

    nanocbor_fmt_map(coreconf_encoder_cbor(&encoder), 1);

    res = coreconf_fmt_sid(&encoder, 0, sid);

    if (res < 0) {
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }

    coap_block2_finish(&encoder.slicer.slicer);

    return resp_len + encoder.slicer.sliced_length;
}

static ssize_t _coreconf_write_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *context)
{
    (void)context;
    (void)pdu;
    (void)buf;
    (void)len;

    return COAP_CODE_NOT_IMPLEMENTED;
}

void coreconf_init(void)
{
    _listener.resources_len = XFA_LEN(coreconf_node_t, coreconf_node_xfa);
    gcoap_register_listener(&_listener);
}
