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


#ifndef RISCV_CSR_H
#define RISCV_CSR_H


/* Machine Status Register, mstatus  */
#define MSTATUS_MPP_MASK    (3L << 11)   /* previous mode.  */
#define MSTATUS_MPP_M       (3L << 11)
#define MSTATUS_MPP_S       (1L << 11)
#define MSTATUS_MPP_U       (0L << 11)
#define MSTATUS_MIE         (1L << 3)    /* machine-mode interrupt enable.  */
#define MSTATUS_MPIE        (1L << 7)
/* Note: MSTATUS_FS / MSTATUS_VS are the "Initial" (01) write encodings of
   the 2-bit FS (bits 14:13) / VS (bits 10:9) fields, not field masks
   (the masks are 3L << 13 / 3L << 9).  They are correct for a csrrs
   enable from Off, but wrong as a mask or as a Dirty test.  */
#define MSTATUS_FS          (1L << 13)   /* floating-point status = FS "Initial".  */
#define MSTATUS_VS          (1L << 9)    /* vector status (RVV) = VS "Initial".  */

/* Machine-mode Interrupt Enable  */
#define MIE_MTIE            (1L << 7)
#define MIE_MSIE            (1L << 3)
#define MIE_MEIE            (1L << 11)
#define MIE_STIE            (1L << 5)    /* supervisor timer  */
#define MIE_SSIE            (1L << 1)
#define MIE_SEIE            (1L << 9)

/* Supervisor Status Register, sstatus  */
#define SSTATUS_SPP         (1L << 8)    /* Previous mode, 1=Supervisor, 0=User  */
#define SSTATUS_SPIE        (1L << 5)    /* Supervisor Previous Interrupt Enable  */
#define SSTATUS_SIE         (1L << 1)    /* Supervisor Interrupt Enable  */

/* Supervisor Interrupt Enable  */
#define SIE_SEIE            (1L << 9)    /* external  */
#define SIE_STIE            (1L << 5)    /* timer  */
#define SIE_SSIE            (1L << 1)    /* software  */

#ifndef __ASSEMBLER__

#include <stdint.h>

/* All CSR accessor functions use uintptr_t so this header works for both
   RV32 (uintptr_t = uint32_t) and RV64 (uintptr_t = uint64_t).  */

static inline uintptr_t riscv_get_core(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, mhartid" : "=r" (x));
    return x;
}

static inline uintptr_t riscv_get_mstatus(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, mstatus" : "=r" (x));
    return x;
}

static inline void riscv_writ_mstatus(uintptr_t x)
{
    __asm__ volatile("csrw mstatus, %0" : : "r" (x));
}

static inline void riscv_writ_mepc(uintptr_t x)
{
    __asm__ volatile("csrw mepc, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_sstatus(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, sstatus" : "=r" (x));
    return x;
}

static inline void riscv_writ_sstatus(uintptr_t x)
{
    __asm__ volatile("csrw sstatus, %0" : : "r" (x));
}

/* Supervisor Interrupt Pending  */
static inline uintptr_t riscv_get_sip(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, sip" : "=r" (x));
    return x;
}

static inline void riscv_writ_sip(uintptr_t x)
{
    __asm__ volatile("csrw sip, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_sie(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, sie" : "=r" (x));
    return x;
}

static inline void riscv_writ_sie(uintptr_t x)
{
    __asm__ volatile("csrw sie, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_mie(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, mie" : "=r" (x));
    return x;
}

static inline void riscv_writ_mie(uintptr_t x)
{
    __asm__ volatile("csrw mie, %0" : : "r" (x));
}

/* Supervisor exception program counter  */
static inline void riscv_writ_sepc(uintptr_t x)
{
    __asm__ volatile("csrw sepc, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_sepc(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, sepc" : "=r" (x));
    return x;
}

/* Machine Exception Delegation  */
static inline uintptr_t riscv_get_medeleg(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, medeleg" : "=r" (x));
    return x;
}

static inline void riscv_writ_medeleg(uintptr_t x)
{
    __asm__ volatile("csrw medeleg, %0" : : "r" (x));
}

/* Machine Interrupt Delegation  */
static inline uintptr_t riscv_get_mideleg(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, mideleg" : "=r" (x));
    return x;
}

static inline void riscv_writ_mideleg(uintptr_t x)
{
    __asm__ volatile("csrw mideleg, %0" : : "r" (x));
}

/* Supervisor Trap-Vector Base Address  */
static inline void riscv_writ_stvec(uintptr_t x)
{
    __asm__ volatile("csrw stvec, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_stvec(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, stvec" : "=r" (x));
    return x;
}

