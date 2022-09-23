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
#include "fmt.h"
#include "xfa.h"
#include "nanocbor/nanocbor.h"
#include "base64.h"
#include "byteorder.h"

XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa);
XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa_root);
XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa_zzzzzzzzz);

coreconf_state_t _state[CONFIG_CORECONF_COAP_STATE_NUM];

#define CORECONF_XFA_TOTAL_LEN \
    (((uintptr_t)coreconf_node_xfa_zzzzzzzzz - (uintptr_t)coreconf_node_xfa) / \
     sizeof(coreconf_node_t))

static ssize_t _encode_links(const coap_resource_t *resource, char *buf,
                             size_t maxlen, coap_link_encoder_ctx_t *context);
static int _request_matcher(gcoap_listener_t *listener, const coap_resource_t
                            **resource, coap_pkt_t *pdu);

static ssize_t _coreconf_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                 coap_request_ctx_t *context);
static int _read_node(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv);

static const char _link_params[] = ";ct=\"core.c.dn\"";

/* todo: removeme */
uint8_t *coap_iterate_option(coap_pkt_t *pkt, uint8_t **optpos,
                             int *opt_len, int first);

static const coap_resource_t _coreconf_resource = {
    NULL,
    0,
    _coreconf_handler,
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

static size_t coreconf_total_node_len(void)
{
    return ((uintptr_t)coreconf_node_xfa_zzzzzzzzz_end - (uintptr_t)coreconf_node_xfa) / \
           sizeof(coreconf_node_t);
}

static void _coreconf_state_refresh(coreconf_state_t *state)
{
    event_timeout_set(&state->timeout, CONFIG_CORECONF_COAP_STATE_TIMEOUT_SEC);
}

static void _coreconf_state_clear(coreconf_state_t *state)
{
    event_timeout_clear(&state->timeout);
    state->message_id = 0;
    state->method = 0;
}

static void _event_clear(event_t *ev)
{
    coreconf_state_t *state = container_of(ev, coreconf_state_t, ev);

    _coreconf_state_clear(state);
}

static coreconf_state_t *_coreconf_state_new(coap_pkt_t *pdu)
{
    uint8_t code = coap_get_code_raw(pdu);
    uint16_t message_id = coap_get_id(pdu);

    for (size_t i = 0; i < CONFIG_CORECONF_COAP_STATE_NUM; i++) {
        coreconf_state_t *state = &_state[i];
        if (state->method == 0) {
            state->ev.handler = _event_clear;
            event_timeout_ztimer_init(&state->timeout, ZTIMER_SEC, gcoap_get_queue(), &state->ev);

            state->method = code;
            state->message_id = message_id;
            memset(state->uri_query, 0, CONFIG_CORECONF_COAP_ARGS_LEN);
            return state;
        }
    }
    return NULL;
}

static coreconf_state_t *_coreconf_state_find_matching(coap_pkt_t *pdu)
{
    uint8_t code = coap_get_code_raw(pdu);
    uint16_t message_id = coap_get_id(pdu);

    for (size_t i = 0; i < CONFIG_CORECONF_COAP_STATE_NUM; i++) {
        coreconf_state_t *state = &_state[i];
        if (state->method == code && state->message_id == message_id) {
            return state;
        }
    }
    return NULL;
}

size_t coreconf_container_len(const coreconf_node_t *container,
                              bool filter, bool config)
{
    size_t res = 0;

    if (!filter) {
        return ((uintptr_t)container->container.end - (uintptr_t)container->container.target) /
               sizeof(coreconf_node_t);
    }
    const coreconf_node_t *sub = (const coreconf_node_t *)container->container.target;

    while (container->container.end != sub) {
        if (filter) {
            if (coreconf_node_config(sub) == config) {
                res++;
            }
        }
        else {
            res++;
        }
        sub++;
    }
    return res;
}

static void _sid2b64(coreconf_sid_t sid, uint8_t b64_buf[12])
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

static int _b642sid(const char *b64, size_t len, coreconf_sid_t *sid)
{
    uint8_t val_buf[9] = { 0 }; /* 9 bytes to contain 12 bytes of base64 */
    uint8_t b64buf[12] = "AAAAAAAAAAAA";
    network_uint64_t big_num = { 0 };
    size_t offset = sizeof(b64buf) - len;

    memcpy(&b64buf[offset], b64, len);

    size_t tmp = sizeof(val_buf);

    if (base64_decode(b64buf, sizeof(b64buf), val_buf, &tmp) < 0) {
        return -1;
    }
    memcpy(&big_num, &val_buf[1], sizeof(big_num));
    *sid = byteorder_ntohll(big_num);
    return 0;
}

static int _pdu2sid(coap_pkt_t *pdu, coreconf_sid_t *sid)
{
    char url_buffer[strlen(CONFIG_CORECONF_STORE_SUBTREE) + 13];

    size_t len = strlen(CONFIG_CORECONF_STORE_SUBTREE);

    if (coap_opt_get_string(pdu, COAP_OPT_URI_PATH, (uint8_t *)url_buffer,
                            sizeof(url_buffer), '/') <= 0) {
        /* The Uri-Path options are longer than
         * CONFIG_NANOCOAP_URI_MAX, and thus do not match anything
         * that could be found by this handler. */
        return -1;
    }

    int res =
        strncmp(url_buffer, CONFIG_CORECONF_STORE_SUBTREE, strlen(CONFIG_CORECONF_STORE_SUBTREE));

    /* check for "/c", root of the config store */
    if (strncmp(url_buffer, CONFIG_CORECONF_STORE_SUBTREE, sizeof(url_buffer)) == 0) {
        /* signal root node */
        sid = 0;
        return 0;
    }
    /* check for "/c/" */
    if (res != 0 || url_buffer[len] != '/') {
        return -1;
    }

    char *sid_start = url_buffer + len + 1;

    return _b642sid(sid_start, strlen(sid_start), sid);
}

static const coreconf_node_t *_find_coreconf_node(coreconf_sid_t sid)
{
    (void)sid;
    for (size_t i = 0; i < coreconf_total_node_len(); i++) {
        const coreconf_node_t *node = (const coreconf_node_t *)&coreconf_node_xfa[i];
        /* TODO: make generic */
        if (coreconf_node_sid(node) == sid) {
            return node;
        }
    }
    return NULL;
}

static bool _node_has_read(const coreconf_node_t *node)
{
    switch (coreconf_node_type(node)) {
    case CORECONF_NODE_LEAF:
        return node->leaf.read;
    case CORECONF_NODE_CONTAINER:
        return true;
    default:
        return false;
    }
}

static bool _node_has_write(const coreconf_node_t *node)
{
    switch (coreconf_node_type(node)) {
    case CORECONF_NODE_LEAF:
        return node->leaf.write;
    case CORECONF_NODE_CONTAINER:
        return true;
    default:
        return false;
    }
}

static int _request_matcher(gcoap_listener_t *listener, const coap_resource_t
                            **resource, coap_pkt_t *pdu)
{
    (void)listener;

    unsigned code = coap_get_code_detail(pdu);

    coreconf_sid_t sid = 0;

    if (_pdu2sid(pdu, &sid) < 0) {
        return GCOAP_RESOURCE_NO_PATH;
    }

    if (sid == 0) {
        *resource = &_coreconf_resource;
        if (code == COAP_METHOD_GET ||
                code == COAP_METHOD_FETCH ||
                code == COAP_METHOD_IPATCH) {
            return GCOAP_RESOURCE_FOUND;
        }

    }

    int ret = GCOAP_RESOURCE_NO_PATH;

    const coreconf_node_t *node = _find_coreconf_node(sid);

    if (node) {
        if (code == COAP_METHOD_GET && _node_has_read(node)) {
            *resource = &_coreconf_resource;
            ret = GCOAP_RESOURCE_FOUND;
        }
        else if ((code == COAP_METHOD_PUT || code == COAP_METHOD_POST ||
                  code == COAP_METHOD_DELETE) && _node_has_write(node)) {
            *resource = &_coreconf_resource;
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
    size_t idx = index_of(_listener.resources, resource);
    const coreconf_node_t *node = (const coreconf_node_t *)&coreconf_node_xfa[idx];

    _sid2b64(coreconf_node_sid(node), base64_buf);

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
        memcpy(&buf[offset], CONFIG_CORECONF_STORE_SUBTREE, strlen(CONFIG_CORECONF_STORE_SUBTREE));
        offset += strlen(CONFIG_CORECONF_STORE_SUBTREE);
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

static inline bool _is_odd(size_t len)
{
    return len & 1;
}

static void _nanocbor_append(nanocbor_encoder_t *enc, void *ctx, const uint8_t *buf, size_t len)
{
    (void)enc;
    coreconf_slicer_helper_t *slice_helper = ctx;

    if (len == 0) {
        return;
    }
    slice_helper->sliced_length +=
        coap_blockwise_put_bytes(&slice_helper->slicer,
                                 slice_helper->buf + slice_helper->sliced_length,
                                 buf, len);
    /* update etag fletcher32 */
    if (_is_odd(slice_helper->resp_len)) {
        uint16_t tmp = (slice_helper->fletcher_tmp << 8) | buf[0];
        fletcher32_update(&slice_helper->fltchr, &tmp, 1);
        len--;
        buf++;
        slice_helper->resp_len++;
    }
    uint16_t *u16buf = (void *)buf;

    fletcher32_update(&slice_helper->fltchr, u16buf, len / 2);
    slice_helper->resp_len += len;
    if (_is_odd(len)) {
        /* store the last byte for the next round */
        slice_helper->fletcher_tmp = buf[len - 1];
    }
}

ssize_t coreconf_read_container(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    assert(coreconf_node_type(node) == CORECONF_NODE_CONTAINER);
    if (node->container.aux && node->container.aux->read) {
        return node->container.aux->read(enc, node, argv);
    }
    size_t len =
        coreconf_container_len(node, enc->ctx.config != 'a', enc->ctx.config == 'c');

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), len);

    const coreconf_node_t *child = (const coreconf_node_t *)node->container.target;

    while (child != node->container.end) {
        if ((enc->ctx.config == 'a') ||
            (enc->ctx.config == 'c' && coreconf_node_config(child)) ||
            (enc->ctx.config == 'n' && !coreconf_node_config(child))) {
            coreconf_cbor_sid(enc, coreconf_node_sid(node), coreconf_node_sid(child));
            ssize_t res = _read_node(enc, (const coreconf_node_t *)child, argv);
            if (res != 0) {
                return res;
            }
        }
        child++;
    }
    return 0;
}

static ssize_t _read_node(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    if (coreconf_node_type(node) == CORECONF_NODE_CONTAINER) {
        if (node->container.aux && node->container.aux->parse) {
            return node->container.aux->parse(&enc->ctx, node, argv);
        }
        else {
            return coreconf_read_container(enc, node, argv);
        }
    }
    else {
        return node->leaf.read(enc, node, argv);
    }
}

static ssize_t _fmt_sid(coreconf_encoder_t *enc, coreconf_sid_t parent_sid, coreconf_sid_t sid)
{
    coreconf_sid_t fmt_sid = sid - parent_sid;

    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), fmt_sid);

    const coreconf_node_t *node = _find_coreconf_node(sid);

    if (!node) {
        return CORECONF_ERR_NOT_FOUND;
    }

    void *argv[CONFIG_CORECONF_COAP_ARGS_NUM] = { 0 };

    return _read_node(enc, node, argv);
}

