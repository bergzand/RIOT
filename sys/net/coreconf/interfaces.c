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

static netstats_t _stats;

static size_t _num_interfaces(void)
{
    size_t num = 0;
    netif_t *last = NULL;

    while ((last = netif_iter(last)) != NULL) {
        num++;
    }
    return num;
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

static netif_t *_get_netif(coreconf_ctx_t *ctx, void **argv)
{
    netif_t *netif = NULL;

    if (argv[0]) {
        netif = argv[0];
    }
    else {
        const char *name;
        if (coreconf_arg_as_str(ctx, 0, &name) == 0) {
            netif = netif_get_by_name(name);
        }
    }
    return netif;
}

static int _ipv6_addr_find_pfx(const netif_t *netif, const ipv6_addr_t *addr,
                               gnrc_ipv6_nib_pl_t *entry)
{
    void *state = NULL;

    while (gnrc_ipv6_nib_pl_iter(netif_get_id(netif), &state, entry)) {
        if (ipv6_addr_match_prefix(addr, &entry->pfx) >= entry->pfx_len) {
            return 0;
        }
    }
    return -1;
}

static uint8_t _ipv6_addr_get_pfx_len(const netif_t *netif, const ipv6_addr_t *addr)
{
    gnrc_ipv6_nib_pl_t entry;

    if (ipv6_addr_is_link_local(addr)) {
        return 64;
    }

    if (_ipv6_addr_find_pfx(netif, addr, &entry) < 0) {
        return 0;
    }

    return entry.pfx_len;
}

static void _fmt_interface_ip6_addr_ip(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                       ipv6_addr_t *addr)
{
    (void)node;
    char addr_str[IPV6_ADDR_MAX_STR_LEN];

    ipv6_addr_to_str(addr_str, addr, sizeof(addr_str));
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), addr_str);
}

static void _fmt_interface_ip6_addr_pfx(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                        const netif_t *netif, const ipv6_addr_t *addr)
{
    (void)node;
    uint8_t prefix_len = _ipv6_addr_get_pfx_len(netif, addr);

    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), prefix_len);
}

static ssize_t _interface_ip6_read(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                   void **argv)
{
    netif_t *netif = _get_netif(&enc->ctx, argv);

    if (!netif) {
        return coreconf_reply_error(&enc->ctx, node, CORECONF_MISSING_ELEMENT, CORECONF_MISSING_KEY,
                                    "Missing interface ID");
    }
    ipv6_addr_t addr;

    if (argv[1]) {
        memcpy(&addr, argv[1], sizeof(ipv6_addr_t));
    }
    else {
        const char *addr_str;
        if (coreconf_arg_as_str(&enc->ctx, 1, &addr_str) == 0) {
            if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
                return coreconf_reply_error(&enc->ctx, node, CORECONF_INVALID_VALUE,
                                            CORECONF_INVALID_DATA_TYPE, "Invalid IPv6 address");
            }
        }
        else {
            return coreconf_reply_error(&enc->ctx, node, CORECONF_MISSING_ELEMENT,
                                        CORECONF_MISSING_KEY, "Missing IPv6 address");
        }
    }

    switch (coreconf_node_sid(node)) {
    case 1644:
        _fmt_interface_ip6_addr_ip(enc, node, &addr);
        break;
    case 1646:
        _fmt_interface_ip6_addr_pfx(enc, node, netif, &addr);
        break;
    }
    return 0;
}

