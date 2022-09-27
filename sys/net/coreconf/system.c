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
#include "periph/pm.h"

ssize_t coreconf_render_sid(coap_pkt_t *pkt, uint8_t *buf, size_t len, uint64_t sid);
void _restart_callback(void *arg);

static ztimer_t _restart_timeout = { .callback=_restart_callback };

void _restart_callback(void *arg)
{
    (void)arg;
    pm_reboot();
}

static ssize_t _system_platform_machine_node(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                         void **argv)
{
    (void)node;
    (void)argv;
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), RIOT_CPU);
    return 0;
}

static ssize_t _system_platform_osname_node(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                        void **argv)
{
    (void)node;
    (void)argv;
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), "RIOT");
    return 0;
}

static ssize_t _system_platform_osrelease_node(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                           void **argv)
{
    (void)node;
    (void)argv;
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

static ssize_t _system_platform_osversion_node(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                           void **argv)
{
    (void)node;
    (void)argv;
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), RIOT_VERSION);
    return 0;
}

static ssize_t _system_restart(coreconf_decoder_t *dec, const coreconf_node_t *node,
        void **argv)
{

    (void)dec;
    (void)node;
    (void)argv;
    /* 500 ms to give gcoap some time to reply with an ack */
    ztimer_set(ZTIMER_MSEC, &_restart_timeout, 500);

    return 0;
}

CORECONF_CONTAINER(root, system_platform, 1724, CORECONF_NODE_STATE);
CORECONF_LEAF(system_platform, 1725, CORECONF_NODE_STATE, _system_platform_machine_node, NULL);
CORECONF_LEAF(system_platform, 1726, CORECONF_NODE_STATE, _system_platform_osname_node, NULL);
CORECONF_LEAF(system_platform, 1727, CORECONF_NODE_STATE, _system_platform_osrelease_node, NULL);
CORECONF_LEAF(system_platform, 1728, CORECONF_NODE_STATE, _system_platform_osversion_node, NULL);
CORECONF_RPC(1718, _system_restart);