static ssize_t _read_root(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    size_t len = XFA_LEN(coreconf_node_t, coreconf_node_xfa_root);

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), len);

    for (size_t i = 0; i < len; i++) {
        node = (const coreconf_node_t *)&coreconf_node_xfa_root[i];

        nanocbor_fmt_uint(coreconf_encoder_cbor(enc), coreconf_node_sid(node));
        ssize_t res = _read_node(enc, node, argv);
        if (res != 0) {
            return res;
        }
    }
    return 0;
}

static ssize_t _fetch_root(coreconf_encoder_t *enc)
{
    /* Decode the payload and construct the individual bits */
    nanocbor_value_t outer, inner;

    nanocbor_decoder_init(&outer, enc->ctx.state->request_data, CONFIG_CORECONF_COAP_ARGS_LEN);
    if (nanocbor_enter_array(&outer, &inner) < 0) {
        return coreconf_reply_error(&enc->ctx, NULL, CORECONF_OPERATION_FAILED,
                                    CORECONF_MALFORMED_MESSAGE, NULL);
    }

    size_t num_sids = nanocbor_container_remaining(&inner);

    nanocbor_fmt_array(coreconf_encoder_cbor(enc), num_sids);
    uint64_t sid = 0;

    while (!nanocbor_at_end(&inner)) {
        int type = nanocbor_get_type(&inner);
        if (type == NANOCBOR_TYPE_UINT) {
            /* Single sid without args */
            nanocbor_get_uint64(&inner, &sid);
            /* No arguments to decode */
            nanocbor_decoder_init(&enc->ctx.decoder, NULL, 0);
        }
        else if (type == NANOCBOR_TYPE_ARR) {
            /* Array with one sid and number of args */
            nanocbor_enter_array(&inner, &enc->ctx.decoder);
            if (nanocbor_get_uint64(&enc->ctx.decoder, &sid) < 0) {
                return -1;
            }
            /* Don't rely on the handler to exhaust the arguments */
            nanocbor_skip(&inner);
        }
        else {
            return coreconf_reply_error(&enc->ctx, NULL, CORECONF_OPERATION_FAILED,
                                        CORECONF_MALFORMED_MESSAGE, NULL);
        }
        nanocbor_fmt_map(coreconf_encoder_cbor(enc), 1);
        ssize_t res = _fmt_sid(enc, 0, sid);
        if (res != 0) {
            return res;
        }
    }
    return 0;
}

