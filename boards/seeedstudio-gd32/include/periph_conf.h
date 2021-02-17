/*
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    boards_seeedstudio-gd32 SeeedStudio GD32 RISC-V board
 * @ingroup     boards
 * @brief       Support for the SeeedStudio GD32 RISC-V board
 * @{
 *
 * @file
 * @brief       Board specific definitions for the SeeedStudio GD32 RISC-V board
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "macros/units.h"
#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_CORECLOCK     MHZ(104)
#define CLOCK_AHB           CLOCK_CORECLOCK
#define CLOCK_APB1          CLOCK_CORECLOCK/2
#define CLOCK_APB2          CLOCK_CORECLOCK

extern void isr_timer2(unsigned irq);
extern void isr_timer3(unsigned irq);

/**
 * @name   Timer configuration
 * @{
 */
static const timer_conf_t timer_config[] = {
    {
        .dev      = TIMER2,
        .max      = 0x0000ffff,
        .rcu_mask = RCU_APB1EN_TIMER2EN_Msk,
        .bus      = APB1,
        .irqn     = TIMER2_IRQn
    },
    {
        .dev      = TIMER3,
        .max      = 0x0000ffff,
        .rcu_mask = RCU_APB1EN_TIMER3EN_Msk,
        .bus      = APB1,
        .irqn     = TIMER3_IRQn
    }
};

#define TIMER_0_IRQN         TIMER2_IRQn
#define TIMER_1_IRQN         TIMER3_IRQn

#define TIMER_NUMOF         ARRAY_SIZE(timer_config)
/** @} */

extern void isr_usart1(unsigned irq);
extern void isr_usart0(unsigned irq);
/**
 * @name   UART configuration
 * @{
 */
static const uart_conf_t uart_config[] = {
    {
        .dev        = USART0,
        .rcu_mask   = RCU_APB2EN_USART0EN_Msk,
        .rx_pin     = GPIO_PIN(PORT_A, 10),
        .tx_pin     = GPIO_PIN(PORT_A, 9),
        .bus        = APB2,
        .irqn       = USART0_IRQn,
        .isr        = isr_usart0
    },
};

#define UART_NUMOF          ARRAY_SIZE(uart_config)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */


