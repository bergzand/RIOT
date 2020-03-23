/*
 * Copyright (C) 2019 Koen Zandberg
 *               2020 Inria
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
 * @brief       SUIT Handlers for the command sequences in the common section of
 *              a SUIT manifest.
 *
 * This file contains the functions to handle command sequences from a SUIT
 * manifest. This includes both directives and conditions.
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include <inttypes.h>
#include <nanocbor/nanocbor.h>

#include "kernel_defines.h"
#include "suit/conditions.h"
#include "suit/handlers.h"
#include "suit/policy.h"
#include "suit.h"
#include "riotboot/hdr.h"
#include "riotboot/slot.h"

#ifdef MODULE_SUIT_TRANSPORT_COAP
#include "suit/transport/coap.h"
#endif

#include "log.h"

static int _validate_uuid(suit_manifest_t *manifest,
                          suit_param_ref_t *ref,
                          uuid_t *uuid)
{
    const uint8_t *uuid_manifest_ptr;
    size_t len = sizeof(uuid_t);
    nanocbor_value_t it;

    if ((suit_param_ref_to_cbor(manifest, ref, &it) == 0) ||
            (nanocbor_get_bstr(&it, &uuid_manifest_ptr, &len) < 0)) {
        return SUIT_ERR_INVALID_MANIFEST;
    }

    char uuid_str[UUID_STR_LEN + 1];
    char uuid_str2[UUID_STR_LEN + 1];
    uuid_to_string((uuid_t *)uuid_manifest_ptr, uuid_str);
    uuid_to_string(uuid, uuid_str2);
    LOG_INFO("Comparing %s to %s from manifest\n", uuid_str2, uuid_str);

    return uuid_equal(uuid, (uuid_t *)uuid_manifest_ptr)
           ? SUIT_OK
           : SUIT_ERR_COND;
}

static int _cond_vendor_handler(suit_manifest_t *manifest,
                                int key,
                                nanocbor_value_t *it)
{
    (void)key;
    (void)it;
    LOG_INFO("validating vendor ID\n");
    int rc = _validate_uuid(manifest, &manifest->param_vendor_id,
                            suit_get_vendor_id());
    if (rc == SUIT_OK) {
        LOG_INFO("validating vendor ID: OK\n");
        manifest->validated |= SUIT_VALIDATED_VENDOR;
    }
    return rc;
}

static int _cond_class_handler(suit_manifest_t *manifest,
                               int key,
                               nanocbor_value_t *it)
{
    (void)key;
    (void)it;
    LOG_INFO("validating class id\n");
    int rc = _validate_uuid(manifest, &manifest->param_class_id,
                            suit_get_class_id());
    if (rc == SUIT_OK) {
        LOG_INFO("validating class id: OK\n");
        manifest->validated |= SUIT_VALIDATED_CLASS;
    }
    return rc;
}

static int _cond_comp_offset(suit_manifest_t *manifest,
                             int key,
                             nanocbor_value_t *it)
{
    (void)manifest;
    (void)key;
    uint32_t offset;

    int rc = nanocbor_get_uint32(it, &offset);
    if (rc < 0) {
        LOG_WARNING("_cond_comp_offset(): expected int, got rc=%i type=%i\n",
                    rc, nanocbor_get_type(it));
        return SUIT_ERR_INVALID_MANIFEST;
    }
    uint32_t other_offset = (uint32_t)riotboot_slot_offset(
        riotboot_slot_other());

    LOG_INFO("Comparing manifest offset %u with other slot offset %u\n",
             (unsigned)offset, (unsigned)other_offset);
    return other_offset == offset ? SUIT_OK : SUIT_ERR_COND;
}

static int _dtv_set_comp_idx(suit_manifest_t *manifest,
                             int key,
                             nanocbor_value_t *it)
{
    (void)key;
    if (nanocbor_get_type(it) == NANOCBOR_TYPE_FLOAT) {
        LOG_DEBUG("_dtv_set_comp_idx() ignoring boolean and floats\n)");
        nanocbor_skip(it);
    }
    else if (nanocbor_get_uint32(it, &manifest->component_current) < 0) {
        return SUIT_ERR_INVALID_MANIFEST;
    }
    if (manifest->component_current >= SUIT_COMPONENT_MAX) {
        return SUIT_ERR_INVALID_MANIFEST;
    }
    LOG_DEBUG("Setting component index to %d\n",
              (int)manifest->component_current);
    return 0;
}

static int _dtv_run_seq_cond(suit_manifest_t *manifest,
                             int key,
                             nanocbor_value_t *it)
{
    (void)key;
    LOG_DEBUG("Starting conditional sequence handler\n");
    return suit_handle_manifest_structure_bstr(manifest, it,
                                               suit_command_sequence_handlers,
                                               suit_command_sequence_handlers_len);
}

static int _dtv_try_each(suit_manifest_t *manifest,
                         int key, nanocbor_value_t *it)
{
    (void)key;
    LOG_DEBUG("Starting suit-directive-try-each handler\n");
    nanocbor_value_t container;

    if ((nanocbor_enter_array(it, &container) < 0) &&
        (nanocbor_enter_map(it, &container) < 0)) {
        return SUIT_ERR_INVALID_MANIFEST;
    }

    int res = SUIT_ERR_COND;
    while (!nanocbor_at_end(&container)) {
        nanocbor_value_t _container = container;
        /* `_container` should be CBOR _bstr wrapped according to the spec, but
         * it is not */
        res = suit_handle_manifest_structure(manifest, &_container,
                                             suit_command_sequence_handlers,
                                             suit_command_sequence_handlers_len);

        nanocbor_skip(&container);

        if (res != SUIT_ERR_COND) {
            break;
        }
    }

    return res;
}

