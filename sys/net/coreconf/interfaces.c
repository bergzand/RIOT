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
 * @brief       CORECONF IETF-interfaces module implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#include "net/coreconf.h"
#include "net/gcoap.h"
#include "xfa.h"
#include "net/netif.h"

static size_t _num_interfaces(void)
{
    size_t num = 0;
    netif_t *last = NULL;
    while ((last = netif_iter(last)) != NULL) {
        num++;
    }
    return num;
}

static int _if_interface_fmt(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    (void)netif;
    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 1);
    coreconf_fmt_sid(enc, 1533, 1542);
    return 0;
}

static int _if_interface(coreconf_encoder_t *enc, const coreconf_node_t *node)
{

    if (coreconf_k_param_empty(enc)) {
        size_t num_interfaces = _num_interfaces();
        nanocbor_fmt_array(coreconf_encoder_cbor(enc), num_interfaces);

        netif_t *last = NULL;
        while (last != netif_iter(NULL)) {
            netif_t *netif = NULL;
            netif_t *next = netif_iter(netif);
            /* Step until next is end of list or was previously listed. */
            do {
                netif = next;
                next = netif_iter(netif);
            } while (next && next != last);
            _if_interface_fmt(enc, node, netif);
            last = netif;
        }
    }

    return 0;
}

static int _if_interface_name(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    netif_t *iface = netif_get_by_name();
    /* fixme: rework to key */
    netif_t *netif = netif_iter(NULL);

    char name[CONFIG_NETIF_NAMELENMAX];
    netif_get_name(netif, name);

    nanocbor_put_tstr(coreconf_encoder_cbor(enc), name);

    return 0;
}

CORECONF_NODE(1533, COAP_GET, _if_interface);
CORECONF_NODE(1542, COAP_GET, _if_interface_name);
