/*
 * Copyright (C) 2019 Kaspar Schleiser <kaspar@schleiser.de>
 *               2019 Inria
 *               2019 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_suit
 * @defgroup    sys_suit_transport_coap SUIT firmware CoAP transport
 * @brief       SUIT secure firmware updates over CoAP
 *
 * @{
 *
 * @brief       SUIT CoAP helper API
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 */

#ifndef SUIT_TRANSPORT_COAP_H
#define SUIT_TRANSPORT_COAP_H

#include "net/nanocoap.h"
#include "net/nanocoap_sock.h"
#include "suit.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SUIT_TRANSPORT_COAP_BLOCKSIZE
#define SUIT_TRANSPORT_COAP_BLOCKSIZE   COAP_BLOCKSIZE_64
#endif /* SUIT_TRANSPORT_COAP_BLOCKSIZE */

/**
 * @brief    Start SUIT CoAP thread
 */
void suit_coap_run(void);

extern const coap_resource_subtree_t coap_resource_subtree_suit;
/**
 * @brief SUIT CoAP endpoint entry.
 *
 * In order to use, include this header, then add SUIT_COAP_SUBTREE to the nanocoap endpoint array.
 * Mind the alphanumerical sorting!
 *
 * See examples/suit_update for an example.
 */
#define SUIT_COAP_SUBTREE \
    { \
        .path="/suit/", \
        .methods=COAP_MATCH_SUBTREE | COAP_METHOD_GET | COAP_METHOD_POST | COAP_METHOD_PUT, \
        .handler=coap_subtree_handler, \
        .context=(void*)&coap_resource_subtree_suit \
    }

/**
 * @brief   Trigger a SUIT udate
 *
 * @param[in] url       url pointer containing the full coap url to the manifest
 * @param[in] len       length of the url
 */
void suit_coap_trigger(const uint8_t *url, size_t len);

int suit_coap_get_blockwise_payload(suit_manifest_t *manifest, const char *url);
#ifdef __cplusplus
}
#endif

#endif /* SUIT_TRANSPORT_COAP_H */
/** @} */
