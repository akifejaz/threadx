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
 * hwtimer.c – CLINT-based timer driver for SpacemiT K1 (BPI-F3)
 *
 * Programs MTIMECMP for hart 0 to fire at a fixed interval, producing the
 * 100 Hz tick required by ThreadX.  The driver must run in Machine mode
 * (M-mode) because it accesses CLINT registers directly.
 */

#include "hwtimer.h"
#include "csr.h"

/*
 * hwtimer_init – Arm the first timer interrupt.
 *
 * Reads the current MTIME value and sets MTIMECMP(hart 0) to
 * MTIME + K1_TICKNUM_PER_TIMER.  The MTIE bit in mie must have been
 * enabled beforehand (done in tx_initialize_low_level.S).
 */
int hwtimer_init(void)
{
    uint64_t hart = riscv_get_core();
    uint64_t now  = *(volatile uint64_t *)K1_CLINT_MTIME;

    *(volatile uint64_t *)K1_CLINT_MTIMECMP(hart) = now + K1_TICKNUM_PER_TIMER;
    return 0;
}

/*
 * hwtimer_handler – Reload the compare register for the next tick.
 *
 * Called from the M-mode trap handler each time a timer interrupt fires.
 * Advances MTIMECMP by exactly one tick interval to keep the period stable.
 */
int hwtimer_handler(void)
{
    uint64_t hart = riscv_get_core();
    uint64_t now  = *(volatile uint64_t *)K1_CLINT_MTIME;

    *(volatile uint64_t *)K1_CLINT_MTIMECMP(hart) = now + K1_TICKNUM_PER_TIMER;
    return 0;
}
