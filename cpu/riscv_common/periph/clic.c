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
 * @brief           RISCV CLIC interrupt controller implementation
 *
 * @author          Koen Zandberg <koen@bergzand.net>
 */

#include <stdint.h>
#include <assert.h>
#include "clic.h"

/* PLIC external ISR function list */
static clic_isr_cb_t _ext_isrs[CLIC_NUM_INTERRUPTS];

void clic_init(void)
{
}

void clic_enable_interrupt(unsigned irq)
{
    CLIC_INT[irq].ie = 1;
    CLIC_INT[irq].attr = 0;
    clic_set_priority(irq, 0xff);
}

void clic_disable_interrupt(unsigned irq)
{
    CLIC_INT[irq].ie = 0;
}

void clic_set_priority(unsigned irq, unsigned priority)
{
    CLIC_INT[irq].ctl = priority;
}

void clic_set_handler(unsigned irq, clic_isr_cb_t cb)
{
    assert(irq < CLIC_NUM_INTERRUPTS);
    _ext_isrs[irq] = cb;
}

void clic_isr_handler(uint32_t irq)
{
    _ext_isrs[irq](irq);
}
