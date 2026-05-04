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

#include "csr.h"

    .section .text
    .align 4

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    trap_entry                           RISC-V64/GNU / BPI-F3 K1      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Eclipse ThreadX contributors                                        */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    Machine-mode trap entry stub for the SpacemiT K1 SoC.              */
/*    Allocates a register save frame on the stack, stores RA, calls     */
/*    _tx_thread_context_save, dispatches to the C trap_handler, then    */
/*    transfers control to _tx_thread_context_restore (which never       */
/*    returns).                                                           */
/*                                                                        */
/*    Frame sizes (bytes):                                                */
/*      With FP  (rv64gc / __riscv_float_abi_double): 65 * 8 = 520      */
/*      Without FP:                                   32 * 8 = 256      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None (entered from hardware trap mechanism)                         */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None (does not return – resumes via context restore)               */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _tx_thread_context_save                                             */
/*    trap_handler                                                        */
/*    _tx_thread_context_restore                                          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Hardware trap mechanism (mtvec)                                     */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  2026-04-29      Eclipse ThreadX contributors  BPI-F3 K1 BSP          */
/*                                                                        */
/**************************************************************************/

    .global  trap_entry
    .extern  trap_handler
    .extern  _tx_thread_context_restore

trap_entry:
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, -520                /* 65 * 8 bytes (GP + FP registers) */
#else
    addi    sp, sp, -256                /* 32 * 8 bytes (GP registers only) */
#endif

    /* RA must be saved before calling _tx_thread_context_save because the
       call will overwrite it.  Slot 28*8 = 224 matches the port convention. */
    sd      x1, 224(sp)

    call    _tx_thread_context_save

    /* Read trap cause, faulting PC, and bad address into a0/a1/a2 to pass
       as arguments to the C trap_handler function.                         */
    csrr    a0, mcause
    csrr    a1, mepc
    csrr    a2, mtval

    /* Save/restore RA around the C call so it is not clobbered. */
    addi    sp, sp, -8
    sd      ra, 0(sp)
    call    trap_handler
    ld      ra, 0(sp)
    addi    sp, sp, 8

    /* _tx_thread_context_restore resumes the appropriate thread and never
       returns to this stub.                                                */
    call    _tx_thread_context_restore

    /* Unreachable – halt if context restore somehow returns. */
_trap_halt:
    wfi
    j       _trap_halt


    .section .text

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_initialize_low_level              RISC-V64/GNU / BPI-F3 K1     */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Scott Larson, Microsoft Corporation                                 */
/*    Eclipse ThreadX contributors (K1 BSP adaptation)                   */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    Low-level ThreadX initialisation for the SpacemiT K1 SoC.          */
/*                                                                        */
/*    Actions performed:                                                  */
/*      1. Save system (boot) stack pointer for ISR use.                  */
/*      2. Record first free memory address for ThreadX heap.             */
/*      3. Initialise FP CSR if the FP extension is present.             */
/*      4. Configure M-mode interrupt enable bits in mstatus and mie.    */
/*      5. Call board_init() (PLIC, UART, CLINT timer).                  */
/*      6. Install the trap entry vector in mtvec (direct mode).         */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    board_init                                                          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _tx_initialize_kernel_enter           ThreadX entry function        */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  03-08-2023      Scott Larson            Initial Version 6.2.1         */
/*  2026-04-29      Eclipse contributors    K1 BSP (board_init, mtvec)   */
/*                                                                        */
/**************************************************************************/

    .global  _tx_initialize_low_level
    .weak    _tx_initialize_low_level
    .extern  _end
    .extern  board_init

_tx_initialize_low_level:

    /* ------------------------------------------------------------------ */
    /* 1. Save the system stack pointer so ISR stacks can be managed.     */
    /* ------------------------------------------------------------------ */
    la      t0, _tx_thread_system_stack_ptr
    sd      sp, 0(t0)

    /* ------------------------------------------------------------------ */
    /* 2. Record the first free memory address for the ThreadX heap.      */
    /* ------------------------------------------------------------------ */
    la      t0, _end
    la      t1, _tx_initialize_unused_memory
    sd      t0, 0(t1)

    /* ------------------------------------------------------------------ */
    /* 3. Initialise FP if the FP extension is compiled in.               */
    /* ------------------------------------------------------------------ */
#ifdef __riscv_flen
    li      t0, MSTATUS_FS
    csrrs   zero, mstatus, t0          /* Set FS bits – mark FP as initial */
    fscsr   x0                         /* Clear FP CSR */
#endif

    /* ------------------------------------------------------------------ */
    /* 4. Configure M-mode interrupt enables.                              */
    /*                                                                     */
    /*    mstatus:                                                         */
    /*      Clear MIE  (bit 3) – global interrupt disable during init     */
    /*      Set   MPP  (bits 12:11) = 11 – return to M-mode from traps   */
    /*      Set   MPIE (bit 7)  – re-enable interrupts after mret         */
    /*                                                                     */
    /*    mie:                                                             */
    /*      MTIE (bit 7)  – Machine Timer Interrupt Enable                */
    /*      MSIE (bit 3)  – Machine Software Interrupt Enable             */
    /*      MEIE (bit 11) – Machine External Interrupt Enable             */
    /* ------------------------------------------------------------------ */
    li      t0, MSTATUS_MIE
    csrrc   zero, mstatus, t0              /* Clear MIE – disable during init */

    li      t0, (MSTATUS_MPP_M | MSTATUS_MPIE)
    csrrs   zero, mstatus, t0             /* Set MPP=M-mode, MPIE=1 */

    li      t0, (MIE_MTIE | MIE_MSIE | MIE_MEIE)
    csrrs   zero, mie, t0                  /* Enable timer, soft, external IRQs */

    /* ------------------------------------------------------------------ */
    /* 5. Call board_init() – initialises PLIC, UART0, and CLINT timer.  */
    /* ------------------------------------------------------------------ */
    addi    sp, sp, -8
    sd      ra, 0(sp)
    call    board_init
    ld      ra, 0(sp)
    addi    sp, sp, 8

    /* ------------------------------------------------------------------ */
    /* 6. Install the trap vector in mtvec (direct mode, bit[1:0] = 00). */
    /* ------------------------------------------------------------------ */
    la      t0, trap_entry
    csrw    mtvec, t0

    ret