static ssize_t _if_interface_ip6_addr_write(coreconf_decoder_t *dec, const coreconf_node_t *node,
                                            void **argv)
{
    netif_t *netif = _get_netif(&dec->ctx, argv);

    if (!netif) {
        return coreconf_reply_error(&dec->ctx, node, CORECONF_MISSING_ELEMENT, CORECONF_MISSING_KEY,
                                    "Missing interface ID");
    }

    if (coreconf_ctx_get_method(&dec->ctx) == COAP_METHOD_DELETE) {

        ipv6_addr_t addr;
        const char *ip6_str;
        if (coreconf_arg_as_str(&dec->ctx, 1, &ip6_str) == 0) {
            if (ipv6_addr_from_str(&addr, ip6_str) == NULL) {
                /* conversion failed */
                return coreconf_reply_error(&dec->ctx, node, CORECONF_INVALID_VALUE,
                                            CORECONF_INVALID_DATA_TYPE, "Invalid IPv6 address");
            }
        }
        ipv6_addr_t prefix;
        uint8_t pfx_len = _ipv6_addr_get_pfx_len(netif, &addr);

        ipv6_addr_init_prefix(&prefix, &addr, pfx_len);

        if (netif_set_opt(netif, NETOPT_IPV6_ADDR_REMOVE, 0, &addr,
                          sizeof(addr)) < 0) {
            return coreconf_reply_error(&dec->ctx, node, CORECONF_INVALID_VALUE,
                                        CORECONF_INSTANCE_REQUIRED, "Unable to delete");
        }

        gnrc_ipv6_nib_pl_del(netif_get_id(netif), &prefix, pfx_len);

        return 0;
    }


    /* expect a payload as {2: '2001:db8::1', 4: 64} */
    char addr_str[IPV6_ADDR_MAX_STR_LEN] = { 0 };
    uint8_t prefix_len;

    nanocbor_value_t payload_arr;

    if (nanocbor_enter_array(&dec->ctx.decoder, &payload_arr) != NANOCBOR_OK) {
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
        case 2:     /* address */
        {
            const uint8_t *addr_ref;
            size_t addr_len = 0;
            if ((nanocbor_get_tstr(&payload_map, &addr_ref, &addr_len) != NANOCBOR_OK) ||
                (addr_len >= IPV6_ADDR_MAX_STR_LEN)) {
                goto cbor_fmt_err;
            }
            memcpy(addr_str, addr_ref, addr_len);
        }
        break;
        case 4:
            if ((nanocbor_get_uint8(&payload_map, &prefix_len) < NANOCBOR_OK) ||
                (prefix_len > 128)) {
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
        return coreconf_reply_error(&dec->ctx, node, CORECONF_OPERATION_FAILED, CORECONF_DUPLICATE,
                                    "IPv6 address already exists");
    }
    return 0;

cbor_fmt_err:
    /* non specific error */
    return coreconf_reply_error(&dec->ctx, node, CORECONF_ERROR, 0, NULL);
}

static ssize_t _if_interface_ip6_mtu(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                     void **argv)
{
    (void)node;
    netif_t *netif = _get_netif(&enc->ctx, argv);
    uint16_t mtu;
    int res = netif_get_opt(netif, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6, &mtu, sizeof(mtu));

    (void)res;
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), mtu);
    return 0;
}

static ssize_t _if_interface_ip6_enabled(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                         void **argv)
{
    (void)node;
    (void)argv;
    nanocbor_fmt_bool(coreconf_encoder_cbor(enc), true);
    return 0;
}

static int _parse_ip6(coreconf_ctx_t *ctx, const coreconf_node_t *node, void **argv)
{
    coreconf_encoder_t *enc = container_of(ctx, coreconf_encoder_t, ctx);
    netif_t *netif = _get_netif(ctx, argv);

    if (!netif) {
        return coreconf_reply_error(&enc->ctx, node, CORECONF_MISSING_ELEMENT, CORECONF_MISSING_KEY,
                                    "Missing interface ID");
    }
    const char *ip6_str = NULL;
    const ipv6_addr_t *match_addr = NULL;
    ipv6_addr_t tmp_addr;

    if (coreconf_arg_as_str(ctx, 1, &ip6_str) == 0) {
        match_addr = ipv6_addr_from_str(&tmp_addr, ip6_str);
        if (!match_addr) {
            /* conversion failed */
            return coreconf_reply_error(&enc->ctx, node, CORECONF_INVALID_VALUE,
                                        CORECONF_INVALID_DATA_TYPE, "Invalid IPv6 address");
        }
    }
    ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
    int res = netif_get_opt(netif, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                            sizeof(ipv6_addrs));

    if (res >= 0) {
        size_t num_addresses = res / sizeof(ipv6_addr_t);

        if (match_addr) {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 1);
        }
        else {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), num_addresses);
        }

        for (unsigned i = 0; i < num_addresses; i++) {
            if (match_addr && !ipv6_addr_equal(match_addr, &ipv6_addrs[i])) {
                continue;
            }
            argv[1] = &ipv6_addrs[i];
            coreconf_read_container(enc, node, argv);
        }
        argv[1] = NULL;
        return 0;
    }
    return CORECONF_ERR_NOT_FOUND;
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

static ssize_t _if_interface_stats(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                   void **argv)
{
    (void)node;
    netif_t *netif = _get_netif(&enc->ctx, argv);

    if (!netif) {
        return coreconf_reply_error(&enc->ctx, node, CORECONF_MISSING_ELEMENT, CORECONF_MISSING_KEY,
                                    "Missing interface ID");
    }
    if (coreconf_first_request_part(&enc->ctx)) {
        int res = netif_get_opt(netif, NETOPT_STATS, NETSTATS_LAYER2, &_stats,
                                sizeof(_stats));
        if (res < 0) {
            return CORECONF_ERR_INTERNAL_SERVER;
        }
    }

    switch (coreconf_node_sid(node)) {
    case 1552:
        _fmt_if_interface_stats_in_octets(enc, &_stats);
        break;
    case 1557:
        _fmt_if_interface_stats_out_error_pkts(enc, &_stats);
        break;
    case 1558:
        _fmt_if_interface_stats_out_mcast_pkts(enc, &_stats);
        break;
    case 1559:
        _fmt_if_interface_stats_out_octets(enc, &_stats);
        break;
    case 1560:
        _fmt_if_interface_stats_out_unicast_pkts(enc, &_stats);
        break;
    }
    return 0;
}

