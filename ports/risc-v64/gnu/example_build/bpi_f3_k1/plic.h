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
 * plic.h – Platform Level Interrupt Controller driver for K1 (BPI-F3)
 *
 * K1 PLIC (Platform Level Interrupt Controller):
 *   Base address : 0xE000_0000   (from mainline Linux DTS, k1.dtsi)
 *   Compatible   : "spacemit,k1-plic", "sifive,plic-1.0.0"
 *   Size         : 0x0400_0000  (64 MB)
 *
 * Standard SiFive PLIC-1.0.0 register layout (M-mode, hart h):
 *   Priority(irq)  : PLIC_BASE + irq * 4             (per-source priority)
 *   Pending        : PLIC_BASE + 0x001000             (pending bits)
 *   Enable(h)      : PLIC_BASE + 0x002000 + h * 0x100 (M-mode enable bitmap)
 *   Threshold(h)   : PLIC_BASE + 0x200000 + h * 0x2000 (M-mode threshold)
 *   Claim(h)       : PLIC_BASE + 0x200004 + h * 0x2000 (M-mode claim/complete)
 *
 * The PLIC supports up to 1023 interrupt sources; K1 uses sources 16–151
 * (sys_int_ap[16..151] from the K1 User Manual, Chapter 7).
 *
 * Selected interrupt source IDs (K1 User Manual, Chapter 7):
 *   42 – UART0_int   (uart0 @ 0xD401_7000)
 *   44 – UART2_int
 *   45 – UART3_int
 *   58 – GPIO_INT_AP
 *   72 – dma_int_ap_non_sec
 */

#ifndef K1_PLIC_H
#define K1_PLIC_H

#include "csr.h"
#include <stdint.h>

/* PLIC base and register layout. */
#define K1_PLIC_BASE                0xE0000000UL

#define K1_PLIC_PRIORITY(irq)       (K1_PLIC_BASE + (uint32_t)(irq) * 4U)
#define K1_PLIC_PENDING             (K1_PLIC_BASE + 0x001000UL)
#define K1_PLIC_MENABLE(hart)       (K1_PLIC_BASE + 0x002000UL + (uint64_t)(hart) * 0x100UL)
#define K1_PLIC_MTHRESHOLD(hart)    (K1_PLIC_BASE + 0x200000UL + (uint64_t)(hart) * 0x2000UL)
#define K1_PLIC_MCLAIM(hart)        (K1_PLIC_BASE + 0x200004UL + (uint64_t)(hart) * 0x2000UL)
#define K1_PLIC_MCOMPLETE(hart)     (K1_PLIC_BASE + 0x200004UL + (uint64_t)(hart) * 0x2000UL)

/* Convenience priority accessors. */
#define K1_PLIC_GET_PRIO(irq)       (*(volatile uint32_t *)K1_PLIC_PRIORITY(irq))
#define K1_PLIC_SET_PRIO(irq, p)    (*(volatile uint32_t *)K1_PLIC_PRIORITY(irq) = (uint32_t)(p))

/* K1 has 151 external interrupt sources (sys_int_ap[0..151]).
 * Round up to the next multiple of 32 for array sizing.               */
#define K1_MAX_PLIC_SOURCES         160

typedef int (*irq_callback)(int irqno);

void plic_irq_enable(int irqno);
void plic_irq_disable(int irqno);
int  plic_prio_get(int irqno);
void plic_prio_set(int irqno, int prio);
int  plic_register_callback(int irqno, irq_callback callback);
int  plic_unregister_callback(int irqno);
int  plic_init(void);
int  plic_claim(void);
void plic_complete(int irqno);
int  plic_irq_intr(void);

#endif /* K1_PLIC_H */