static void _init_encoder(coreconf_encoder_t *encoder)
{
    memset(encoder, 0, sizeof(coreconf_encoder_t));
    encoder->slicer.sliced_length = 0;
    encoder->ctx.szx2 = CONFIG_NANOCOAP_BLOCK_SIZE_EXP_MAX,
    encoder->ctx.config = 'a';
}

static void _init_decoder(coreconf_decoder_t *decoder)
{
    (void)decoder;
}

static int _parse_uri_query(coreconf_ctx_t *ctx,
                            uint8_t *value, ssize_t optlen)
{
    /* checked by the caller */
    assert(optlen >= 3);
    if (value[1] != '=') {
        return COAP_CODE_BAD_REQUEST;
    }
    switch (value[0]) {
    case 'k':
        /* Option too long to copy or fetch method */
        if (optlen > (CONFIG_CORECONF_COAP_ARGS_LEN + 1) ||
            ctx->state->method == COAP_METHOD_FETCH) {
            return COAP_CODE_NOT_ACCEPTABLE;
        }
        /* copy only the value */
        char *uri_query = ctx->state->uri_query;
        memcpy(uri_query, &value[2], optlen - 2);
        char *pos = uri_query;
        while ((pos = strchr(pos, ',')) != NULL) {
            *pos++ = '\0';
        }
        return 0;
    case 'c':
    {
        char setting = value[2];
        if (optlen == 3 && (
                setting == 'a' ||
                setting == 'c' ||
                setting == 'n'
                )) {
            ctx->config = setting;
            return 0;
        }
        return COAP_CODE_BAD_REQUEST;
    }
    case 'd':
    {
        char setting = value[2];
        if (optlen == 3 && (
                setting == 'a' ||
                setting == 't'
                )) {
            ctx->with_default = setting;
            return 0;
        }
        return COAP_CODE_BAD_REQUEST;
    }
    default:
        return COAP_CODE_BAD_REQUEST;
    }
}

