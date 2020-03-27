/*
 * Copyright (C) 2019 Freie Universität Berlin
 *               2019 Inria
 *               2019 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_suit
 * @{
 *
 * @file
 * @brief       SUIT coap
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @}
 */

#include <inttypes.h>
#include <string.h>

#include "msg.h"
#include "log.h"
#include "net/nanocoap.h"
#include "net/nanocoap_sock.h"
#include "thread.h"
#include "periph/pm.h"

#include "suit/transport/coap.h"
#include "net/sock/util.h"

#ifdef MODULE_RIOTBOOT_SLOT
#include "riotboot/slot.h"
#include "riotboot/flashwrite.h"
#endif

#ifdef MODULE_SUIT
#include "suit.h"
#endif

#if defined(MODULE_PROGRESS_BAR)
#include "progress_bar.h"
#endif

#define ENABLE_DEBUG (0)
#include "debug.h"

#ifndef SUIT_COAP_STACKSIZE
/* allocate stack needed to keep a page buffer and do manifest validation */
#define SUIT_COAP_STACKSIZE (3 * THREAD_STACKSIZE_LARGE + FLASHPAGE_SIZE)
#endif

#ifndef SUIT_COAP_PRIO
#define SUIT_COAP_PRIO THREAD_PRIORITY_MAIN - 1
#endif

#ifndef SUIT_URL_MAX
#define SUIT_URL_MAX            128
#endif

#ifndef SUIT_MANIFEST_BUFSIZE
#define SUIT_MANIFEST_BUFSIZE   640
#endif

#define SUIT_MSG_TRIGGER        0x12345

static char _stack[SUIT_COAP_STACKSIZE];
static char _url[SUIT_URL_MAX];
static uint8_t _manifest_buf[SUIT_MANIFEST_BUFSIZE];
static kernel_pid_t _suit_coap_pid;

static int _flashwrite_helper(void *arg, size_t offset, uint8_t *buf, size_t len,
                           int more);
static int _get_blockwise_url(const char *url,
                              coap_blockwise_cb_t cb, void *arg);

#ifdef MODULE_SUIT
static inline void _print_download_progress(size_t offset, size_t len,
                                            uint32_t image_size)
{
    (void)offset;
    (void)len;
    (void)image_size;
    DEBUG("_suit_flashwrite(): writing %u bytes at pos %u\n", len, offset);
#if defined(MODULE_PROGRESS_BAR)
    if (image_size != 0) {
        char _suffix[7] = { 0 };
        uint8_t _progress = 100 * (offset + len) / image_size;
        sprintf(_suffix, " %3d%%", _progress);
        progress_bar_print("Fetching firmware ", _suffix, _progress);
        if (_progress == 100) {
            puts("");
        }
    }
#endif
}
#endif

static int _handle_block(coap_pkt_t *pkt, void *arg)
{

    coap_block1_t block;
    if (coap_get_block2(pkt, &block) != 1) {
        return -1;
    }
    return _flashwrite_helper(arg, block.offset, pkt->payload, pkt->payload_len, block.more);
}

int suit_coap_get_blockwise_payload(suit_manifest_t *manifest, const char *url)
{
    return _get_blockwise_url(url, _handle_block, manifest);
}

int _get_blockwise_url(const char *url,
                       coap_blockwise_cb_t cb, void *arg)
{
    /* mmmmh dynamically sized array */
    static const size_t buflen = 64 + (0x1 << (SUIT_TRANSPORT_COAP_BLOCKSIZE + 4));
    uint8_t buf[buflen];

    char hostport[CONFIG_SOCK_HOSTPORT_MAXLEN];
    char urlpath[CONFIG_SOCK_URLPATH_MAXLEN];
    sock_udp_ep_t remote;

    if (strncmp(url, "coap://", 7)) {
        LOG_INFO("suit: URL doesn't start with \"coap://\"\n");
        return -EINVAL;
    }

    if (sock_urlsplit(url, hostport, urlpath) < 0) {
        LOG_INFO("suit: invalid URL\n");
        return -EINVAL;
    }

    if (sock_udp_str2ep(&remote, hostport) < 0) {
        LOG_INFO("suit: invalid URL\n");
        return -EINVAL;
    }

    if (!remote.port) {
        remote.port = COAP_PORT;
    }

    return nanocoap_get_blockwise(&remote, urlpath, buf, buflen,
                                  SUIT_TRANSPORT_COAP_BLOCKSIZE,
                                  cb, arg);
}

typedef struct {
    size_t offset;
    uint8_t *ptr;
    size_t len;
} _buf_t;

static int _2buf(coap_pkt_t *pkt, void *arg)
{
    coap_block1_t block;
    if (coap_get_block2(pkt, &block) == 0) {
        return -1;
    }

    size_t len = pkt->payload_len;

    _buf_t *_buf = arg;
    if (_buf->offset != block.offset) {
        return 0;
    }
    if (len > _buf->len) {
        return -1;
    }
    else {
        memcpy(_buf->ptr, pkt->payload, len);
        _buf->offset += len;
        _buf->ptr += len;
        _buf->len -= len;
        return 0;
    }
}

ssize_t suit_coap_get_blockwise_url_buf(const char *url,
                                        uint8_t *buf, size_t len)
{
    _buf_t _buf = { .ptr = buf, .len = len };
    int res = _get_blockwise_url(url, _2buf, &_buf);

    return (res < 0) ? (ssize_t)res : (ssize_t)_buf.offset;
}

