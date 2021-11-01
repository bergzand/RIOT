/*
 * Copyright (C) 2021 Koen Zandberg <koen@bergzand.net>
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
 * @brief       Test application for the matrix keypad driver
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 *
 * @}
 */

#include <stdlib.h>
#include <stdio.h>

#include "matrix_keypad_params.h"
#include "matrix_keypad.h"
#include "ztimer.h"
#include "fmt.h"

#define MAINLOOP_DELAY  (2)         /* read sensor every 2 seconds */

int main(void)
{
    matrix_keypad_t dev;
    (void)dev;
    return 0;
}
