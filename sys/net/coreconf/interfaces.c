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
#include "net/netif.h"
#include "net/netdev.h"
#include "net/gcoap.h"
#include "xfa.h"
#include "net/netif.h"

static uint64_t _iftype2sid(uint16_t type)
{
    switch (type) {
        case NETDEV_TYPE_ETHERNET:
            return 1888;
        case NETDEV_TYPE_IEEE802154:
            return 1933;
        case NETDEV_TYPE_SLIP:
            return 2036;
        default:
            return 1989;
    }
}

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

    const uint64_t mysid = 1533;

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 4);
    coreconf_fmt_sid(enc, mysid, 1542);
    coreconf_fmt_sid(enc, mysid, 1544);
    coreconf_fmt_sid(enc, mysid, 1561);
    coreconf_fmt_sid(enc, mysid, 1642);
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

            /* Insert name into the k_params */
            char name[CONFIG_NETIF_NAMELENMAX];
            int len = netif_get_name(netif, name);
            if (len < CORECONF_COAP_K_LEN) {
                strncpy(enc->k_param, name, len);
            }
            enc->num_k_args = 1;

            _if_interface_fmt(enc, node, netif);

            *enc->k_param = '\0';
            enc->num_k_args = 0;

            last = netif;
        }
    }
    else {
        netif_t *netif = netif_get_by_name(enc->k_param);
        if (netif) {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 1);
            _if_interface_fmt(enc, node, NULL);
        }
        else {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 0);
        }
    }

    return -1;
}

static int _if_interface_name(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    /* fixme: rework to key */
    netif_t *netif = netif_get_by_name(enc->k_param);
    if (netif) {
        char name[CONFIG_NETIF_NAMELENMAX];
        netif_get_name(netif, name);

        nanocbor_put_tstr(coreconf_encoder_cbor(enc), name);
    }

    return 0;
}

static int _if_interface_phys_addr(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    /* fixme: rework to key */
    uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
    netif_t *netif = netif_get_by_name(enc->k_param);
    if (netif) {
        int res = netif_get_opt(netif, NETOPT_ADDRESS, 0, hwaddr, sizeof(hwaddr));
        if (res >= 0) {
            char hwaddr_str[res * 3];
            gnrc_netif_addr_to_str(hwaddr, res, hwaddr_str);
            nanocbor_put_tstr(coreconf_encoder_cbor(enc), hwaddr_str);
        }
    }

    return 0;
}

static int _if_interface_type(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    /* fixme: rework to key */
    netif_t *netif = netif_get_by_name(enc->k_param);
    if (netif) {
        uint16_t type;
        int res = netif_get_opt(netif, NETOPT_DEVICE_TYPE, 0, &type, sizeof(type));
        if (res >= 0) {
            nanocbor_fmt_uint(coreconf_encoder_cbor(enc), _iftype2sid(type));
        }
    }

    return 0;
}

static int _if_interface_ip6(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 2);
    coreconf_fmt_sid(enc, 1642, 1654);
    coreconf_fmt_sid(enc, 1642, 1656);
    return 0;
}

static int _if_interface_ip6_enabled(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    /* fixme: rework to key */
    nanocbor_fmt_bool(coreconf_encoder_cbor(enc), true);

    return 0;
}

static int _if_interface_ip6_mtu(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    (void)node;
    netif_t *netif = netif_get_by_name(enc->k_param);
    if (netif) {
        uint16_t mtu;
        int res = netif_get_opt(netif, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6, &mtu, sizeof(mtu));
        (void)res;
        nanocbor_fmt_uint(coreconf_encoder_cbor(enc), mtu);
    }
    return 0;
}



CORECONF_NODE(1533, COAP_GET, _if_interface, NULL);
CORECONF_NODE(1542, COAP_GET, _if_interface_name, NULL);
CORECONF_NODE(1544, COAP_GET, _if_interface_phys_addr, NULL);
CORECONF_NODE(1561, COAP_GET, _if_interface_type, NULL);

CORECONF_NODE(1642, COAP_GET, _if_interface_ip6, NULL);
CORECONF_NODE(1654, COAP_GET, _if_interface_ip6_enabled, NULL);
CORECONF_NODE(1656, COAP_GET, _if_interface_ip6_mtu, NULL);
