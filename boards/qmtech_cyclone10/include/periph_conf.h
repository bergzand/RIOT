/*
 * Copyright (C) 2022 Koen Zandberg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     boards_qmtech_cyclone10
 * @{
 *
 * @brief       Peripheral specific definitions for the qmtech cyclone 10 board
 *
 * @author      Koen Zandberg
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "periph_cpu.h"
#include "kernel_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Timer configuration
 *
 * @{
 */
#define TIMER_NUMOF                 (1)
/** @} */

#define CLOCK_CORECLOCK             (50000000UL)
#define XTIMER_HZ                   (50000000UL)

/**
 * @name   UART configuration
 * @{
 */
static const uart_conf_t uart_config[] = {
    {
        .addr       = (0x40020000L),
        .isr_num    = INT_UART0_BASE,
    },
};

#define UART_NUMOF                  ARRAY_SIZE(uart_config)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */
