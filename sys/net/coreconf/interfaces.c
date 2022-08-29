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
#include "net/ipv6/addr.h"
#include "net/netif.h"
#include "net/netstats.h"
#include "net/gnrc/ipv6/nib.h"

static int _fmt_if_interface(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif);

static uint8_t _ipv6_addr_get_pfx_len(const netif_t *netif, const ipv6_addr_t *addr)
{
    gnrc_ipv6_nib_pl_t entry;
    void *state = NULL;

    if (ipv6_addr_is_link_local(addr)) {
        return 64;
    }

    while (gnrc_ipv6_nib_pl_iter(netif_get_id(netif), &state, &entry)) {
        if (ipv6_addr_match_prefix(addr, &entry.pfx) >= entry.pfx_len) {
            return entry.pfx_len;
        }
    }
    return 0;
}

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

static bool _netopt_state2enabled(netopt_state_t state)
{
    if ((state == NETOPT_STATE_TX) ||
        (state == NETOPT_STATE_RX) ||
        (state == NETOPT_STATE_IDLE)) {
        return true;
    }
    return false;
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

            _fmt_if_interface(enc, node, netif);

            *enc->k_param = '\0';
            enc->num_k_args = 0;

            last = netif;
        }
    }
    else {
        netif_t *netif = netif_get_by_name(enc->k_param);
        if (netif) {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 1);
            _fmt_if_interface(enc, node, netif);
        }
        else {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 0);
        }
    }

    return 0;
}

static void _fmt_interface_ip6_addr_ip(coreconf_encoder_t *enc, const coreconf_node_t *node, ipv6_addr_t *addr)
{
    (void)node;
    char addr_str[IPV6_ADDR_MAX_STR_LEN];

    ipv6_addr_to_str(addr_str, addr, sizeof(addr_str));
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), addr_str);
}

static void _fmt_interface_ip6_addr_pfx(coreconf_encoder_t *enc, const coreconf_node_t *node, const netif_t *netif, const ipv6_addr_t *addr)
{
    (void)node;
    uint8_t prefix_len = _ipv6_addr_get_pfx_len(netif, addr);
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), prefix_len);
}

static int _fmt_if_interface_ip6_addr(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    const uint64_t mysid = 1642;
    ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
    int res = netif_get_opt(netif, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                          sizeof(ipv6_addrs));
    if (res >= 0) {
        size_t num_addresses = res / sizeof(ipv6_addr_t);

        nanocbor_fmt_array(coreconf_encoder_cbor(enc), num_addresses);

        for (unsigned i = 0; i < num_addresses; i++) {
            nanocbor_fmt_map(coreconf_encoder_cbor(enc), 2);
            coreconf_cbor_sid(enc, mysid, 1644);
            _fmt_interface_ip6_addr_ip(enc, node, &ipv6_addrs[i]);
            coreconf_cbor_sid(enc, mysid, 1646);
            _fmt_interface_ip6_addr_pfx(enc, node, netif, &ipv6_addrs[i]);
        }
        return 0;
    }
    return -1;
}

static int _fmt_if_interface_name(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    char name[CONFIG_NETIF_NAMELENMAX];
    netif_get_name(netif, name);

    nanocbor_put_tstr(coreconf_encoder_cbor(enc), name);
    return 0;
}

static int _fmt_if_interface_enabled_read(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    netopt_state_t state = NETOPT_STATE_OFF;
    int res = netif_get_opt(netif, NETOPT_STATE, 0, &state, sizeof(state));
    if (res >= 0) {
        nanocbor_fmt_bool(coreconf_encoder_cbor(enc),
                _netopt_state2enabled(state));
        return 0;
    }
    if (res == -ENOTSUP) {
        /* interface is always enabled */
        nanocbor_fmt_bool(coreconf_encoder_cbor(enc),
                true);
        return 0;
    }
    return -1;
}

static int _fmt_if_interface_phys_addr(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
    int res = netif_get_opt(netif, NETOPT_ADDRESS, 0, hwaddr, sizeof(hwaddr));
    if (res >= 0) {
        char hwaddr_str[res * 3];
        gnrc_netif_addr_to_str(hwaddr, res, hwaddr_str);
        nanocbor_put_tstr(coreconf_encoder_cbor(enc), hwaddr_str);
    }
    return 0;
}

