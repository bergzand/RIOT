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
#include "net/nanocoap/nanocbor_helper.h"
#include "macros/utils.h"

XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa);
XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa_root);
XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa_rpc);
XFA_INIT_CONST(coreconf_node_t, coreconf_node_xfa_zzzzzzzzz);

XFA_INIT_CONST(coreconf_node_t, coreconf_rpc_xfa);

coreconf_memo_t _memo[CONFIG_CORECONF_COAP_STATE_NUM];

#define CORECONF_XFA_TOTAL_LEN \
    (((uintptr_t)coreconf_node_xfa_zzzzzzzzz - (uintptr_t)coreconf_node_xfa) / \
     sizeof(coreconf_node_t))

static ssize_t _coreconf_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                 coap_request_ctx_t *context);
static int _read_node(coreconf_encoder_t *enc, const coreconf_node_t *node,void **args);

//static const char _link_params[] = ";ct=\"core.c.dn\"";

static const coap_resource_t _coreconf_resource[] = {{
    "/c",
    (COAP_FETCH|COAP_GET|COAP_POST|COAP_IPATCH),
    _coreconf_handler,
    NULL
}};

static gcoap_listener_t _listener = {
    _coreconf_resource,
    ARRAY_SIZE(_coreconf_resource),
    GCOAP_SOCKET_TYPE_UNDEF,
    NULL,
    NULL,
    NULL
};

static size_t coreconf_total_node_len(void)
{
    return ((uintptr_t)coreconf_node_xfa_zzzzzzzzz_end - (uintptr_t)coreconf_node_xfa) / \
           sizeof(coreconf_node_t);
}

static void _coreconf_memo_refresh(coreconf_memo_t *memo)
{
    event_timeout_set(&memo->timeout, CONFIG_CORECONF_COAP_STATE_TIMEOUT_SEC);
}

static void _coreconf_memo_clear(coreconf_memo_t *memo)
{
    event_timeout_clear(&memo->timeout);
    memo->message_id = 0;
    memo->method = 0;
}

static void _event_clear(event_t *ev)
{
    coreconf_memo_t *memo = container_of(ev, coreconf_memo_t, ev);

    _coreconf_memo_clear(memo);
}

static coreconf_memo_t *_coreconf_memo_new(coap_pkt_t *pdu)
{
    uint8_t code = coap_get_code_raw(pdu);
    uint16_t message_id = coap_get_id(pdu);

    for (size_t i = 0; i < CONFIG_CORECONF_COAP_STATE_NUM; i++) {
        coreconf_memo_t *memo = &_memo[i];
        if (memo->method == 0) {
            memo->ev.handler = _event_clear;
            event_timeout_ztimer_init(&memo->timeout, ZTIMER_SEC, gcoap_get_queue(), &memo->ev);

            memo->method = code;
            memo->message_id = message_id;
            memset(memo->request_data, 0, CONFIG_CORECONF_COAP_ARGS_LEN);
            return memo;
        }
    }
    return NULL;
}