static int _dtv_set_param(suit_manifest_t *manifest, int key,
                          nanocbor_value_t *it)
{
    (void)key;
    /* `it` points to the entry of the map containing the type and value */
    nanocbor_value_t map;

    nanocbor_enter_map(it, &map);

    while (!nanocbor_at_end(&map)) {
        /* map points to the key of the param */
        int32_t param_key;
        if (nanocbor_get_int32(&map, &param_key) < 0) {
            return SUIT_ERR_INVALID_MANIFEST;
        }
        LOG_DEBUG("param_key=%" PRIi32 "\n", param_key);
        unsigned int type = nanocbor_get_type(&map);
        /* Filter 'complex' types and only allow int, nint, bstr and tstr types
         * for parameter values */
        if (type > NANOCBOR_TYPE_TSTR) {
            return SUIT_ERR_INVALID_MANIFEST;
        }
        suit_param_ref_t *ref;
        switch (param_key) {
            case 1: /* SUIT VENDOR ID */
                ref = &manifest->param_vendor_id;
                break;
            case 2: /* SUIT URI LIST */
                ref = &manifest->param_class_id;
                break;
            case 3: /* SUIT DIGEST */
                ref = &manifest->param_digest;
                break;
            case 14: /* SUIT IMAGE SIZE */
                ref = &manifest->param_size;
                break;
            case 21: /* SUIT URI */
                ref = &manifest->param_uri;
                break;
            default:
                LOG_DEBUG("Unsupported parameter %" PRIi32 "\n", param_key);
                return SUIT_ERR_UNSUPPORTED;
        }


        suit_param_cbor_to_ref(manifest, ref, &map);

        /* Simple skip is sufficient to skip non-complex types */
        nanocbor_skip_simple(&map);
    }
    return SUIT_OK;
}

