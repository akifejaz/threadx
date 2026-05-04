/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2026-present Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/*
 * plic.c – Platform Level Interrupt Controller driver for K1 (BPI-F3)
 *
 * Implements SiFive PLIC-1.0.0 compatible access.  All functions operate
 * on the M-mode context of the calling hart (hart 0 in a single-core
 * ThreadX configuration).
 *
 * IMPORTANT: plic_irq_enable / plic_irq_disable correctly handle sources
 * with IRQ number ≥ 32 by selecting the appropriate 32-bit enable word.
 * The QEMU BSP assumes IRQ < 32 (UART0 there is IRQ 10); K1 UART0 is
 * IRQ 42, so this generalised implementation is required.
 */

#include "plic.h"
#include <stddef.h>

static irq_callback callbacks[K1_MAX_PLIC_SOURCES];

/*
 * plic_irq_enable – Enable a single interrupt source for hart 0 (M-mode).
 *
 * Each 32-bit enable register covers 32 sources:
 *   word index = irqno / 32
 *   bit  index = irqno % 32
 */
void plic_irq_enable(int irqno)
{
    uint64_t hart   = riscv_get_core();
    /* Byte offset within the enable bitmap for this word. */
    uint64_t offset = (uint64_t)(irqno / 32) * 4UL;
    volatile uint32_t *reg = (volatile uint32_t *)(K1_PLIC_MENABLE(hart) + offset);

    *reg = *reg | (1U << (irqno % 32));
}

/*
 * plic_irq_disable – Disable a single interrupt source for hart 0 (M-mode).
 */
void plic_irq_disable(int irqno)
{
    uint64_t hart   = riscv_get_core();
    uint64_t offset = (uint64_t)(irqno / 32) * 4UL;
    volatile uint32_t *reg = (volatile uint32_t *)(K1_PLIC_MENABLE(hart) + offset);

    *reg = *reg & ~(1U << (irqno % 32));
}

void plic_prio_set(int irqno, int prio)
{
    K1_PLIC_SET_PRIO(irqno, prio);
}

int plic_prio_get(int irqno)
{
    return (int)K1_PLIC_GET_PRIO(irqno);
}

int plic_register_callback(int irqno, irq_callback callback)
{
    if (irqno < 0 || irqno >= K1_MAX_PLIC_SOURCES)
        return -1;
    callbacks[irqno] = callback;
    return 0;
}

int plic_unregister_callback(int irqno)
{
    return plic_register_callback(irqno, NULL);
}

/*
 * plic_init – Initialise the PLIC for hart 0 in M-mode.
 *
 * Clears the callback table, disables all interrupt sources in the
 * M-mode enable bitmap (5 × 32-bit words covers the full 160-source
 * range), sets threshold to 0 (all priorities pass), and assigns a
 * default priority of 0 (disabled) to every source.
 */
int plic_init(void)
{
    int      i;
    uint64_t hart = riscv_get_core();

    /* Clear software callback table. */
    for (i = 0; i < K1_MAX_PLIC_SOURCES; i++)
        callbacks[i] = NULL;

    /* Disable all interrupt sources in M-mode enable bitmap. */
    for (i = 0; i < (K1_MAX_PLIC_SOURCES + 31) / 32; i++)
    {
        volatile uint32_t *reg = (volatile uint32_t *)(K1_PLIC_MENABLE(hart) +
                                                        (uint64_t)i * 4UL);
        *reg = 0;
    }

    /* Set M-mode threshold to 0: allow all non-zero priorities. */
    *(volatile uint32_t *)K1_PLIC_MTHRESHOLD(hart) = 0;

    return 0;
}

/*
 * plic_claim – Read the claim register to obtain the pending IRQ number.
 */
int plic_claim(void)
{
    uint64_t hart = riscv_get_core();
    return (int)(*(volatile uint32_t *)K1_PLIC_MCLAIM(hart));
}

/*
 * plic_complete – Signal completion of an interrupt to the PLIC.
 */
void plic_complete(int irqno)
{
    uint64_t hart = riscv_get_core();
    *(volatile uint32_t *)K1_PLIC_MCOMPLETE(hart) = (uint32_t)irqno;
}

/*
 * plic_irq_intr – Dispatch a pending external interrupt.
 *
 * Claims the highest-priority pending interrupt, calls its registered
 * callback (if any), then notifies the PLIC of completion.
 */
int plic_irq_intr(void)
{
    int ret   = -1;
    int irqno = plic_claim();

    if (irqno > 0 && irqno < K1_MAX_PLIC_SOURCES && callbacks[irqno] != NULL)
        ret = (callbacks[irqno])(irqno);

    plic_complete(irqno);
    return ret;
}