static int _parse_opts(coap_pkt_t *pdu, coreconf_ctx_t *ctx)
{
    coap_optpos_t opt = {
        .offset = coap_get_total_hdr_len(pdu),
    };
    uint8_t *value;
    ssize_t optlen;

    /* Needs the etag, block2, the uri queries and at some point the block1 */
    while ((optlen = coap_opt_get_next(pdu, &opt, &value, 0)) != -ENOENT) {

        if (optlen < 0) {
            return COAP_CODE_BAD_REQUEST;
        }

        switch (opt.opt_num) {
        case COAP_OPT_URI_HOST:
        case COAP_OPT_URI_PATH:
            break;
        case COAP_OPT_URI_QUERY:
            /* Expecting values as k=something, must be at least 3 chars
             * long */
            if (optlen < 3) {
                return COAP_CODE_BAD_REQUEST;
            }
            else {
                int res = _parse_uri_query(ctx, value, optlen);
                if (res != 0) {
                    return res;
                }
            }
            break;
        case COAP_OPT_ETAG:
            if (optlen != sizeof(ctx->etag)) {
                /* Can't be a matching tag, no use in carrying that */
                continue;
            }
            if (ctx->etag_sent) {
                /* We can reasonably only check for a limited sized set,
                 * and it size is 1 here (sending multiple ETags is
                 * possible but rare) */
                continue;
            }
            ctx->etag_sent = true;
            memcpy(&ctx->etag, value, sizeof(ctx->etag));
            break;
        case COAP_OPT_BLOCK2:
            coap_get_blockopt(pdu, COAP_OPT_BLOCK2, &ctx->blocknum2, &ctx->szx2);
            break;
        default:
            if (opt.opt_num & 1) {
                return COAP_CODE_BAD_REQUEST;
            }
            else {
                /* Ignoring elective option */
            }
        }
    }
    return 0;
}

