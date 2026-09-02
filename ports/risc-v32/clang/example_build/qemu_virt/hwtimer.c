/***************************************************************************
 * Copyright (c) 2026 Quintauris
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

#define CLINT                  (0x02000000L)
#define CLINT_TIME             (CLINT + 0xBFF8)
#define CLINT_TIMECMP(hart_id) (CLINT + 0x4000 + 8 * (hart_id))

#define MTIME_LO            (*(volatile uint32_t *)(CLINT_TIME))
#define MTIME_HI            (*(volatile uint32_t *)(CLINT_TIME + 4))
#define MTIMECMP_LO(hart)   (*(volatile uint32_t *)(CLINT_TIMECMP(hart)))
#define MTIMECMP_HI(hart)   (*(volatile uint32_t *)(CLINT_TIMECMP(hart) + 4))

static uint64_t clint_time_read(void)
{
    uint32_t hi;
    uint32_t lo;

    do
    {
        hi = MTIME_HI;
        lo = MTIME_LO;
    } while (hi != MTIME_HI);

    return ((uint64_t)hi << 32) | lo;
}

static uint64_t clint_timecmp_read(int hart)
{
    uint32_t lo = MTIMECMP_LO(hart);
    uint32_t hi = MTIMECMP_HI(hart);

    return ((uint64_t)hi << 32) | lo;
}

static void clint_timecmp_write(int hart, uint64_t value)
{
    MTIMECMP_LO(hart) = 0xFFFFFFFF;
    MTIMECMP_HI(hart) = (uint32_t)(value >> 32);
    MTIMECMP_LO(hart) = (uint32_t)value;
}

int hwtimer_init(void)
{
    int hart = riscv_get_core();

    clint_timecmp_write(hart, clint_time_read() + TICKNUM_PER_TIMER);
    return 0;
}

int hwtimer_handler(void)
{
    int hart = riscv_get_core();
    uint64_t next = clint_timecmp_read(hart) + TICKNUM_PER_TIMER;
    uint64_t now = clint_time_read();

    if (next <= now)
        next = now + TICKNUM_PER_TIMER;

    clint_timecmp_write(hart, next);
    return 0;
}
