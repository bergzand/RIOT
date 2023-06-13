/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_scd4x scd4x
 * @ingroup     drivers_sensors
 * @brief       scd4x CO2 sensor
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef SCD4X_H
#define SCD4X_H

#include "periph/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Status and error return codes
 */
enum {
    SCD4X_OK                    =  0, /**< everything was fine */
    SCD4X_ERR_BUS               = -1, /**< bus error */
    SCD4X_ERR_NODEV             = -2, /**< did not detect scd40 or scd41 device */
    SCD4X_ERR_CRC               = -3, /**< CRC error in data */
    SCD4X_ERR_NOT_MEASURING     = -5, /**< Device is not measuring and can't handle request */
    SCD4X_ERR_MEASURING         = -4, /**< Device is measuring and can't handle request */
};

/**
 * @brief   Device initialization parameters
 */
typedef struct {
    /* I2C details */
    i2c_t i2c;                      /**< I2C device which is used */
} scd4x_params_t;

/**
 * @brief   Device descriptor for the driver
 */
typedef struct {
    scd4x_params_t params;  /**< Device initialization parameters */
    bool measuring;         /**< Device is measuring co2 levels */
} scd4x_t;

/**
 * @brief   Initialize the given device
 *
 * @param[inout] dev        Device descriptor of the driver
 * @param[in]    params     Initialization parameters
 *
 * @return                  0 on success
 */
int scd4x_init(scd4x_t *dev, const scd4x_params_t *params);

int scd4x_start_measurements(scd4x_t *dev, bool low_power);
int scd4x_read_measurement(scd4x_t *dev);
int scd4x_read_co2_measurement(scd4x_t *dev);
int scd4x_measurement_ready(scd4x_t *dev);
#ifdef __cplusplus
}
#endif

#endif /* SCD4X_H */
/** @} */
