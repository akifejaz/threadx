/***************************************************************************
 * Copyright (c) 2026 10xEngineers
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

#include "tx_api.h"
#include "csr.h"
#include "hwtimer.h"

/*
 * SBI legacy set_timer ecall
 *
 * EID (a7) = 0  (SBI_SET_TIMER)
 * a0       = absolute mtime compare value
 *
 * Programs mtimecmp for the current hart and clears sip.STIP.
 */
static inline void sbi_set_timer(uint64_t stime_value)
{
    register uint64_t a0 __asm__("a0") = stime_value;
    register uint64_t a7 __asm__("a7") = 0;  /* SBI_SET_TIMER */
    __asm__ volatile("ecall"
                     : "+r"(a0)
                     : "r"(a7)
                     : "memory");
}

/*
 * Read the free-running mtime counter via the rdtime pseudo-instruction.
 * Accessible from S-mode per RISC-V Priv Spec §10.1 (Zicntr extension).
 */
static inline uint64_t read_time(void)
{
    uint64_t t;
    __asm__ volatile("rdtime %0" : "=r"(t));
    return t;
}

/* Last programmed compare value: SBI offers no read-back, so keep it
   here for the absolute re-arm.  */
static uint64_t next_compare;

int hwtimer_init(void)
{
    next_compare = read_time() + TICKNUM_PER_TIMER;
    sbi_set_timer(next_compare);
    return 0;
}

int hwtimer_handler(void)
{
    /* Absolute re-arm: advance from the previous compare value, so trap
       latency does not accumulate as tick drift; catch up if the next
       compare already passed.  */
    uint64_t now;

    next_compare += TICKNUM_PER_TIMER;
    now = read_time();
    if (next_compare <= now)
        next_compare = now + TICKNUM_PER_TIMER;

    sbi_set_timer(next_compare);
    return 0;
}
