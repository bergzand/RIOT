/*
 * Copyright (C) 2021 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_matrix_keypad
 *
 * @{
 * @file
 * @brief       Default configuration
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef MATRIX_KEYPAD_PARAMS_H
#define MATRIX_KEYPAD_PARAMS_H

#include "board.h"
#include "matrix_keypad.h"
#include "matrix_keypad_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Set default configuration parameters
 * @{
 */
#ifndef MATRIX_KEYPAD_PARAM_PARAM1
#define MATRIX_KEYPAD_PARAM_PARAM1
#endif

#ifndef MATRIX_KEYPAD_PARAMS
#define MATRIX_KEYPAD_PARAMS
#endif
/**@}*/

/**
 * @brief   Configuration struct
 */
static const matrix_keypad_params_t matrix_keypad_params[] =
{
    MATRIX_KEYPAD_PARAMS
};

#ifdef __cplusplus
}
#endif

#endif /* MATRIX_KEYPAD_PARAMS_H */
/** @} */