/* Supervisor Timer Comparison Register  */
/* RV32 caveat: stimecmp is a 64-bit CSR; these uintptr_t accessors reach
   only the low half on RV32 (no stimecmph twin exists here).  */
static inline uintptr_t riscv_get_stimecmp(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, 0x14d" : "=r" (x));
    return x;
}

static inline void riscv_writ_stimecmp(uintptr_t x)
{
    __asm__ volatile("csrw 0x14d, %0" : : "r" (x));
}

/* Machine Environment Configuration Register  */
/* RV32 caveat: menvcfg is a 64-bit CSR; these uintptr_t accessors reach
   only the low half on RV32 (no menvcfgh twin exists here).  */
static inline uintptr_t riscv_get_menvcfg(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, 0x30a" : "=r" (x));
    return x;
}

static inline void riscv_writ_menvcfg(uintptr_t x)
{
    __asm__ volatile("csrw 0x30a, %0" : : "r" (x));
}

/* Physical Memory Protection  */
static inline void riscv_writ_pmpcfg0(uintptr_t x)
{
    __asm__ volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void riscv_writ_pmpaddr0(uintptr_t x)
{
    __asm__ volatile("csrw pmpaddr0, %0" : : "r" (x));
}

/* Supervisor address translation and protection  */
static inline void riscv_writ_satp(uintptr_t x)
{
    __asm__ volatile("csrw satp, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_satp(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, satp" : "=r" (x));
    return x;
}

/* Supervisor Trap Cause  */
static inline uintptr_t riscv_get_scause(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, scause" : "=r" (x));
    return x;
}

/* Supervisor Trap Value  */
static inline uintptr_t riscv_get_stval(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, stval" : "=r" (x));
    return x;
}

/* Machine-mode Counter-Enable  */
static inline void riscv_writ_mcounteren(uintptr_t x)
{
    __asm__ volatile("csrw mcounteren, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_mcounteren(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, mcounteren" : "=r" (x));
    return x;
}

/* Caveat: the time CSR read traps unless the current mode is granted
   access (mcounteren/scounteren TM gating); on RV32 it returns only the
   low 32 bits.  */
static inline uintptr_t riscv_get_time(void)
{
    uintptr_t x;
    __asm__ volatile("csrr %0, time" : "=r" (x));
    return x;
}

static inline void riscv_sintr_on(void)
{
    uintptr_t sstatus = riscv_get_sstatus();
    sstatus |= SSTATUS_SIE;
    riscv_writ_sstatus(sstatus);
}

static inline void riscv_sintr_off(void)
{
    uintptr_t sstatus = riscv_get_sstatus();
    sstatus &= (~(uintptr_t)SSTATUS_SIE);
    riscv_writ_sstatus(sstatus);
}

static inline int riscv_sintr_get(void)
{
    uintptr_t x = riscv_get_sstatus();
    return (x & SSTATUS_SIE) != 0;
}

static inline void riscv_sintr_restore(int x)
{
    if (x)
        riscv_sintr_on();
    else
        riscv_sintr_off();
}

static inline void riscv_mintr_on(void)
{
    uintptr_t mstatus = riscv_get_mstatus();
    mstatus |= MSTATUS_MIE;
    riscv_writ_mstatus(mstatus);
}

static inline void riscv_mintr_off(void)
{
    uintptr_t mstatus = riscv_get_mstatus();
    mstatus &= (~(uintptr_t)MSTATUS_MIE);
    riscv_writ_mstatus(mstatus);
}

static inline int riscv_mintr_get(void)
{
    uintptr_t x = riscv_get_mstatus();
    return (x & MSTATUS_MIE) != 0;
}

static inline void riscv_mintr_restore(int x)
{
    if (x)
        riscv_mintr_on();
    else
        riscv_mintr_off();
}

static inline uintptr_t riscv_get_sp(void)
{
    uintptr_t x;
    __asm__ volatile("mv %0, sp" : "=r" (x));
    return x;
}

/* Thread pointer (tp), used by some ports for hart-local storage.  */
static inline uintptr_t riscv_get_tp(void)
{
    uintptr_t x;
    __asm__ volatile("mv %0, tp" : "=r" (x));
    return x;
}

static inline void riscv_writ_tp(uintptr_t x)
{
    __asm__ volatile("mv tp, %0" : : "r" (x));
}

static inline uintptr_t riscv_get_ra(void)
{
    uintptr_t x;
    __asm__ volatile("mv %0, ra" : "=r" (x));
    return x;
}

/* Flush all TLB entries.  */
static inline void sfence_vma(void)
{
    __asm__ volatile("sfence.vma zero, zero");
}

#endif /* __ASSEMBLER__ */

#endif /* RISCV_CSR_H */
