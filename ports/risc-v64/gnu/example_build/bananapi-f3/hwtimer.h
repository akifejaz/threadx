/***************************************************************************
 * Copyright (c) 2026 10xEngineers
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

#ifndef RISCV_HWTIMER_H
#define RISCV_HWTIMER_H

#include <stdint.h>

/* SpacemiT K1 TIMER (S-mode via SBI ecall)
 *
 * In S-mode the CLINT MMIO registers (mtime / mtimecmp) are protected
 * by PMP and inaccessible.  Timer operations are performed through the
 * SBI legacy interface.
 *
 *   rdtime - pseudo-instruction reading the time CSR (aliased to mtime
 *            by the implementation; accessible from S-mode per Priv
 *            Spec).
 *
 *   SBI legacy set_timer (EID = 0, FID = 0) - programs mtimecmp on
 *            the current hart.  Argument a0 = absolute compare value.
 *            Clears the pending supervisor timer interrupt (sip.STIP)
 *            as a side effect.
 *
 *
 * Timebase frequency (DTS cpus { timebase-frequency = <0x16e3600>; }):
 *   24,000,000 Hz (24 MHz).
 *
 * ThreadX sets the tick rate with TX_TIMER_TICKS_PER_SECOND.
 */
#define TICKNUM_PER_SECOND      24000000UL
#define TICKNUM_PER_TIMER       (TICKNUM_PER_SECOND / TX_TIMER_TICKS_PER_SECOND)

int hwtimer_init(void);
int hwtimer_handler(void);

#endif /* RISCV_HWTIMER_H */
