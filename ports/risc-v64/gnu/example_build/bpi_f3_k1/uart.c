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
 * uart.c – UART0 driver for SpacemiT K1 (BPI-F3)
 *
 * The K1 UART is "intel,xscale-uart" (PXA-style) with 32-bit wide
 * registers on a 4-byte stride (reg-shift=2, reg-io-width=4).
 *
 * Standard 16550A registers are accessed as follows:
 *   Logical register index N → physical address = K1_UART0_BASE + (N << 2)
 *
 * Critical XScale/PXA difference from plain 16550:
 *   IER bit 6 (0x40) = UUE (Unit Enable).  This bit MUST be set for the
 *   UART unit to be active.  Clearing IER to 0 would disable the UART.
 *   We therefore always OR in UART_IER_UUE when touching IER.
 *
 * Baud rate:
 *   Functional clock = 14.7456 MHz (FNCLKSEL = 3'b001 in APBC register).
 *   115200 baud → divisor = 14745600 / (16 × 115200) = 8
 *   → DLL = 8, DLH = 0
 *
 * APBC clock register:
 *   Address : 0xD401_5000 + 0x00 (UART0)
 *   Value   : 0x13  (FNCLKSEL=001 → bits[6:4]=0b001=0x10,
 *                    FNCLK=1 → bit[1]=0x02, APBCLK=1 → bit[0]=0x01,
 *                    RST=0 → bit[2]=0 de-asserted)
 */

#include "uart.h"
#include "plic.h"
#include "csr.h"
#include <stdint.h>

/* -------------------------------------------------------------------------
 * APBC clock controller (K1 User Manual, Chapter 9 – Top System).
 * APBC_BASE + 0x00 = UART0 clock/reset control register.
 * -------------------------------------------------------------------------*/
#define K1_APBC_BASE                0xD4015000UL
#define K1_APBC_UART0_CLK_RST       (K1_APBC_BASE + 0x00UL)

/*
 * APBC_UART0_CLK_RST bit fields (K1 User Manual §9.2.4.3.1):
 *   [6:4] FNCLKSEL : 001 → 14.7456 MHz functional clock
 *   [2]   RST      : 1 → assert reset, 0 → deassert (operating)
 *   [1]   FNCLK    : 1 → functional clock enable
 *   [0]   APBCLK   : 1 → APB bus clock enable
 *
 * Write 0x13 = 0b0001_0011 → FNCLKSEL=001, RST=0, FNCLK=1, APBCLK=1
 */
#define K1_APBC_UART0_INIT_VAL      0x13U

/* -------------------------------------------------------------------------
 * 16550A register indices (logical; physical = base + index*4).
 * Plain short names to avoid clash with per-register bit-field macros.
 * -------------------------------------------------------------------------*/
#define RBR     0   /* Receive Buffer Register  (DLAB=0, read) */
#define THR     0   /* Transmit Holding Register (DLAB=0, write) */
#define DLL     0   /* Divisor Latch Low         (DLAB=1) */
#define IER     1   /* Interrupt Enable Register (DLAB=0) */
#define DLH     1   /* Divisor Latch High        (DLAB=1) */
#define FCR     2   /* FIFO Control Register     (write) */
#define IIR     2   /* Interrupt ID Register     (read) */
#define LCR     3   /* Line Control Register */
#define MCR     4   /* Modem Control Register */
#define LSR     5   /* Line Status Register */
#define MSR     6   /* Modem Status Register */

/* IER bits. */
#define UART_IER_RX_ENABLE  (1U << 0)   /* Received data available */
#define UART_IER_TX_ENABLE  (1U << 1)   /* Transmitter holding empty */
#define UART_IER_UUE        (1U << 6)   /* XScale/PXA Unit Enable (must stay set) */

/* FCR bits. */
#define UART_FCR_FIFO_ENABLE    (1U << 0)
#define UART_FCR_RX_FIFO_RESET  (1U << 1)
#define UART_FCR_TX_FIFO_RESET  (1U << 2)
#define UART_FCR_TRIGGER_4      (1U << 6)   /* RX trigger: 4 bytes */

/* LCR bits. */
#define UART_LCR_WLEN8      (3U << 0)   /* 8-bit word length */
#define UART_LCR_STOP1      (0U << 2)   /* 1 stop bit */
#define UART_LCR_NO_PARITY  (0U << 3)   /* no parity */
#define UART_LCR_DLAB       (1U << 7)   /* Divisor Latch Access Bit */

/* LSR bits. */
#define UART_LSR_DATA_READY (1U << 0)   /* Received byte available */
#define UART_LSR_THRE       (1U << 5)   /* Transmitter Holding Register Empty */

/* Baud rate divisor for 115200 baud with 14.7456 MHz clock.
 * divisor = 14745600 / (16 × 115200) = 8                              */
#define UART_DLL_115200     8U
#define UART_DLH_115200     0U

/* -------------------------------------------------------------------------
 * Register access helpers (32-bit, 4-byte stride).
 * -------------------------------------------------------------------------*/
static inline volatile uint32_t *uart_reg(unsigned int index)
{
    return (volatile uint32_t *)(K1_UART0_BASE + ((uint32_t)index << 2));
}

#define ReadReg(reg)        (*uart_reg(reg))
#define WriteReg(reg, val)  (*uart_reg(reg) = (uint32_t)(val))

/* -------------------------------------------------------------------------
 * uart_init – Configure UART0 for 115200 8N1 polled I/O.
 * -------------------------------------------------------------------------*/
int uart_init(void)
{
    /* Step 1: Enable UART0 clocks and de-assert reset via APBC.
     *         APBC_UART0_CLK_RST = 0x13:
     *           FNCLKSEL = 001 (14.7456 MHz), RST = 0, FNCLK = 1, APBCLK = 1
     */
    *(volatile uint32_t *)K1_APBC_UART0_CLK_RST = K1_APBC_UART0_INIT_VAL;

    /* Step 2: Access divisor latches (DLAB = 1) and load divisor. */
    WriteReg(LCR, UART_LCR_DLAB | UART_LCR_WLEN8);
    WriteReg(DLL, UART_DLL_115200);
    WriteReg(DLH, UART_DLH_115200);

    /* Step 3: Switch back to normal operation (DLAB = 0), 8N1. */
    WriteReg(LCR, UART_LCR_WLEN8 | UART_LCR_STOP1 | UART_LCR_NO_PARITY);

    /* Step 4: Enable and flush FIFOs. */
    WriteReg(FCR, UART_FCR_FIFO_ENABLE |
                  UART_FCR_RX_FIFO_RESET |
                  UART_FCR_TX_FIFO_RESET |
                  UART_FCR_TRIGGER_4);

    /* Step 5: Enable the UART unit (UUE) but disable all interrupt sources.
     *         UUE must remain set or the UART becomes inactive.
     */
    WriteReg(IER, UART_IER_UUE);

    /* Step 6: Enable UART0 IRQ in the PLIC (IRQ 42) and set its priority. */
    plic_irq_enable(K1_UART0_IRQ);
    plic_prio_set(K1_UART0_IRQ, 1);

    puts("[UART0] Init done – BPI-F3 K1 115200 8N1");
    return 0;
}

/* -------------------------------------------------------------------------
 * uart_putc_nolock – Transmit one character (busy-wait, no lock).
 * -------------------------------------------------------------------------*/
static inline void uart_putc_nolock(int ch)
{
    while ((ReadReg(LSR) & UART_LSR_THRE) == 0)
        ;
    WriteReg(THR, (uint32_t)ch);
}

/* -------------------------------------------------------------------------
 * uart_putc – Transmit one character with interrupt-safe critical section.
 * -------------------------------------------------------------------------*/
int uart_putc(int ch)
{
    int saved = riscv_mintr_get();
    riscv_mintr_off();
    uart_putc_nolock(ch);
    riscv_mintr_restore(saved);
    return 1;
}

/* -------------------------------------------------------------------------
 * uart_puts – Transmit a NUL-terminated string followed by a newline.
 * -------------------------------------------------------------------------*/
int uart_puts(const char *str)
{
    int i;
    int saved = riscv_mintr_get();
    riscv_mintr_off();
    for (i = 0; str[i] != '\0'; i++)
        uart_putc_nolock(str[i]);
    uart_putc_nolock('\n');
    riscv_mintr_restore(saved);
    return i;
}
