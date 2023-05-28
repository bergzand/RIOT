/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_scd4x
 *
 * @{
 * @file
 * @brief       Default configuration
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef SCD4X_PARAMS_H
#define SCD4X_PARAMS_H

#include "board.h"
#include "scd4x.h"
#include "scd4x_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters
 * @{
 */
#ifndef SCD4X_PARAM_I2C_DEV
#define SCD4X_PARAM_I2C_DEV        I2C_DEV(0)
#endif

#ifndef SCD4X_PARAMS
#define SCD4X_PARAMS                        \
    {                                       \
        .i2c = SCD4X_PARAM_I2C_DEV,         \
    }
#endif
/**@}*/

/**
 * @brief   Configuration struct
 */
static const scd4x_params_t scd4x_params[] =
{
    SCD4X_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* SCD4X_PARAMS_H */
/** @} */
