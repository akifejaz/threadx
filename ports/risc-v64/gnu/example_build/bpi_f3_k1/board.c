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
 * board.c – Board-level initialisation for BananaPi BPI-F3 (SpacemiT K1)
 *
 * Called from _tx_initialize_low_level (tx_initialize_low_level.S) before
 * the trap vector is installed and ThreadX is started.
 *
 * Initialisation sequence:
 *   1. plic_init()    – clear PLIC enable bitmap, set threshold to 0
 *   2. uart_init()    – configure UART0 for 115200 8N1 (enables APBC clock)
 *   3. hwtimer_init() – arm the first CLINT timer interrupt
 *
 * Assumptions:
 *   - Executing in Machine mode (M-mode) as an OpenSBI fw_payload
 *   - UART0 pinmux may have been configured by the earlier boot stage;
 *     if loading from cold bare-metal JTAG, pad configuration is done
 *     automatically since K1 ROM configures default pad functions.
 */

#include "plic.h"
#include "uart.h"
#include "hwtimer.h"
#include <stdint.h>
#include <stddef.h>

/* Provide a minimal memset for BSS initialisation helpers. */
void *memset(void *dest, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    size_t         i;

    for (i = 0; i < n; i++)
        d[i] = (unsigned char)c;
    return dest;
}

int board_init(void)
{
    int ret;

    ret = plic_init();
    if (ret != 0)
        return ret;

    ret = uart_init();
    if (ret != 0)
        return ret;

    ret = hwtimer_init();
    if (ret != 0)
        return ret;

    return 0;
}
