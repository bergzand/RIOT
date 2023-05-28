/*
 * Copyright (C) 2023 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the scd4x CO2 sensor
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include <stdio.h>
#include "scd4x_params.h"
#include "scd4x.h"
#include "fmt.h"
#include "ztimer.h"

int main(void)
{
    scd4x_t dev;
    puts("scd4x test application\n");
    switch (scd4x_init(&dev, &scd4x_params[0])) {
        case SCD4X_ERR_BUS:
            puts("[Error] Something went wrong when using the I2C bus");
            return 1;
        case SCD4X_ERR_NODEV:
            puts("[Error] Unable to communicate with any scd4x device");
            return 1;
        default:
            /* all good -> do nothing */
            break;
    }
    int res = scd4x_start_measurements(&dev, false);
    if (res < 0) {
        printf("Unable to start measurements: %d\n", res);
    }
    puts("Initialization successful\n");

    puts("\n+--------Starting Measurements--------+");
    while (1) {
        res = scd4x_measurement_ready(&dev);
        res = 1;
        if (res < 0) {
            printf("Error reading sensor measurement ready: %i\n", res);
        }
        else if (res == 1) {
            //puts("Reading measurement");
            int level = scd4x_read_measurement(&dev);
            if (level < 0) {
                //printf("Error reading sensor measurement: %i\n", level);
            }
            else {
                printf("CO2: %"PRIu16"\n", (uint16_t)level);
            }
        }
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