static int _fmt_if_interface_type(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    uint16_t type;
    int res = netif_get_opt(netif, NETOPT_DEVICE_TYPE, 0, &type, sizeof(type));
    if (res >= 0) {
        nanocbor_fmt_uint(coreconf_encoder_cbor(enc), _iftype2sid(type));
    }
    return 0;
}

static void _fmt_if_interface_stats_in_octets(coreconf_encoder_t *enc,
        const netstats_t *stats)
{
    /* Check: spec says to include framing chars */
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), stats->rx_bytes);
}

static void _fmt_if_interface_stats_out_octets(coreconf_encoder_t *enc,
        const netstats_t *stats)
{
    /* Check: spec says to include framing chars */
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), stats->tx_bytes);
}

static void _fmt_if_interface_stats_out_mcast_pkts(coreconf_encoder_t *enc,
        const netstats_t *stats)
{
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), stats->tx_mcast_count);
}

static void _fmt_if_interface_stats_out_unicast_pkts(coreconf_encoder_t *enc,
        const netstats_t *stats)
{
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), stats->tx_unicast_count);
}

static void _fmt_if_interface_stats_out_error_pkts(coreconf_encoder_t *enc,
        const netstats_t *stats)
{
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), stats->tx_failed);
}

static int _fmt_if_interface_stats(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    netstats_t stats;
    int res = netif_get_opt(netif, NETOPT_STATS, NETSTATS_LAYER2, &stats,
                            sizeof(stats));
    if (res < 0) {
        return -1;
    }

    const int64_t mysid = 1517;

    nanocbor_encoder_t *nc = coreconf_encoder_cbor(enc);
    nanocbor_fmt_map(nc, 5);

    coreconf_cbor_sid(enc, mysid, 1552);
    _fmt_if_interface_stats_in_octets(enc, &stats);

    coreconf_cbor_sid(enc, mysid, 1557);
    _fmt_if_interface_stats_out_error_pkts(enc, &stats);

    coreconf_cbor_sid(enc, mysid, 1558);
    _fmt_if_interface_stats_out_mcast_pkts(enc, &stats);

    coreconf_cbor_sid(enc, mysid, 1559);
    _fmt_if_interface_stats_out_octets(enc, &stats);

    coreconf_cbor_sid(enc, mysid, 1560);
    _fmt_if_interface_stats_out_unicast_pkts(enc, &stats);

    return 0;
}

static int _fmt_if_interface(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    (void)netif;

    const uint64_t mysid = 1533;

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 6);

    coreconf_cbor_sid(enc, mysid, 1542);
    _fmt_if_interface_name(enc, node, netif);

    coreconf_cbor_sid(enc, mysid, 1543);
    _fmt_if_interface_enabled_read(enc, node, netif);

    coreconf_cbor_sid(enc, mysid, 1544);
    _fmt_if_interface_phys_addr(enc, node, netif);

    coreconf_cbor_sid(enc, mysid, 1546);
    _fmt_if_interface_stats(enc, node, netif);

    coreconf_cbor_sid(enc, mysid, 1561);
    _fmt_if_interface_type(enc, node, netif);

    coreconf_fmt_sid(enc, mysid, 1642);
    return 0;
}

static int _fmt_if_interface_ip6_mtu(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    uint16_t mtu;
    int res = netif_get_opt(netif, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6, &mtu, sizeof(mtu));
    (void)res;
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), mtu);
    return 0;
}

static int _fmt_if_interface_ip6_enabled(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    (void)netif;
    /* fixme: rework to key */
    nanocbor_fmt_bool(coreconf_encoder_cbor(enc), true);
    return 0;
}

static int _fmt_if_interface_ip6(coreconf_encoder_t *enc, const coreconf_node_t *node, netif_t *netif)
{
    (void)node;
    (void)netif;
    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 3);

    coreconf_cbor_sid(enc, 1642, 1643);
    _fmt_if_interface_ip6_addr(enc, node, netif);

    coreconf_fmt_sid(enc, 1642, 1654);
    coreconf_fmt_sid(enc, 1642, 1656);

    return 0;
}

static int _if_interface_ip6_enabled(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    _fmt_if_interface_ip6_enabled(enc, node, NULL);
    return 0;
}