static ssize_t _coreconf_read_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                      coap_request_ctx_t *context)
{
    (void)context;
    coreconf_encoder_t encoder;

    _init_encoder(&encoder);

    coreconf_state_t *state = _coreconf_state_find_matching(pdu);

    if (!state) {
        state = _coreconf_state_new(pdu);
    }
    if (!state) {
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }

    encoder.ctx.state = state;
    encoder.ctx.pdu_len = len;
    encoder.ctx.pdu = pdu;
    assert(encoder.ctx.state);

    coreconf_sid_t sid = 0;

    if (_pdu2sid(pdu, &sid)) {
        return gcoap_response(pdu, buf, len, COAP_CODE_NOT_ACCEPTABLE);
    }
    ssize_t res = _parse_opts(pdu, &encoder.ctx);

    if (res < 0) {
        return res;
    }

    if (encoder.ctx.state->method == COAP_METHOD_FETCH) {
        /* Copy the payload into the k_param buffer */
        if (pdu->payload_len > CONFIG_CORECONF_COAP_ARGS_LEN) {
            return gcoap_response(pdu, buf, len, COAP_CODE_REQUEST_ENTITY_TOO_LARGE);
        }
        if (encoder.ctx.blocknum2 == 0) {
            memcpy(encoder.ctx.state->request_data, pdu->payload, pdu->payload_len);
        }
    }

    uint32_t etag = 0;

    /* initialize response packet */
    coap_block_slicer_init(&encoder.slicer.slicer,
                           encoder.ctx.blocknum2, coap_szx2size(encoder.ctx.szx2));
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

    coap_opt_add_opaque(pdu, COAP_OPT_ETAG, &etag, sizeof(etag));
    uint8_t *etag_ptr = pdu->payload;

    coap_opt_add_format(pdu, CORECONF_COAP_FORMAT); /* Yang data + cbor */
    coap_opt_add_block2(pdu, &encoder.slicer.slicer, 1);
    ssize_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    encoder.slicer.buf = buf + resp_len;

    nanocbor_encoder_stream_init(coreconf_encoder_cbor(&encoder), &encoder.slicer,
                                 _nanocbor_append, _nanocbor_fits);

    if (sid == 0) {
        void *argv[CONFIG_CORECONF_COAP_ARGS_NUM] = { 0 };
        if (encoder.ctx.state->method == COAP_METHOD_GET) {
            res = _read_root(&encoder, NULL, argv);
        }
        else if (encoder.ctx.state->method == COAP_METHOD_FETCH) {
            res = _fetch_root(&encoder);
        }
        else {
            return gcoap_response(pdu, buf, len, COAP_CODE_METHOD_NOT_ALLOWED);
        }
    }

    else {
        nanocbor_fmt_map(coreconf_encoder_cbor(&encoder), 1);

        res = _fmt_sid(&encoder, 0, sid);

        if (res < 0) {
            return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
        }
    }

    if (res != 0) {
        /* coap_reply_error called */
        return res;
    }
    bool more = coap_block2_finish(&encoder.slicer.slicer);

    /* Wrap up the etag calculation */
    if (_is_odd(encoder.slicer.resp_len)) {
        fletcher32_update(&encoder.slicer.fltchr, &encoder.slicer.fletcher_tmp, 1);
    }
    /* Adding the length to the checksum prevents issues with the additional
     * zero byte with odd length payloads */
    uint32_t total_len = encoder.slicer.resp_len;

    fletcher32_update(&encoder.slicer.fltchr, (uint16_t *)&total_len, sizeof(total_len) / 2);
    etag = fletcher32_finish(&encoder.slicer.fltchr);
    if (!more) {
        _coreconf_state_clear(state);
    }

    if (encoder.ctx.etag_sent && etag == encoder.ctx.etag) {
        return gcoap_response(pdu, buf, len, COAP_CODE_VALID);
    }

    /* Copy in the etag */
    memcpy(etag_ptr - sizeof(etag), &etag, sizeof(etag));

    _coreconf_state_refresh(state);
    return resp_len + encoder.slicer.sliced_length;
}