static int _fmt_if_interface_enabled_read(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                          netif_t *netif)
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
    return CORECONF_ERR_INTERNAL_SERVER;
}

static int _fmt_if_interface_phys_addr(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                       netif_t *netif)
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

static int _fmt_if_interface_type(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                  netif_t *netif)
{
    (void)node;
    uint16_t type;
    int res = netif_get_opt(netif, NETOPT_DEVICE_TYPE, 0, &type, sizeof(type));

    if (res >= 0) {
        nanocbor_fmt_uint(coreconf_encoder_cbor(enc), _iftype2sid(type));
    }
    return 0;
}

static int _fmt_if_interface_name(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                  netif_t *netif)
{
    (void)node;
    char name[CONFIG_NETIF_NAMELENMAX];

    netif_get_name(netif, name);

    nanocbor_put_tstr(coreconf_encoder_cbor(enc), name);
    return 0;
}

/* single property from a network interface */
static ssize_t _if_interface_prop_read(coreconf_encoder_t *enc, const coreconf_node_t *node,
                                       void **argv)
{
    netif_t *netif = _get_netif(&enc->ctx, argv);

    if (netif) {
        switch (coreconf_node_sid(node)) {
        case 1542:
            return _fmt_if_interface_name(enc, node, netif);
        case 1543:
            return _fmt_if_interface_enabled_read(enc, node, netif);
        case 1544:
            return _fmt_if_interface_phys_addr(enc, node, netif);
        case 1561:
            return _fmt_if_interface_type(enc, node, netif);
        }
    }
    return CORECONF_ERR_NOT_FOUND;
}

static ssize_t _parse_iface(coreconf_ctx_t *ctx, const coreconf_node_t *node, void **argv)
{
    coreconf_encoder_t *enc = container_of(ctx, coreconf_encoder_t, ctx);

    if (coreconf_enc_args_empty(enc)) {
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

            argv[0] = netif;

            coreconf_read_container(enc, node, argv);

            last = netif;
        }
    }
    else {
        netif_t *netif = netif_get_by_name(enc->ctx.state->uri_query);
        if (netif) {
            argv[0] = netif;
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 1);
            return coreconf_read_container(enc, node, argv);
        }
        else {
            nanocbor_fmt_array(coreconf_encoder_cbor(enc), 0);
        }
    }

    return 0;
}


CORECONF_CONTAINER(root, interfaces_interfaces, 1505, CORECONF_NODE_CONFIG);
CORECONF_CONTAINER_LIST(interfaces_interfaces,
                        interfaces_interfaces_interface, 1533, CORECONF_NODE_CONFIG,
                        _parse_iface, NULL, NULL);
CORECONF_LEAF(interfaces_interfaces_interface, 1542, CORECONF_NODE_CONFIG, _if_interface_prop_read,
              NULL);
CORECONF_LEAF(interfaces_interfaces_interface, 1543, CORECONF_NODE_CONFIG, _if_interface_prop_read,
              NULL);
CORECONF_LEAF(interfaces_interfaces_interface, 1544, CORECONF_NODE_CONFIG, _if_interface_prop_read,
              NULL);
CORECONF_LEAF(interfaces_interfaces_interface, 1561, CORECONF_NODE_CONFIG, _if_interface_prop_read,
              NULL);

CORECONF_CONTAINER(interfaces_interfaces_interface,
                   interfaces_interfaces_interface_statistics,
                   1546, CORECONF_NODE_STATE);
CORECONF_LEAF(interfaces_interfaces_interface_statistics,
              1552, CORECONF_NODE_CONFIG, _if_interface_stats, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_statistics,
              1557, CORECONF_NODE_CONFIG, _if_interface_stats, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_statistics,
              1558, CORECONF_NODE_CONFIG, _if_interface_stats, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_statistics,
              1559, CORECONF_NODE_CONFIG, _if_interface_stats, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_statistics,
              1560, CORECONF_NODE_CONFIG, _if_interface_stats, NULL);

CORECONF_CONTAINER(interfaces_interfaces_interface,
                   interfaces_interfaces_interface_ipv6,
                   1642, CORECONF_NODE_CONFIG);

CORECONF_CONTAINER_LIST(interfaces_interfaces_interface_ipv6,
                        interfaces_interfaces_interface_ipv6_address,
                        1643, CORECONF_NODE_CONFIG, _parse_ip6, NULL, _if_interface_ip6_addr_write);

CORECONF_LEAF(interfaces_interfaces_interface_ipv6_address, 1644, CORECONF_NODE_CONFIG,
              _interface_ip6_read, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_ipv6_address, 1646, CORECONF_NODE_CONFIG,
              _interface_ip6_read, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_ipv6, 1654, CORECONF_NODE_CONFIG,
              _if_interface_ip6_enabled, NULL);
CORECONF_LEAF(interfaces_interfaces_interface_ipv6, 1656, CORECONF_NODE_CONFIG,
              _if_interface_ip6_mtu, NULL);