static int _if_interface_ip6_mtu(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    netif_t *netif = netif_get_by_name(enc->k_param);
    if (netif) {
        _fmt_if_interface_ip6_mtu(enc, node, netif);
        return 0;
    }
    return -1;
}

/* single property from a network interface */
static int _if_interface_prop_read(coreconf_encoder_t *enc, const coreconf_node_t *node)
{
    netif_t *netif = netif_get_by_name(enc->k_param);
    if (netif) {
        switch (node->num) {
            case 1542:
                return _fmt_if_interface_name(enc, node, netif);
            case 1543:
                return _fmt_if_interface_enabled_read(enc, node, netif);
            case 1544:
                return _fmt_if_interface_phys_addr(enc, node, netif);
            case 1546:
                return _fmt_if_interface_stats(enc, node, netif);
            case 1561:
                return _fmt_if_interface_type(enc, node, netif);
            case 1642:
                return _fmt_if_interface_ip6(enc, node, netif);
            case 1643:
                return _fmt_if_interface_ip6_addr(enc, node, netif);
        }
    }
    return -1;
}

static int _if_interface_ip6_addr_write(coreconf_decoder_t *dec, const coreconf_node_t *node)
{
    (void)node;
    netif_t *netif = netif_get_by_name(dec->k_param);
    if (!netif) {
        return -1;
    }

    /* expect a payload as {2: '2001:db8::1', 4: 64} */
    char addr_str[IPV6_ADDR_MAX_STR_LEN] = { 0 };
    uint8_t prefix_len;

    nanocbor_value_t payload_arr;
    if (nanocbor_enter_array(&dec->decoder, &payload_arr) != NANOCBOR_OK) {
        goto cbor_fmt_err;
    }

    nanocbor_value_t payload_map;
    if (nanocbor_enter_map(&payload_arr, &payload_map) != NANOCBOR_OK) {
        goto cbor_fmt_err;
    }

    while (!nanocbor_at_end(&payload_map)) {
        uint64_t key = 0;
        if (nanocbor_get_uint64(&payload_map, &key) < NANOCBOR_OK) {
            goto cbor_fmt_err;
        }

        switch (key) {
            case 2: /* address */
                {
                    const uint8_t *addr_ref;
                    size_t addr_len = 0;
                    if (nanocbor_get_tstr(&payload_map, &addr_ref, &addr_len) != NANOCBOR_OK) {
                        goto cbor_fmt_err;
                    }
                    memcpy(addr_str, addr_ref, addr_len);
                }
                break;
            case 4:
                if (nanocbor_get_uint8(&payload_map, &prefix_len) < NANOCBOR_OK) {
                    goto cbor_fmt_err;
                }
                break;
                /* prefix */
            default:
                goto cbor_fmt_err;
        }
    }

    ipv6_addr_t addr;
    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID | (prefix_len << 8U);

    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        goto cbor_fmt_err;
    }

    if (netif_set_opt(netif, NETOPT_IPV6_ADDR, flags, &addr,
                      sizeof(addr)) < 0) {
        return COAP_CODE_INTERNAL_SERVER_ERROR;
    }
    return COAP_CODE_CREATED;

cbor_fmt_err:
    return COAP_CODE_BAD_REQUEST;
}

CORECONF_NODE(1533, COAP_GET, _if_interface, NULL);
CORECONF_NODE(1542, COAP_GET, _if_interface_prop_read, NULL);
CORECONF_NODE(1543, COAP_GET, _if_interface_prop_read, NULL);
CORECONF_NODE(1544, COAP_GET, _if_interface_prop_read, NULL);
CORECONF_NODE(1546, COAP_GET, _if_interface_prop_read, NULL);
CORECONF_NODE(1561, COAP_GET, _if_interface_prop_read, NULL);

CORECONF_NODE(1642, COAP_GET, _if_interface_prop_read, NULL);
CORECONF_NODE(1643, COAP_GET, _if_interface_prop_read, _if_interface_ip6_addr_write);
CORECONF_NODE(1654, COAP_GET, _if_interface_ip6_enabled, NULL);
CORECONF_NODE(1656, COAP_GET, _if_interface_ip6_mtu, NULL);