static ssize_t _coreconf_write_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                       coap_request_ctx_t *context)
{
    (void)context;
    coreconf_decoder_t decoder;

    _init_decoder(&decoder);

    coreconf_state_t *state = _coreconf_state_find_matching(pdu);

    if (!state) {
        state = _coreconf_state_new(pdu);
    }
    if (!state) {
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }

    decoder.ctx.state = state;
    decoder.ctx.pdu_len = len;
    decoder.ctx.pdu = pdu;
    assert(state);
    decoder.ctx.state->method = coap_get_code_raw(pdu);

    ssize_t res = _parse_opts(pdu, &decoder.ctx);

    if (res < 0) {
        return res;
    }

    coreconf_sid_t sid = 0;

    if (_pdu2sid(pdu, &sid) < 0) {
        return gcoap_response(pdu, buf, len, COAP_CODE_NOT_ACCEPTABLE);
    }

    if (coreconf_ctx_get_method(&decoder.ctx) == COAP_METHOD_DELETE) {
        nanocbor_decoder_init(&decoder.ctx.decoder, NULL, 0);
    }
    else {
        nanocbor_value_t outer_map;
        nanocbor_decoder_init(&outer_map, pdu->payload, pdu->payload_len);
        /* todo: walk through the whole cbor struct to validate it */

        nanocbor_value_t inner_map;
        if (nanocbor_enter_map(&outer_map, &inner_map) != NANOCBOR_OK) {
            goto cbor_fmt_err;
        }

        uint64_t inner_sid;
        if ((nanocbor_get_uint64(&inner_map, &inner_sid) < NANOCBOR_OK) ||
            (inner_sid != sid)) {
            goto cbor_fmt_err;
        }

        decoder.ctx.decoder = inner_map; /* now points to the actual content */

        nanocbor_skip(&inner_map);
        if (!nanocbor_at_end(&inner_map)) {
            goto cbor_fmt_err;
        }

        nanocbor_leave_container(&outer_map, &inner_map);

        if (!nanocbor_at_end(&outer_map)) {
            goto cbor_fmt_err;
        }
    }

    /* Validated the outer cbor somewhat, delegate to handler */
    const coreconf_node_t *node = _find_coreconf_node(sid);

    void *argv[CONFIG_CORECONF_COAP_ARGS_NUM] = { 0 };

    ssize_t wres = 0;

    switch (coreconf_node_type(node)) {
    case CORECONF_NODE_LEAF:
        wres = node->leaf.write(&decoder, node, argv);
        break;
    case CORECONF_NODE_CONTAINER:
        if (node->container.aux && node->container.aux->write) {
            wres = node->container.aux->write(&decoder, node, argv);
        }
        break;
    default:
        wres = CORECONF_ERR_NOT_FOUND;
    }


    if (wres < 0) {
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }
    if (wres == 0) {
        _coreconf_state_refresh(state);
        /* Todo, lookup and use method */
        return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
    }
    return wres;

cbor_fmt_err:
    _coreconf_state_refresh(state);
    return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
}

static ssize_t _coreconf_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                 coap_request_ctx_t *context)
{
    switch (coap_get_code_raw(pdu)) {
    case COAP_METHOD_FETCH:
    case COAP_METHOD_GET:
        return _coreconf_read_handler(pdu, buf, len, context);
    case COAP_METHOD_POST:
    case COAP_METHOD_PUT:
    case COAP_METHOD_DELETE:
        return _coreconf_write_handler(pdu, buf, len, context);
    default:
        return gcoap_response(pdu, buf, len, COAP_CODE_METHOD_NOT_ALLOWED);
    }
}

