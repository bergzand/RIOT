/*
 * Copyright (C) 2021 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_matrix_keypad Matrix Keypad
 * @ingroup     drivers_sensors
 * @brief       Matrix keypad driver for row/column keypads
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef MATRIX_KEYPAD_H
#define MATRIX_KEYPAD_H

#include <stdint.h>
#include "periph/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of rows
 */
#ifndef CONFIG_MATRIX_KEYPAD_NUM_ROWS
#define CONFIG_MATRIX_KEYPAD_NUM_ROWS   2
#endif

/**
 * @brief Maximum number of columns
 */
#ifndef CONFIG_MATRIX_KEYPAD_NUM_COLS
#define CONFIG_MATRIX_KEYPAD_NUM_COLS   2
#endif

/**
 * @brief Debounce pattern mask
 */
#ifndef CONFIG_MATRIX_KEYPAD_DEBOUNCE_MASK
#define CONFIG_MATRIX_KEYPAD_DEBOUNCE_MASK  0b11000111
#endif

#if CONFIG_MATRIX_KEYPAD_NUM_COLS <= 8
typedef uint8_t matrix_keypad_state_row_t;
#elif CONFIG_MATRIX_KEYPAD_NUM_COLS <= 16
typedef uint16_t matrix_keypad_state_row_t;
#elif CONFIG_MATRIX_KEYPAD_NUM_COLS <= 32
typedef uint32_t matrix_keypad_state_row_t;
#elif CONFIG_MATRIX_KEYPAD_NUM_COLS <= 64
typedef uint64_t matrix_keypad_state_row_t;
#else
#error Too many columns on matrix keypad
#endif

/**
 * @brief   Device initialization parameters
 */
typedef struct {
    gpio_t rows[CONFIG_MATRIX_KEYPAD_NUM_ROWS]; /** Rows */
    gpio_t columns[CONFIG_MATRIX_KEYPAD_NUM_COLS]; /** Columns */
    uint32_t row2col_delay;                     /** Row change to column scan delay in us */
} matrix_keypad_params_t;

/**
 * @brief   Callback for key state changes
 *
 * @param   arg     callback context
 * @param   row     Row that changed
 * @param   column  Column that changed
 * @param   state   New state of the key, 1 = pressed, 0 = released
 */
typedef void (*matrix_keypad_cb_t)(void *arg, size_t row, size_t column, bool state);

/**
 * @brief   Device descriptor for the driver
 */
typedef struct {
    /** Device initialization parameters */
    matrix_keypad_params_t params;

    /**
     * @brief Debounce history
     */
    uint8_t debounce[CONFIG_MATRIX_KEYPAD_NUM_ROWS][CONFIG_MATRIX_KEYPAD_NUM_COLS];

    /**
     * @brief Current button state
     */
    matrix_keypad_state_row_t state[CONFIG_MATRIX_KEYPAD_NUM_ROWS];

    /**
     * @brief callback context
     */
    void *arg;

    /**
     * @brief Callback
     */
    matrix_keypad_cb_t callback;
} matrix_keypad_t;

/**
 * @brief   Initialize the given device
 *
 * @param[inout] dev        Device descriptor of the driver
 * @param[in]    params     Initialization parameters
 * @param[in]    callback   Callback to call on state changes
 * @param[in]    arg        Context argument for the callback
 *
 * @return                  0 on success
 */
int matrix_keypad_init(matrix_keypad_t *dev,
                       const matrix_keypad_params_t *params,
                       matrix_keypad_cb_t callback,
                       void *arg);

unsigned matrix_keypad_scan(matrix_keypad_t *dev);

static inline bool matrix_keypad_get_key(matrix_keypad_t *dev,
                                         size_t row, size_t column)
{
    assert(column < CONFIG_MATRIX_KEYPAD_NUM_COLS);
    assert(row < CONFIG_MATRIX_KEYPAD_NUM_ROWS);

    return dev->state[row] & (1 << column);
}

#ifdef __cplusplus
}
#endif

#endif /* MATRIX_KEYPAD_H */
/** @} */
