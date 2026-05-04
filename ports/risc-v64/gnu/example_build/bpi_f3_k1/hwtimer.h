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
 * hwtimer.h – CLINT-based hardware timer for SpacemiT K1 (BPI-F3)
 *
 * K1 CLINT (Core Local Interruptor):
 *   Base address : 0xE400_0000   (from mainline Linux DTS, k1.dtsi)
 *   Compatible   : "spacemit,k1-clint", "sifive,clint0"
 *   Size         : 0x0001_0000
 *
 * Standard SiFive CLINT register layout:
 *   MSIP(h)      : CLINT_BASE + h * 4           (software interrupt per hart)
 *   MTIMECMP(h)  : CLINT_BASE + 0x4000 + h * 8  (timer compare per hart)
 *   MTIME        : CLINT_BASE + 0xBFF8           (global 64-bit time counter)
 *
 * The counter increments at the SoC reference frequency:
 *   timebase-frequency = 24 000 000 Hz  (from mainline Linux DTS, k1.dtsi)
 *
 * ThreadX tick rate: 100 Hz  →  interval = 24 000 000 / 100 = 240 000 counts
 */

#ifndef K1_HWTIMER_H
#define K1_HWTIMER_H

#include <stdint.h>

/* CLINT register addresses (K1 / BPI-F3). */
#define K1_CLINT_BASE               0xE4000000UL
#define K1_CLINT_MSIP(hart)         (K1_CLINT_BASE + (uint64_t)(hart) * 4)
#define K1_CLINT_MTIMECMP(hart)     (K1_CLINT_BASE + 0x4000UL + (uint64_t)(hart) * 8)
#define K1_CLINT_MTIME              (K1_CLINT_BASE + 0xBFF8UL)

/* Timebase: 24 MHz.  100 Hz ThreadX tick = 240 000 counts per interval. */
#define K1_TIMEBASE_HZ              24000000UL
#define K1_TX_TICKS_PER_SECOND      100UL
#define K1_TICKNUM_PER_TIMER        (K1_TIMEBASE_HZ / K1_TX_TICKS_PER_SECOND)

int hwtimer_init(void);
int hwtimer_handler(void);

#endif /* K1_HWTIMER_H */