static coreconf_memo_t *_coreconf_memo_find_matching(coap_pkt_t *pdu)
{
    uint8_t code = coap_get_code_raw(pdu);
    uint16_t message_id = coap_get_id(pdu);

    for (size_t i = 0; i < CONFIG_CORECONF_COAP_STATE_NUM; i++) {
        coreconf_memo_t *memo = &_memo[i];
        if (memo->method == code && memo->message_id == message_id) {
            return memo;
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
    case CORECONF_NODE_RPC:
    case CORECONF_NODE_LEAF:
        return node->leaf.write;
    case CORECONF_NODE_CONTAINER:
        return true;
    default:
        return false;
    }
}

ssize_t coreconf_read_container(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    assert(coreconf_node_type(node) == CORECONF_NODE_CONTAINER);
    if (node->container.aux && node->container.aux->read) {
        return node->container.aux->read(enc, node, argv);
    }
    size_t len =
        coreconf_container_len(node, enc->ctx.config != CORECONF_QUERY_CONFIG_ALL, enc->ctx.config == 'c');

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

    void *argv[1] = { 0 };

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

static ssize_t _decode_single_fetch(coreconf_encoder_t *enc, nanocbor_value_t *inner, uint64_t *sid)
{
    int type = nanocbor_get_type(inner);
    if (type == NANOCBOR_TYPE_UINT) {
        /* Single sid without args */
        nanocbor_get_uint64(inner, sid);
        /* No arguments to decode */
        nanocbor_decoder_init(&enc->ctx.decoder, NULL, 0);
    }
    else if (type == NANOCBOR_TYPE_ARR) {
        /* Array with one sid and number of args */
        nanocbor_enter_array(inner, &enc->ctx.decoder);
        if (nanocbor_get_uint64(&enc->ctx.decoder, sid) < 0) {
            return -1;
        }
        /* Don't rely on the handler to exhaust the arguments */
        nanocbor_skip(inner);
    }
    else {
        return coreconf_reply_error(&enc->ctx, NULL, CORECONF_OPERATION_FAILED,
                                    CORECONF_MALFORMED_MESSAGE, NULL);
    }
    return 0;
}

static ssize_t _fetch_root(coreconf_encoder_t *enc)
{
    /* Decode the payload and construct the individual bits */
    nanocbor_value_t outer, inner;

    nanocbor_decoder_init(&outer, enc->ctx.memo->request_data, CONFIG_CORECONF_COAP_ARGS_LEN);
    if (nanocbor_enter_array(&outer, &inner) < 0) {
        return coreconf_reply_error(&enc->ctx, NULL, CORECONF_OPERATION_FAILED,
                                    CORECONF_MALFORMED_MESSAGE, NULL);
    }

    size_t num_sids = nanocbor_container_remaining(&inner);

    nanocbor_fmt_array(coreconf_encoder_cbor(enc), num_sids);
    uint64_t sid = 0;

    while (!nanocbor_at_end(&inner)) {
        nanocbor_fmt_map(coreconf_encoder_cbor(enc), 1);
        _decode_single_fetch(enc, &inner, &sid);
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
    encoder->ctx.szx2 = CONFIG_NANOCOAP_BLOCK_SIZE_EXP_MAX,
    encoder->ctx.config = CORECONF_QUERY_CONFIG_ALL;
}

static int _parse_uri_query(coreconf_ctx_t *ctx,
                            const uint8_t *value, ssize_t optlen)
{
    if ((optlen != 3) || (value[1] != '=')) {
        return COAP_CODE_BAD_REQUEST;
    }
    char key = value[0];
    char setting = value[2];
    switch (key) {
    case 'c':
    {
        if ((setting == CORECONF_QUERY_CONFIG_ALL) ||
                (setting == CORECONF_QUERY_CONFIG_ONLY) ||
                (setting == CORECONF_QUERY_CONFIG_NONE)) {
            ctx->config = setting;
            return 0;
        }
        return COAP_CODE_BAD_REQUEST;
    }
    case 'd':
    {
        if ((setting == 'a') ||
            (setting == 't')) {
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
    coap_optpos_t opt;
    bool initopts = true;
    uint8_t *value;
    ssize_t optlen;

    /* Needs the etag, block2, the uri queries and at some point the block1 */
    while ((optlen = coap_opt_get_next(pdu, &opt, &value, initopts)) != -ENOENT) {
        initopts = false;

        if (optlen < 0) {
            return COAP_CODE_BAD_REQUEST;
        }

        switch (opt.opt_num) {
        case COAP_OPT_URI_QUERY:
            {
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

static coreconf_memo_t *_init_coreconf_memo(coap_pkt_t *pdu)
{
    coreconf_memo_t *memo = _coreconf_memo_find_matching(pdu);

    if (!memo) {
        memo = _coreconf_memo_new(pdu);
    }
    return memo;
}

static ssize_t _coreconf_read_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                      coap_request_ctx_t *context)
{
    (void)context;
    coreconf_encoder_t encoder;

    _init_encoder(&encoder);

    coreconf_memo_t *memo = _init_coreconf_memo(pdu);
    if (!memo) {
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }

    encoder.ctx.memo = memo;
    encoder.ctx.pdu_len = len;
    encoder.ctx.pdu = pdu;
    assert(encoder.ctx.memo);

    ssize_t res = _parse_opts(pdu, &encoder.ctx);
    if (res < 0) {
        return res;
    }

    if (encoder.ctx.memo->method == COAP_METHOD_FETCH) {
        /* Copy the payload into the k_param buffer */
        if (pdu->payload_len > CONFIG_CORECONF_COAP_ARGS_LEN) {
            return gcoap_response(pdu, buf, len, COAP_CODE_REQUEST_ENTITY_TOO_LARGE);
        }
        if (encoder.ctx.blocknum2 == 0) {
            memcpy(encoder.ctx.memo->request_data, pdu->payload, pdu->payload_len);
        }
    }

    uint32_t etag = 0;

    /* initialize response packet */
    coap_block_slicer_init(&encoder.slicer,
                           encoder.ctx.blocknum2, coap_szx2size(encoder.ctx.szx2));
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

    coap_opt_add_etag_dummy(pdu, 4);

    coap_opt_add_format(pdu, CORECONF_COAP_FORMAT); /* Yang data + cbor */
    coap_opt_add_block2(pdu, &encoder.slicer, 1);
    coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    coap_nanocbor_slicer_helper_init(&encoder.helper, pdu, &encoder.slicer);
    coap_nanocbor_encoder_init(&encoder.encoder, &encoder.helper);

    void *argv[1] = { 0 };
    if (encoder.ctx.memo->method == COAP_METHOD_GET) {
        res = _read_root(&encoder, NULL, argv);
    }
    else if (encoder.ctx.memo->method == COAP_METHOD_FETCH) {
        res = _fetch_root(&encoder);
    }
    else {
        assert(false);
    }


    if (res != 0) {
        /* coap_reply_error called */
        return res;
    }



    _coreconf_memo_refresh(memo);

    if (encoder.ctx.etag_sent && etag == encoder.ctx.etag) {
        return gcoap_response(pdu, buf, len, COAP_CODE_VALID);
    }
    return coap_nanocbor_block2_finish(pdu, &encoder.helper);
}

static ssize_t _coreconf_write_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                       coap_request_ctx_t *context)
{
    (void)context;
    coreconf_decoder_t decoder;

    _init_decoder(&decoder);

    coreconf_memo_t *memo = _init_coreconf_memo(pdu);
    if (!memo) {
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }

    decoder.ctx.memo = memo;
    decoder.ctx.pdu_len = len;
    decoder.ctx.pdu = pdu;
    assert(memo);
    decoder.ctx.memo->method = coap_get_code_raw(pdu);

    ssize_t res = _parse_opts(pdu, &decoder.ctx);

    if (res < 0) {
        return res;
    }

    coreconf_sid_t sid = 0;

    if (coreconf_ctx_get_method(&decoder.ctx) == COAP_METHOD_DELETE) {
        nanocbor_decoder_init(&decoder.ctx.decoder, NULL, 0);
    }
    else if (pdu->payload_len) {
        /* { [sidnum, args]: {vals } } */
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
    else {
        nanocbor_decoder_init(&decoder.ctx.decoder, NULL, 0);
    }

    /* Validated the outer cbor somewhat, delegate to handler */
    const coreconf_node_t *node = _find_coreconf_node(sid);

    void *argv[1] = { 0 };

    ssize_t wres = 0;

    switch (coreconf_node_type(node)) {
    case CORECONF_NODE_LEAF:
    case CORECONF_NODE_RPC:
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
        _coreconf_memo_refresh(memo);
        /* Todo, lookup and use method */
        return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
    }
    return wres;

cbor_fmt_err:
    _coreconf_memo_refresh(memo);
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

static const char *_coreconf_arg_uri_at(const coreconf_memo_t *memo, size_t pos)
{
    const char *arg = memo->uri_query;

    while (pos--) {
        arg += strlen(arg) + 1;
    }
    return arg;
}

int coreconf_arg_as_uint(const coreconf_ctx_t *ctx, int offset, uint64_t *val)
{
    const coreconf_memo_t *memo = ctx->memo;

    assert(offset >= 0);

    if (memo->method == COAP_METHOD_GET) {
        /* decode from url string */
        const char *arg = _coreconf_arg_uri_at(memo, offset);
        if (!arg) {
            return -1;
        }
        if (!fmt_is_number(arg)) {
            return -1;
        }
        *val = scn_u32_dec(arg, strlen(arg));
        return 0;
    }
    else if (memo->method == COAP_METHOD_FETCH) {
        /* decode as cbor from payload */

    }
    return -1;
}

int coreconf_arg_as_str(const coreconf_ctx_t *ctx, int offset, const char **val)
{
    const coreconf_memo_t *memo = ctx->memo;

    assert(offset >= 0);

    if (memo->method == COAP_METHOD_GET ||
        memo->method == COAP_METHOD_PUT ||
        memo->method == COAP_METHOD_POST ||
        memo->method == COAP_METHOD_DELETE) {
        /* decode from url string */
        *val = _coreconf_arg_uri_at(memo, offset);
        if (!val || (*val)[0] == '\0') {
            return -1;
        }
        return 0;
    }
    else if (memo->method == COAP_METHOD_FETCH ||
             memo->method == COAP_METHOD_IPATCH) {
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

void _fmt_error(nanocbor_encoder_t *enc,
                coreconf_sid_t sid,
                coreconf_error_tag_t error_tag,
                coreconf_app_tag_t app_tag, const char *error_msg)
{
    nanocbor_fmt_map(enc, 1);
    nanocbor_fmt_uint(enc, 1024);

    nanocbor_fmt_map(enc, 4);
    nanocbor_fmt_uint(enc, 1);
    nanocbor_fmt_uint(enc, app_tag);
    nanocbor_fmt_uint(enc, 2);
    nanocbor_fmt_uint(enc, sid);
    nanocbor_fmt_uint(enc, 3);
    if (error_msg) {
        nanocbor_put_tstr(enc, error_msg);
    }
    else {
        nanocbor_put_tstr(enc, "");
    }
    nanocbor_fmt_uint(enc, 4);
    nanocbor_fmt_uint(enc, error_tag);
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

    _fmt_error(&enc, sid, error_tag, app_tag, error_msg);

    size_t encoded_len = nanocbor_encoded_len(&enc);
    return resp_len + MIN(encoded_len, remaining_len);
}
