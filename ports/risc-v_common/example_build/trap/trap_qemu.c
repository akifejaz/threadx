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

/* Trap handler for QEMU virt machine (RV32 and RV64).
 *
 * mcause constants use __riscv_xlen to resolve correctly for both ISAs:
 *   RV32: interrupt bit = bit 31 (0x80000000)
 *   RV64: interrupt bit = bit 63 (0x8000000000000000)
 */

#include "csr.h"
#include <stdint.h>
#include "uart.h"
#include "hwtimer.h"
#include "plic.h"

#define MCAUSE_INT_BIT          ((uintptr_t)1 << (__riscv_xlen - 1))

#define OS_IS_INTERRUPT(mcause) ((mcause) & MCAUSE_INT_BIT)
#define OS_IS_TICK_INT(mcause)  ((mcause) == (MCAUSE_INT_BIT | 7u))
#define OS_IS_SOFT_INT(mcause)  ((mcause) == (MCAUSE_INT_BIT | 3u))
#define OS_IS_EXT_INT(mcause)   ((mcause) == (MCAUSE_INT_BIT | 11u))

extern void _tx_timer_interrupt(void);

static void print_str(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

static void print_hex(uintptr_t val)
{
    const char digits[] = "0123456789ABCDEF";
    uart_putc('0');
    uart_putc('x');
    for (int i = (int)(sizeof(uintptr_t) * 2) - 1; i >= 0; i--)
    {
        int d = (val >> (i * 4)) & 0xF;
        uart_putc(digits[d]);
    }
}

void trap_handler(uintptr_t mcause, uintptr_t mepc, uintptr_t mtval)
{
    if (OS_IS_INTERRUPT(mcause))
    {
        if (OS_IS_TICK_INT(mcause))
        {
            hwtimer_handler();
            _tx_timer_interrupt();
        }
        else if (OS_IS_EXT_INT(mcause))
        {
            int ret = plic_irq_intr();
            if (ret)
            {
                puts("[INTERRUPT]: handler irq error!");
                while (1) ;
            }
        }
        else
        {
            puts("[INTERRUPT]: now can't deal with the interrupt!");
            while (1) ;
        }
    }
    else
    {
        /* The BSP has no execution service or debugger route for exceptions. */
        print_str("[EXCEPTION] mcause=");
        print_hex(mcause);
        print_str(" mepc=");
        print_hex(mepc);
        print_str(" mtval=");
        print_hex(mtval);
        uart_putc('\n');
        while (1) ;
    }
}