static int _dtv_fetch(suit_manifest_t *manifest, int key,
                      nanocbor_value_t *_it)
{
    (void)key; (void)_it;
    LOG_DEBUG("_dtv_fetch() key=%i\n", key);

    const uint8_t *url;
    size_t url_len;

    int err = nanocbor_get_tstr(&manifest->components[0].url, &url, &url_len);
    if (err < 0) {
        LOG_DEBUG("URL parsing failed\n)");
        return err;
    }
    memcpy(manifest->urlbuf, url, url_len);
    manifest->urlbuf[url_len] = '\0';

    LOG_DEBUG("_dtv_fetch() fetching \"%s\" (url_len=%u)\n", manifest->urlbuf,
              (unsigned)url_len);

    int target_slot = riotboot_slot_other();
    riotboot_flashwrite_init(manifest->writer, target_slot);

    int res = -1;

    if (0) {}
#ifdef MODULE_SUIT_TRANSPORT_COAP
    else if (strncmp(manifest->urlbuf, "coap://", 7) == 0) {
        res = suit_coap_get_blockwise_url(manifest->urlbuf, COAP_BLOCKSIZE_64,
                                          suit_flashwrite_helper,
                                          manifest);
    }
#endif
#ifdef MODULE_SUIT_TRANSPORT_MOCK
    else if (strncmp(manifest->urlbuf, "test://", 7) == 0) {
        res = SUIT_OK;
    }
#endif
    else {
        LOG_WARNING("suit: unsupported URL scheme!\n)");
        return res;
    }

    if (res) {
        LOG_INFO("image download failed\n)");
        return res;
    }

    manifest->state |= SUIT_MANIFEST_HAVE_IMAGE;

    LOG_DEBUG("Update OK\n");
    return SUIT_OK;
}

static int _dtv_verify_image_match(suit_manifest_t *manifest, int key,
                                   nanocbor_value_t *_it)
{
    (void)key; (void)_it;
    LOG_DEBUG("dtv_image_match\n");
    const uint8_t *digest;
    size_t digest_len;
    int target_slot = riotboot_slot_other();

    uint32_t img_size;
    nanocbor_value_t param_size;
    if ((suit_param_ref_to_cbor(manifest, &manifest->param_size, &param_size) == 0) ||
            (nanocbor_get_uint32(&param_size, &img_size) < 0)) {
        return SUIT_ERR_INVALID_MANIFEST;
    }

    LOG_INFO("Verifying image digest\n");
    nanocbor_value_t _v;
    if (suit_param_ref_to_cbor(manifest, &manifest->param_digest, &_v) == 0) {
        return SUIT_ERR_INVALID_MANIFEST;
    }

    int res = nanocbor_get_subcbor(&_v, &digest, &digest_len);
    if (res < 0) {
        LOG_DEBUG("Unable to parse digest structure\n");
        return SUIT_ERR_INVALID_MANIFEST;
    }

    /* "digest" points to a 36 byte string that includes the digest type.
     * riotboot_flashwrite_verify_sha256() is only interested in the 32b digest,
     * so shift the pointer accordingly.
     */
    res = riotboot_flashwrite_verify_sha256(digest + 4,
                                            img_size,
                                            target_slot);
    if (res != 0) {
        return SUIT_ERR_COND;
    }
    return SUIT_OK;
}

/* begin{code-style-ignore} */
const suit_manifest_handler_t suit_command_sequence_handlers[] = {
    [SUIT_COND_VENDOR_ID]        = _cond_vendor_handler,
    [SUIT_COND_CLASS_ID]         = _cond_class_handler,
    [SUIT_COND_IMAGE_MATCH]      = _dtv_verify_image_match,
    [SUIT_COND_COMPONENT_OFFSET] = _cond_comp_offset,
    [SUIT_DIR_SET_COMPONENT_IDX] = _dtv_set_comp_idx,
    [SUIT_DIR_TRY_EACH]          = _dtv_try_each,
    [SUIT_DIR_SET_PARAM]         = _dtv_set_param,
    [SUIT_DIR_OVERRIDE_PARAM]    = _dtv_set_param,
    [SUIT_DIR_FETCH]             = _dtv_fetch,
    [SUIT_DIR_RUN_SEQUENCE]      = _dtv_run_seq_cond,
};
/* end{code-style-ignore} */

const size_t suit_command_sequence_handlers_len = ARRAY_SIZE(suit_command_sequence_handlers);
