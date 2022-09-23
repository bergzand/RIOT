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
 * @brief       CORECONF IETF-hardware module implementation
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#include "net/coreconf.h"
#include "net/gcoap.h"
#include "xfa.h"
#include "saul.h"
#include "saul_reg.h"
#include "fmt.h"

#define HARDWARE_MODULE_SID                             1001000
#define HARDWARE_HARDWARE_SID                           1001004
#define HARDWARE_HARDWARE_COMPONENT_SID                 1001014
#define HARDWARE_HARDWARE_COMPONENT_DESC_SID            1001019
#define HARDWARE_HARDWARE_COMPONENT_CLASS_SID           (HARDWARE_MODULE_SID + 17)
#define HARDWARE_HARDWARE_COMPONENT_NAME_SID            1001026
#define HARDWARE_HARDWARE_COMPONENT_SD_SID              (HARDWARE_MODULE_SID + 30)
#define HARDWARE_HARDWARE_COMPONENT_SD_UNITS_SID        (HARDWARE_MODULE_SID + 32)
#define HARDWARE_HARDWARE_COMPONENT_SD_VAL_SID          (HARDWARE_MODULE_SID + 33)
#define HARDWARE_HARDWARE_COMPONENT_SD_VAL_SCALE_SID    (HARDWARE_MODULE_SID + 35)
#define HARDWARE_HARDWARE_COMPONENT_SD_VAL_TYPE_SID     (HARDWARE_MODULE_SID + 37)
#define HARDWARE_HARDWARE_COMPONENT_STATE_SID           (HARDWARE_MODULE_SID + 41)

typedef enum {
    HW_SENSOR_TYPE_OTHER = 1,
    HW_SENSOR_TYPE_UNKNOWN = 2,
    HW_SENSOR_TYPE_VOLTS_AC = 3,
    HW_SENSOR_TYPE_VOLTS_DC = 4,
    HW_SENSOR_TYPE_AMPS = 5,
    HW_SENSOR_TYPE_WATT = 6,
    HW_SENSOR_TYPE_HERTZ = 7,
    HW_SENSOR_TYPE_CELSIUS = 8,
    HW_SENSOR_TYPE_PERCENT_RH = 9,
    HW_SENSOR_TYPE_RPM = 10,
    HW_SENSOR_TYPE_CMM = 11,
    HW_SENSOR_TYPE_TRUTH = 12,
} hw_sensor_value_type;

static unsigned _phydat_sense2yang_type(uint8_t type)
{
    switch (type) {
        case UNIT_UNDEF:
            return HW_SENSOR_TYPE_UNKNOWN;
        case UNIT_V:
            return HW_SENSOR_TYPE_VOLTS_DC;
        case UNIT_A:
            return HW_SENSOR_TYPE_AMPS;
        case UNIT_W:
            return HW_SENSOR_TYPE_WATT;
        case UNIT_TEMP_C:
            return HW_SENSOR_TYPE_CELSIUS;
        case UNIT_M3:
            return HW_SENSOR_TYPE_CMM;
        case UNIT_BOOL:
            return HW_SENSOR_TYPE_TRUTH;
        default:
            return HW_SENSOR_TYPE_OTHER;
    }
}

static const saul_reg_t *_get_dev(coreconf_ctx_t *ctx, void **argv)
{
    if (argv[0]) {
        return argv[0];
    }
    if (fmt_is_number(ctx->state->uri_query)) {
        uint32_t num = scn_u32_dec(ctx->state->uri_query, 5);
        return saul_reg_find_nth(num);
    }
    return NULL;
}

static size_t _num_saul_devs(void)
{
    size_t num = 0;
    saul_reg_t *dev = saul_reg;
    while (dev) {
        dev = dev->next;
        num++;
    }
    return num;
}

static const char *_devdescriptor(const saul_reg_t *dev) {
    if (dev->name == NULL) {
        return "(no name)";
    } else {
        return dev->name;
    }
}

static int _dev_id(const saul_reg_t *dev) {
    int res = 0;
    saul_reg_t *lookup = saul_reg;
    while (lookup) {
        if (lookup == dev) {
            break;
        }
        lookup = lookup->next;
        res++;
    }
    return res;
}

static void _hw_hardware_component_class(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    (void)node;
    (void)argv;
    nanocbor_fmt_uint(coreconf_encoder_cbor(enc), 1000012); /* Temp number for "sensor" */
}

static int _hw_hardware_component_name(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    (void)node;
    const saul_reg_t *dev = _get_dev(&enc->ctx, argv);
    if (!dev) {
        return -1;
    }
    uint16_t num = _dev_id(dev);
    char name[6];
    fmt_u16_dec(name, num);
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), name);
    return 0;
}


static int _hw_hardware_component_description(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{
    (void)node;
    const saul_reg_t *dev = _get_dev(&enc->ctx, argv);
    if (!dev) {
        return -1;
    }
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), _devdescriptor(dev));
    return 0;
}

static int _fmt_hw_hardware_component_sensor(coreconf_encoder_t *enc, const coreconf_node_t *node, const saul_reg_t *dev)
{
    (void)node;
    int dim;
    phydat_t res;

    dim = saul_reg_read((saul_reg_t*)dev, &res);

    if (dim < 0) {
        return -1;
    }

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), 4);

    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SD_SID,
            HARDWARE_HARDWARE_COMPONENT_SD_UNITS_SID);
    nanocbor_put_tstr(coreconf_encoder_cbor(enc), phydat_unit_to_str(res.unit));

    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SD_SID,
            HARDWARE_HARDWARE_COMPONENT_SD_VAL_SID);
    nanocbor_fmt_int(coreconf_encoder_cbor(enc), res.val[0]);

    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SD_SID,
            HARDWARE_HARDWARE_COMPONENT_SD_VAL_SCALE_SID);
    nanocbor_fmt_int(coreconf_encoder_cbor(enc), res.scale);

    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SD_SID,
            HARDWARE_HARDWARE_COMPONENT_SD_VAL_TYPE_SID);
    nanocbor_fmt_int(coreconf_encoder_cbor(enc), _phydat_sense2yang_type(res.unit));
    return 0;
}

static int _read_component(coreconf_encoder_t *enc, const coreconf_node_t *node, void **argv)
{

    saul_reg_t *dev = argv[0];
    unsigned type = dev->driver->type;

    size_t map_len = type & SAUL_CAT_SENSE ? 4 : 3;

    nanocbor_fmt_map(coreconf_encoder_cbor(enc), map_len);

    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SID,
            HARDWARE_HARDWARE_COMPONENT_NAME_SID);
    _hw_hardware_component_name(enc, node, argv);
    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SID,
            HARDWARE_HARDWARE_COMPONENT_CLASS_SID);
    _hw_hardware_component_class(enc, node, argv);
    coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SID,
            HARDWARE_HARDWARE_COMPONENT_DESC_SID);
    _hw_hardware_component_description(enc, node, argv);

    if (type & SAUL_CAT_SENSE) {
        /* It's a sensor */
        coreconf_cbor_sid(enc, HARDWARE_HARDWARE_COMPONENT_SID,
                HARDWARE_HARDWARE_COMPONENT_SD_SID);
        return _fmt_hw_hardware_component_sensor(enc, node, dev);
    }
    return 0;
}

static int _parse_component(coreconf_ctx_t *ctx, const coreconf_node_t *node, void **argv)
{

    coreconf_encoder_t *enc = container_of(ctx, coreconf_encoder_t, ctx);
    if (coreconf_enc_args_empty(enc)) {
        size_t num_devices = _num_saul_devs();

        nanocbor_fmt_array(coreconf_encoder_cbor(enc), num_devices);

        saul_reg_t *dev = saul_reg;
        while (dev) {
            argv[0] = dev;
            int res = coreconf_read_container(enc, node, argv);
            if (res < 0) {
                return res;
            }
            dev = dev->next;
        }
        argv[0] = NULL;
    }
    else {
        const saul_reg_t *dev = _get_dev(ctx, argv);
        argv[0] = (void*)dev;
        return coreconf_read_container(enc, node, argv);
    }
    return 0;
}

CORECONF_CONTAINER(root, hardware_hardware, HARDWARE_HARDWARE_SID, CORECONF_NODE_CONFIG);

CORECONF_CONTAINER_LIST(hardware_hardware, hardware_hardware_component,
        HARDWARE_HARDWARE_COMPONENT_SID, CORECONF_NODE_CONFIG, _parse_component,
        _read_component, NULL);

CORECONF_LEAF(hardware_hardware_component, HARDWARE_HARDWARE_COMPONENT_NAME_SID,
        CORECONF_NODE_CONFIG, _hw_hardware_component_name, NULL);
CORECONF_LEAF(hardware_hardware_component, HARDWARE_HARDWARE_COMPONENT_DESC_SID,
        CORECONF_NODE_CONFIG, _hw_hardware_component_description, NULL);
