/*
 * Copyright (C) 2021 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_matrix_keypad
 * @{
 *
 * @file
 * @brief       Device driver implementation for the drivers
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */
#include <string.h>

#include "matrix_keypad.h"
#include "matrix_keypad_params.h"

#include "periph/gpio.h"
#include "ztimer.h"

static uint8_t _mask_bits(uint8_t bits)
{
    return bits & CONFIG_MATRIX_KEYPAD_DEBOUNCE_MASK;
}

static void _set_state(matrix_keypad_t *dev, size_t row, size_t column)
{
    dev->state[row] |= (1 << column);
}

static void _clear_state(matrix_keypad_t *dev, size_t row, size_t column)
{
    dev->state[row] &= ~(1 << column);
}

static void _setup_columns(matrix_keypad_t *dev)
{
    for (size_t i = 0; i < CONFIG_MATRIX_KEYPAD_NUM_COLS; i++) {
        gpio_t column = dev->params.columns[i];
        if (column != GPIO_UNDEF) {
            gpio_init(column, GPIO_IN_PU);
        }
    }
}

static void _setup_rows(matrix_keypad_t *dev)
{
    for (size_t i = 0; i < CONFIG_MATRIX_KEYPAD_NUM_ROWS; i++) {
        gpio_t row = dev->params.rows[i];
        if (row != GPIO_UNDEF) {
            gpio_init(row, GPIO_OD_PU); /* Open drain to ensure rows don't conflict */
            gpio_set(row);
        }
    }
}

unsigned _update_key(matrix_keypad_t *dev,
                     size_t row, size_t column, bool status)
{
    /* Pattern based debounce:
     * https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
     */
    unsigned res = 0;
    bool state = dev->state[row] & (1 << column);
    uint8_t *debounce = &dev->debounce[row][column];

    *debounce = (*debounce << 1) | status; /* Update state */

    /* If current state is pressed */
    if (state) {
        if (_mask_bits(*debounce) == 0b11000000) {
            /* Changed */
            *debounce = 0xff;
            _clear_state(dev, row, column);
            res = 1;
            dev->callback(dev->arg, row, column, status);
        }
    }
    else {
        if (_mask_bits(*debounce) == 0x07) {
            *debounce = 0x00;
            /* Changed */
            _set_state(dev, row, column);
            res = 1;
            dev->callback(dev->arg, row, column, status);
        }
    }
    return res;
}

int matrix_keypad_init(matrix_keypad_t *dev, const matrix_keypad_params_t *params,
                       matrix_keypad_cb_t callback, void *arg)
{
    memset(dev, 0, sizeof(matrix_keypad_t));
    memcpy(&dev->params, params, sizeof(matrix_keypad_params_t));
    dev->callback = callback;
    dev->arg = arg;
    _setup_columns(dev);
    _setup_rows(dev);
    return 0;
}

unsigned matrix_keypad_scan(matrix_keypad_t *dev)
{
    unsigned res = 0;
    /* Scan rows */
    for (size_t i = 0; i < CONFIG_MATRIX_KEYPAD_NUM_ROWS; i++) {
        gpio_t row = dev->params.rows[i];

        if (row == GPIO_UNDEF) {
            continue;
        }

        /* Pull the row low */
        gpio_clear(row);

        /* Wait for the row delay */
        ztimer_sleep(ZTIMER_USEC, dev->params.row2col_delay);

        /* Scan columns */
        for (size_t j = 0; j < CONFIG_MATRIX_KEYPAD_NUM_COLS; j++) {
            gpio_t column = dev->params.columns[j];
            if (column == GPIO_UNDEF) {
                continue;
            }
            bool status = !gpio_read(column);
            res += _update_key(dev, i, j, status);
        }
        /* Return the row to high-Z */
        gpio_set(row);
    }
    return res;
}
