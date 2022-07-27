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
 * @brief       CORECONF IETF-system module implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#include "net/coreconf.h"
#include "net/gcoap.h"
#include "xfa.h"
#include "fmt.h"

ssize_t coreconf_render_sid(coap_pkt_t *pkt, uint8_t *buf, size_t len, uint64_t sid);

static int _system_platform_node(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)enc;
    (void)node;

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 4);


    coreconf_fmt_sid(enc, 1724, 1725);
    coreconf_fmt_sid(enc, 1724, 1726);
    coreconf_fmt_sid(enc, 1724, 1727);
    coreconf_fmt_sid(enc, 1724, 1728);

    return 0;
}

static int _system_platform_machine_node(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), RIOT_CPU);
    return 0;
}

static int _system_platform_osname_node(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), "RIOT");
    return 0;
}

static int _system_platform_osrelease_node(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    uint16_t major = (RIOT_VERSION_CODE >> 48) & 0xFFFF;
    uint16_t minor = (RIOT_VERSION_CODE >> 32) & 0xFFFF;
    uint16_t patch = (RIOT_VERSION_CODE >> 16) & 0xFFFF;
    uint16_t extra = RIOT_VERSION_CODE & 0xFFFF;

    char version_string[32] = { 0 };
    size_t offset = 0;

    offset += fmt_u16_dec(&version_string[offset], major);
    version_string[offset++] = '.';
    offset += fmt_u16_dec(&version_string[offset], minor);
    version_string[offset++] = '.';
    offset += fmt_u16_dec(&version_string[offset], patch);

    if (extra) {
        version_string[offset++] = '+';
        offset += fmt_u16_dec(&version_string[offset], extra);
    }

    nanocbor_put_tstr(coreconf_encoder_cbor(enc), version_string);
    return 0;
}

static int _system_platform_osversion_node(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), RIOT_VERSION);
    return 0;
}

CORECONF_NODE(1724, COAP_GET, _system_platform_node);
CORECONF_NODE(1725, COAP_GET, _system_platform_machine_node);
CORECONF_NODE(1726, COAP_GET, _system_platform_osname_node);
CORECONF_NODE(1727, COAP_GET, _system_platform_osrelease_node);
CORECONF_NODE(1728, COAP_GET, _system_platform_osversion_node);