void coreconf_init(void)
{
    _listener.resources_len = CORECONF_XFA_TOTAL_LEN;
    gcoap_register_listener(&_listener);
}

static const char *_coreconf_arg_uri_at(const coreconf_state_t *state, size_t pos)
{
    const char *arg = state->uri_query;

    while (pos--) {
        arg += strlen(arg) + 1;
    }
    return arg;
}

int coreconf_arg_as_uint(const coreconf_ctx_t *ctx, int offset, uint64_t *val)
{
    const coreconf_state_t *state = ctx->state;

    assert(offset >= 0);

    if (state->method == COAP_METHOD_GET) {
        /* decode from url string */
        const char *arg = _coreconf_arg_uri_at(state, offset);
        if (!arg) {
            return -1;
        }
        if (!fmt_is_number(arg)) {
            return -1;
        }
        *val = scn_u32_dec(arg, strlen(arg));
        return 0;
    }
    else if (state->method == COAP_METHOD_FETCH) {
        /* decode as cbor from payload */

    }
    return -1;
}

int coreconf_arg_as_str(const coreconf_ctx_t *ctx, int offset, const char **val)
{
    const coreconf_state_t *state = ctx->state;

    assert(offset >= 0);

    if (state->method == COAP_METHOD_GET ||
        state->method == COAP_METHOD_PUT ||
        state->method == COAP_METHOD_POST ||
        state->method == COAP_METHOD_DELETE) {
        /* decode from url string */
        *val = _coreconf_arg_uri_at(state, offset);
        if (!val || (*val)[0] == '\0') {
            return -1;
        }
        return 0;
    }
    else if (state->method == COAP_METHOD_FETCH ||
             state->method == COAP_METHOD_IPATCH) {
        /* decode as cbor from payload */
        if (nanocbor_at_end(&ctx->decoder)) {
            /* No argument */
            return -1;
        }
        nanocbor_value_t decoder = ctx->decoder;
        while (offset--) {
            nanocbor_skip(&decoder);
        }
        size_t len;
        if (nanocbor_get_tstr(&decoder, (const uint8_t **)val, &len) < 0) {
            return -1;
        }
        return 0;
    }
    return -1;
}

ssize_t coreconf_reply_error(const coreconf_ctx_t *ctx,
                             const coreconf_node_t *node, coreconf_error_tag_t error_tag,
                             coreconf_app_tag_t app_tag, const char *error_msg)
{
    gcoap_resp_init(ctx->pdu, (uint8_t *)ctx->pdu->hdr, ctx->pdu_len, COAP_CODE_BAD_REQUEST);
    coap_opt_add_format(ctx->pdu, CORECONF_COAP_FORMAT); /* Yang data + cbor */
    ssize_t resp_len = coap_opt_finish(ctx->pdu, COAP_OPT_FINISH_PAYLOAD);
    size_t remaining_len = ctx->pdu_len - resp_len;

    coreconf_sid_t sid = 0;

    if (node) {
        sid = coreconf_node_sid(node);
    }

    nanocbor_encoder_t enc;

    nanocbor_encoder_init(&enc, (uint8_t *)ctx->pdu->payload, remaining_len);

    nanocbor_fmt_map(&enc, 1);
    nanocbor_fmt_uint(&enc, 1024);

    nanocbor_fmt_map(&enc, 4);
    nanocbor_fmt_uint(&enc, 1);
    nanocbor_fmt_uint(&enc, app_tag);
    nanocbor_fmt_uint(&enc, 2);
    nanocbor_fmt_uint(&enc, sid);
    nanocbor_fmt_uint(&enc, 3);
    if (error_msg) {
        nanocbor_put_tstr(&enc, error_msg);
    }
    else {
        nanocbor_put_tstr(&enc, "");
    }
    nanocbor_fmt_uint(&enc, 4);
    nanocbor_fmt_uint(&enc, error_tag);
    size_t encoded_len = nanocbor_encoded_len(&enc);

    if (encoded_len >  remaining_len) {
        return resp_len + remaining_len;
    }
    return resp_len + encoded_len;
}
