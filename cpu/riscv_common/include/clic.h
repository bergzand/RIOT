/*
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */
/**
 * @ingroup         cpu_riscv_common
 * @{
 *
 * @file
 * @brief           RISCV CLIC interrupt controller definitions
 *
 * RISCV implementations using this peripheral must define the `CLIC_BASE_ADDR`,
 * in order to use the PLIC as interrupt controller. Also required is
 * CLIC_NUM_INTERRUPTS.
 *
 * @author          Koen Zandberg <koen@bergzand.net>
 */
#ifndef CLIC_H
#define CLIC_H

#include "cpu_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CLIC registry offset helper
 */
#define CLIC_REGP(offset)   ((volatile uint32_t *) ((CLIC_BASE_ADDR) + (offset)))

/**
 * @name CLIC configuration registers
 * @{
 */
#define CLIC_CFG        *((volatile uint8_t*)CLIC_REGP(0x0))
#define CLIC_INFO       *((volatile uint32_t*)CLIC_REGP(0x4)
#define CLIC_MTH        *((volatile uint8_t*)CLIC_REGP(0xb)
#define CLIC_INT_ADDR   CLIC_REGP(0x1000)
#define CLIC_INT        ((volatile eclic_clicint_t *)CLIC_INT_ADDR)
/** @} */

/**
 * @brief   PLIC callback declaration
 *
 * @param   irq     Interrupt number
 */
typedef void (*clic_isr_cb_t)(unsigned irq);

/**
 * @brief RISC-V CLIC per interrupt configuration registers
 */
typedef struct __attribute((packed)) {
    volatile uint8_t ip;     /**< Interrupt pending */
    volatile uint8_t ie;     /**< Interrupt enable */
    volatile uint8_t attr;   /**< Interrupt attributes */
    volatile uint8_t ctl;    /**< Interrupt control */
} clic_clicint_t;

/**
 * @brief Initialize the CLIC interrupt controller
 */
void clic_init(void);

/**
 * @brief Enable a single interrupt
 *
 * @param   irq     Interrupt number to enable
 */
void clic_enable_interrupt(unsigned irq);

/**
 * @brief Disable a single interrupt
 *
 * @param   irq     Interrupt number to disable
 */
void clic_disable_interrupt(unsigned irq);

/**
 * @brief Set the priority of an interrupt
 *
 * @param   irq         Interrupt number to configure
 * @param   priority    Priority level to configure
 */
void clic_set_priority(unsigned irq, unsigned priority);

/**
 * @brief Set the handler for an interrupt
 *
 * @param   irq     Interrupt number to configure
 * @param   cb      Callback handler to configure
 */
void clic_set_handler(unsigned irq, clic_isr_cb_t cb);

/**
 * @brief CLIC interrupt handler
 *
 * @internal
 *
 * @param   irq     Interrupt number to call the handler for
 */
void clic_isr_handler(uint32_t irq);

//#undef MIP_MSIP
//#undef MIP_MEIP
//#undef MIP_MTIP
//#define MIP_MSIP    CLIC_INT_SFT
//#define MIP_MEIP    CLIC_INT_RESERVED
//#define MIP_MTIP    CLIC_INT_TMR

static inline void write_core_irq_flags(uint32_t flag)
{
    (void)flag;
}

static inline void set_core_irq_flags(uint32_t flag)
{
    clic_enable_interrupt(flag);
}

static inline void clear_core_irq_flags(uint32_t flag)
{
    clic_disable_interrupt(flag);
}

#ifdef __cplusplus
}
#endif

#endif /* CLIC_H */
/** @} */