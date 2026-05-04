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
 * uart.h – UART0 driver for SpacemiT K1 (BPI-F3)
 *
 * K1 UART0 hardware:
 *   Base address  : 0xD401_7000  (K1 User Manual, Chapter 6 address map)
 *   Compatible    : "spacemit,k1-uart", "intel,xscale-uart"
 *   PLIC IRQ      : 42           (K1 User Manual, Chapter 7, sys_int_ap[42])
 *   reg-shift     : 2            (registers at 4-byte stride)
 *   reg-io-width  : 4            (32-bit read/write access)
 *
 * Clock:
 *   Source        : APBC (APB Bus Clock Unit) at 0xD401_5000
 *   UART0 CLK_RST : APBC_BASE + 0x00
 *   Functional clk: 14.7456 MHz (FNCLKSEL = 3'b001)
 *   For 115200 baud: DLL = 8, DLH = 0
 *
 * Key XScale/PXA vs. standard 16550 difference:
 *   IER bit 6 (UUE) – Unit Enable – must be SET for the UART to operate.
 *   Clearing IER entirely would disable the UART unit itself.
 */

#ifndef K1_UART_H
#define K1_UART_H

/* UART0 base address (K1 User Manual, Chapter 6 – Address Mapping). */
#define K1_UART0_BASE       0xD4017000UL

/* PLIC source ID for UART0 (K1 User Manual, Chapter 7 – sys_int_ap[42]). */
#define K1_UART0_IRQ        42

/* Macro to redirect puts() to the BSP UART driver. */
#define puts                uart_puts

int  uart_init(void);
int  uart_putc(int ch);
int  uart_puts(const char *str);

#endif /* K1_UART_H */