static void _suit_handle_url(const char *url)
{
    LOG_INFO("suit_coap: downloading \"%s\"\n", url);
    ssize_t size = suit_coap_get_blockwise_url_buf(url,
                                                   _manifest_buf,
                                                   SUIT_MANIFEST_BUFSIZE);
    if (size >= 0) {
        LOG_INFO("suit_coap: got manifest with size %u\n", (unsigned)size);

        riotboot_flashwrite_t writer;
#ifdef MODULE_SUIT
        suit_manifest_t manifest;
        memset(&manifest, 0, sizeof(manifest));

        manifest.writer = &writer;
        manifest.urlbuf = _url;
        manifest.urlbuf_len = SUIT_URL_MAX;

        int res;
        if ((res = suit_parse(&manifest, _manifest_buf, size)) != SUIT_OK) {
            LOG_INFO("suit_parse() failed. res=%i\n", res);
            return;
        }

        LOG_INFO("suit_parse() success\n");
        if (!(manifest.state & SUIT_MANIFEST_HAVE_IMAGE)) {
            LOG_INFO("manifest parsed, but no image fetched\n");
            return;
        }

        res = suit_policy_check(&manifest);
        if (res) {
            return;
        }

#endif
        if (res == 0) {
            LOG_INFO("suit_coap: finalizing image flash\n");
            riotboot_flashwrite_finish(&writer);

            const riotboot_hdr_t *hdr = riotboot_slot_get_hdr(
                riotboot_slot_other());
            riotboot_hdr_print(hdr);
            xtimer_sleep(1);

            if (riotboot_hdr_validate(hdr) == 0) {
                LOG_INFO("suit_coap: rebooting...");
                pm_reboot();
            }
            else {
                LOG_INFO("suit_coap: update failed, hdr invalid");
            }
        }
    }
    else {
        LOG_INFO("suit_coap: error getting manifest\n");
    }
}

static int _flashwrite_helper(void *arg, size_t offset, uint8_t *buf, size_t len,
                           int more)
{
    suit_manifest_t *manifest = (suit_manifest_t *)arg;
    riotboot_flashwrite_t *writer = manifest->writer;

    if (offset == 0) {
        if (len < RIOTBOOT_FLASHWRITE_SKIPLEN) {
            LOG_WARNING("_suit_flashwrite(): offset==0, len<4. aborting\n");
            return -1;
        }
        offset = RIOTBOOT_FLASHWRITE_SKIPLEN;
        buf += RIOTBOOT_FLASHWRITE_SKIPLEN;
        len -= RIOTBOOT_FLASHWRITE_SKIPLEN;
    }

    if (writer->offset != offset) {
        LOG_WARNING(
            "_suit_flashwrite(): writer->offset=%u, offset==%u, aborting\n",
            (unsigned)writer->offset, (unsigned)offset);
        return -1;
    }

    _print_download_progress(offset, len, manifest->components[0].size);

    return riotboot_flashwrite_putbytes(writer, buf, len, more);
}

static void *_suit_coap_thread(void *arg)
{
    (void)arg;

    LOG_INFO("suit_coap: started.\n");
    msg_t msg_queue[4];
    msg_init_queue(msg_queue, 4);

    _suit_coap_pid = thread_getpid();

    msg_t m;
    while (true) {
        msg_receive(&m);
        DEBUG("suit_coap: got msg with type %" PRIu32 "\n", m.content.value);
        switch (m.content.value) {
            case SUIT_MSG_TRIGGER:
                LOG_INFO("suit_coap: trigger received\n");
                _suit_handle_url(_url);
                break;
            default:
                LOG_WARNING("suit_coap: warning: unhandled msg\n");
        }
    }
    return NULL;
}

void suit_coap_run(void)
{
    thread_create(_stack, SUIT_COAP_STACKSIZE, SUIT_COAP_PRIO,
                  THREAD_CREATE_STACKTEST,
                  _suit_coap_thread, NULL, "suit_coap");
}

static ssize_t _version_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                void *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)"NONE", 4);
}

#ifdef MODULE_RIOTBOOT_SLOT
static ssize_t _slot_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             void *context)
{
    /* context is passed either as NULL or 0x1 for /active or /inactive */
    char c = '0';

    if (context) {
        c += riotboot_slot_other();
    }
    else {
        c += riotboot_slot_current();
    }

    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)&c, 1);
}
#endif

static ssize_t _trigger_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                void *context)
{
    (void)context;
    unsigned code;
    size_t payload_len = pkt->payload_len;
    if (payload_len) {
        if (payload_len >= SUIT_URL_MAX) {
            code = COAP_CODE_REQUEST_ENTITY_TOO_LARGE;
        }
        else {
            code = COAP_CODE_CREATED;
            LOG_INFO("suit: received URL: \"%s\"\n", (char *)pkt->payload);
            suit_coap_trigger(pkt->payload, payload_len);
        }
    }
    else {
        code = COAP_CODE_REQUEST_ENTITY_INCOMPLETE;
    }

    return coap_reply_simple(pkt, code, buf, len,
                             COAP_FORMAT_NONE, NULL, 0);
}

void suit_coap_trigger(const uint8_t *url, size_t len)
{
    memcpy(_url, url, len);
    _url[len] = '\0';
    msg_t m = { .content.value = SUIT_MSG_TRIGGER };
    msg_send(&m, _suit_coap_pid);
}

static const coap_resource_t _subtree[] = {
#ifdef MODULE_RIOTBOOT_SLOT
    { "/suit/slot/active", COAP_METHOD_GET, _slot_handler, NULL },
    { "/suit/slot/inactive", COAP_METHOD_GET, _slot_handler, (void *)0x1 },
#endif
    { "/suit/trigger", COAP_METHOD_PUT | COAP_METHOD_POST, _trigger_handler,
      NULL },
    { "/suit/version", COAP_METHOD_GET, _version_handler, NULL },
};

const coap_resource_subtree_t coap_resource_subtree_suit =
{
    .resources = &_subtree[0],
    .resources_numof = ARRAY_SIZE(_subtree)
};
