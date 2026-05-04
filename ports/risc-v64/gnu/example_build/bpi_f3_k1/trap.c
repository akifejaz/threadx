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
 * trap.c – Machine-mode trap dispatcher for SpacemiT K1 (BPI-F3)
 *
 * Handles all M-mode traps for a single-hart (hart 0) ThreadX system:
 *
 *   mcause[63]  = 1  → interrupt
 *     mcause[62:0] = 7  → Machine timer interrupt  (CLINT MTIE)
 *     mcause[62:0] = 3  → Machine software interrupt (CLINT MSIE)
 *     mcause[62:0] = 11 → Machine external interrupt (PLIC MEIE)
 *
 *   mcause[63]  = 0  → synchronous exception (unhandled → halt)
 *
 * The trap entry assembly (in tx_initialize_low_level.S) has already
 * called _tx_thread_context_save() before invoking trap_handler(), and
 * will call _tx_thread_context_restore() afterwards.
 */

#include "csr.h"
#include "uart.h"
#include "hwtimer.h"
#include "plic.h"
#include <stdint.h>
#include <tx_port.h>
#include <tx_api.h>

/* Interrupt bit in mcause (bit 63 for RV64). */
#define MCAUSE_INTERRUPT_BIT    (1ULL << 63)

/* M-mode interrupt cause codes (RISC-V Privileged Spec, Table 3.6). */
#define MCAUSE_SOFT_INT         (MCAUSE_INTERRUPT_BIT | 3ULL)
#define MCAUSE_TIMER_INT        (MCAUSE_INTERRUPT_BIT | 7ULL)
#define MCAUSE_EXT_INT          (MCAUSE_INTERRUPT_BIT | 11ULL)

extern void _tx_timer_interrupt(void);

/*
 * trap_handler – C-level trap dispatcher.
 *
 * Called from the assembly trap_entry stub in tx_initialize_low_level.S.
 * Parameters are passed in the standard RISC-V calling convention:
 *   a0 = mcause, a1 = mepc, a2 = mtval
 */
void trap_handler(uintptr_t mcause, uintptr_t mepc, uintptr_t mtval)
{
    (void)mepc;
    (void)mtval;

    if (mcause & MCAUSE_INTERRUPT_BIT)
    {
        if (mcause == MCAUSE_TIMER_INT)
        {
            /* Machine timer interrupt: reload compare register then tick ThreadX. */
            hwtimer_handler();
            _tx_timer_interrupt();
        }
        else if (mcause == MCAUSE_EXT_INT)
        {
            /* Machine external interrupt: dispatch through PLIC. */
            int ret = plic_irq_intr();
            if (ret != 0)
            {
                puts("[IRQ] plic_irq_intr error");
                for (;;)
                    ;
            }
        }
        else if (mcause == MCAUSE_SOFT_INT)
        {
            /* Machine software interrupt: clear MSIP and continue. */
            uint64_t hart = riscv_get_core();
            *(volatile uint32_t *)K1_CLINT_MSIP(hart) = 0;
        }
        else
        {
            puts("[IRQ] Unhandled interrupt");
            for (;;)
                ;
        }
    }
    else
    {
        /* Synchronous exception: print cause and halt. */
        puts("[EXC] Synchronous exception – system halted");
        for (;;)
            ;
    }
}
