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
 * @brief       Internal addresses, registers and constants
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef SCD4X_CONSTANTS_H
#define SCD4X_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define SCD4X_I2C_ADDRESS       (0x62)

#define SCD4X_CRC8_POLY         (0x31)

#define SCD4X_ERR_CRC         (-1000)
/**
 * @name Commands for the SCD4x
 * @{
 */
#define SCD4X_START_PERIODIC_MEASUREMENT              (0x21B1) /**< Start periodic measurements */
#define SCD4X_READ_MEASUREMENT                        (0xEC05) /**< Read sensor output */
#define SCD4X_STOP_PERIODIC_MEASUREMENT               (0x3F86) /**< Stop periodic measurement */
#define SCD4X_SET_TEMPERATURE_OFFSET                  (0x241D) /**< Set the temperature offset */
#define SCD4X_GET_TEMPERATURE_OFFSET                  (0x2318) /**< Get the temperature offset */
#define SCD4X_SET_SENSOR_ALTITUDE                     (0x2427) /**< Set the sensor altitude */
#define SCD4X_GET_SENSOR_ALTITUDE                     (0x2322) /**< Get the sensor altitude */
#define SCD4X_SET_AMBIENT_PRESSURE                    (0xE000) /**< Set the ambient pressure */
#define SCD4X_PERFORM_FORCED_RECALIBRATION            (0x362F) /**< Perform forced recalibration */
#define SCD4X_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED  (0x2416) /**< Configure the ASC */
#define SCD4X_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED  (0x2313) /**< Get the ASC configuration */
#define SCD4X_START_LOW_POWER_PERIODIC_MEASUREMENT    (0x21AC) /**< Start low power periodic */
#define SCD4X_GET_DATA_READY_STATUS                   (0xE4B8) /**< Retrieve data ready status */
#define SCD4X_PERSIST_SETTINGS                        (0x3615) /**< Write settings to ROM */
#define SCD4X_GET_SERIAL_NUMBER                       (0x3682) /**< Read the serial number */
#define SCD4X_PERFORM_SELF_TEST                       (0x3639) /**< Perform self test */
#define SCD4X_PERFORM_FACTORY_RESET                   (0x3632) /**< Reset all configuration */
#define SCD4X_REINIT                                  (0x3646) /**< Reinitialize the sensor */
#define SCD4X_MEASURE_SINGLE_SHOT                     (0x219D) /**< Perform a single measurement */
#define SCD4X_MEASURE_SINGLE_SHOT_RHT_ONLY            (0x2196) /**< Single shot CO2, RH and temp */
#define SCD4X_POWER_DOWN                              (0x36E0) /**< Power down the sensor */
#define SCD4X_WAKE_UP                                 (0x36F6) /**< Wake the sensor */
/** @} */


#ifdef __cplusplus
}
#endif

#endif /* SCD4X_CONSTANTS_H */
/** @} */
