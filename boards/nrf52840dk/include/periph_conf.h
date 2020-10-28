/*
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_nrf52840dk
 * @{
 *
 * @file
 * @brief       Peripheral configuration for the nRF52840 DK
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "periph_conf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    SPI configuration
 * @{
 */
static const spi_conf_t spi_config[] = {
    {
        .dev  = NRF_SPIM0,
        .sclk = GPIO_PIN(1, 15),
        .mosi = GPIO_PIN(1, 13),
        .miso = GPIO_PIN(1, 14),
    },
};
#define SPI_NUMOF           ARRAY_SIZE(spi_config)
/** @} */

/**
 * @name    UART configuration
 * @{
 */
static const uart_conf_t uart_config[] = {
    { /* Mapped to USB virtual COM port */
        .dev        = NRF_UARTE0,
        .rx_pin     = GPIO_PIN(0,8),
        .tx_pin     = GPIO_PIN(0,6),
#ifdef MODULE_PERIPH_UART_HW_FC
        .rts_pin    = GPIO_UNDEF,
        .cts_pin    = GPIO_UNDEF,
#endif
        .irqn       = UARTE0_UART0_IRQn,
    },
    { /* Mapped to Arduino D0/D1 pins */
        .dev        = NRF_UARTE1,
        .rx_pin     = GPIO_PIN(1,1),
        .tx_pin     = GPIO_PIN(1,2),
#ifdef MODULE_PERIPH_UART_HW_FC
        .rts_pin    = GPIO_UNDEF,
        .cts_pin    = GPIO_UNDEF,
#endif
        .irqn       = UARTE1_IRQn,
    },
};

#define UART_0_ISR          (isr_uart0)
#define UART_1_ISR          (isr_uarte1)

#define UART_NUMOF          ARRAY_SIZE(uart_config)
/** @} */

/**
 * @name    QSPI configuration
 * @{
 */
static const qspi_conf_t qspi_config[] = {
    {
        .dev    = NRF_QSPI,
        .cs     = GPIO_PIN(0, 17),
        .sclk   = GPIO_PIN(0, 19),
        .io0    = GPIO_PIN(0, 20),
        .io1    = GPIO_PIN(0, 21),
        .io2    = GPIO_PIN(0, 22),
        .io3    = GPIO_PIN(0, 23)
    }
};

#define QSPI_NUMOF          ARRAY_SIZE(qspi_config)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */
