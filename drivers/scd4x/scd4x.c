/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_scd4x
 * @{
 *
 * @file
 * @brief       Device driver implementation for the scd4x
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include <errno.h>
#include "byteorder.h"
#include "checksum/crc8.h"
#include "periph/i2c.h"
#include "scd4x.h"
#include "scd4x_constants.h"
#include "scd4x_params.h"
#include "ztimer.h"

static uint8_t _calculate_crc(const uint8_t *data)
{
    return crc8(data, 2, SCD4X_CRC8_POLY, 0xff);
}

static bool _check_crc(const uint8_t *data, uint8_t crc)
{
    return _calculate_crc(data) == crc;
}

static int _i2c_translate_err(int err, int res)
{
    if (err < 0) {
        if (err == -ENXIO) {
            return SCD4X_ERR_NODEV;
        }
        return SCD4X_ERR_BUS;
    }
    return res;
}

static inline i2c_t bus(scd4x_t *dev)
{
    return dev->params.i2c;
}

static int _send_command(scd4x_t *dev, uint16_t command, i2c_flags_t flags)
{
    be_uint16_t be_command = byteorder_htons(command);

    return i2c_write_bytes(bus(dev), SCD4X_I2C_ADDRESS, &be_command, sizeof(be_command), flags);
}

int _read_data(scd4x_t *dev, uint16_t *result, i2c_flags_t flags)
{
    uint8_t data[3];
    int res = i2c_read_bytes(bus(dev), SCD4X_I2C_ADDRESS, &data, sizeof(data), flags);
    if (res < 0) {
        return res;
    }

    if (!_check_crc(data, data[2])) {
        return SCD4X_ERR_CRC;
    }

    *result = (uint16_t)data[0] << 8 | data[1];

    return 0;
}

static int _read_register(scd4x_t *dev, uint16_t reg, uint16_t *result)
{
    uint8_t data[3];
    int res = i2c_read_regs(bus(dev), SCD4X_I2C_ADDRESS, reg, &data, sizeof(data), I2C_REG16);
    if (res < 0) {
        return res;
    }

    if (!_check_crc(data, data[2])) {
        return SCD4X_ERR_CRC;
    }

    *result = (uint16_t)data[0] << 8 | data[1];

    return 0;
}

int scd4x_start_measurements(scd4x_t *dev, bool low_power)
{
    if (!dev->measuring) {
        return SCD4X_ERR_MEASURING;
    }

    i2c_acquire(bus(dev));
    int res = _send_command(dev,
            low_power ? SCD4X_START_LOW_POWER_PERIODIC_MEASUREMENT : SCD4X_START_PERIODIC_MEASUREMENT,
            0);
    i2c_release(bus(dev));

    if (res == 0) {
        dev->measuring = true;
    }
    return _i2c_translate_err(res, 0);
}

int scd4x_stop_measurements(scd4x_t *dev)
{
    if (dev->measuring) {
        return SCD4X_ERR_NOT_MEASURING;
    }

    i2c_acquire(bus(dev));
    int res = _send_command(dev, SCD4X_STOP_PERIODIC_MEASUREMENT, 0);
    i2c_release(bus(dev));

    if (res == 0) {
        dev->measuring = false;
    }
    return _i2c_translate_err(res, 0);
}

int scd4x_measurement_ready(scd4x_t *dev)
{
    uint16_t data = 0;

    if (!dev->measuring) {
        return SCD4X_ERR_NOT_MEASURING;
    }

    i2c_acquire(bus(dev));
    int res = _read_register(dev, SCD4X_GET_DATA_READY_STATUS, &data);
    i2c_release(bus(dev));

    if (res < 0) {
        return res;
    }
    int ready = (data & 0x7FF) == 0;

    return _i2c_translate_err(res, ready);
}

int scd4x_read_co2_measurement(scd4x_t *dev)
{
    uint16_t co2 = 0;

    if (!dev->measuring) {
        return SCD4X_ERR_NOT_MEASURING;
    }
    i2c_acquire(bus(dev));
    int res = _read_register(dev, SCD4X_READ_MEASUREMENT, &co2);
    i2c_release(bus(dev));

    return _i2c_translate_err(res, co2);
}

int scd4x_init(scd4x_t *dev, const scd4x_params_t *params)
{
    assert(dev && params);
    dev->params = *params;


    return SCD4X_OK;
}
