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
 * entry.s – Reset / startup code for BananaPi BPI-F3 (SpacemiT K1)
 *
 * The K1 SoC has 8 RISC-V X60 harts (harts 0-3 in cluster 0,
 * harts 4-7 in cluster 1).  Only hart 0 initialises the system and
 * enters ThreadX; all other harts spin in a low-power wait loop.
 *
 * Expected entry state (when loaded as an OpenSBI fw_payload in M-mode):
 *   a0 = hart ID   (preserved for future use, not required by ThreadX)
 *   a1 = FDT ptr   (preserved for future use, not required by ThreadX)
 */

.section .text
.align 4
.global _start
.extern main
.extern _sysstack_start
.extern _bss_start
.extern _bss_end

_start:
    /* ------------------------------------------------------------------ */
    /* Only hart 0 continues; all other harts park in WFI.                */
    /*                                                                     */
    /* a0 = hart ID, set by the calling firmware (OpenSBI fw_payload or   */
    /* U-Boot bootm) per the RISC-V boot protocol.  We use a0 directly   */
    /* instead of csrr mhartid so this code is valid in both M-mode and  */
    /* S-mode contexts.                                                    */
    /* ------------------------------------------------------------------ */
    bne     a0, zero, _hart_park

    /* ------------------------------------------------------------------ */
    /* Store hart ID in tp register.  riscv_get_core() reads tp so that  */
    /* PLIC/CLINT per-hart offsets work without accessing mhartid CSR.   */
    /* ------------------------------------------------------------------ */
    mv      tp, a0

    /* ------------------------------------------------------------------ */
    /* Clear all integer registers (except a0/a1 which carry boot info,  */
    /* and tp/x4 which holds the hart ID saved above).                   */
    /* ------------------------------------------------------------------ */
    li      x3,  0
    /* x4 = tp: skip – holds hart ID from mv tp, a0 above */
    li      x5,  0
    li      x6,  0
    li      x7,  0
    li      x8,  0
    li      x9,  0
    li      x12, 0
    li      x13, 0
    li      x14, 0
    li      x15, 0
    li      x16, 0
    li      x17, 0
    li      x18, 0
    li      x19, 0
    li      x20, 0
    li      x21, 0
    li      x22, 0
    li      x23, 0
    li      x24, 0
    li      x25, 0
    li      x26, 0
    li      x27, 0
    li      x28, 0
    li      x29, 0
    li      x30, 0
    li      x31, 0

    /* ------------------------------------------------------------------ */
    /* Set up the initial stack pointer (grows up from _sysstack_start).  */
    /* ------------------------------------------------------------------ */
    la      t0, _sysstack_start
    li      t1, 0x2000             /* 8 KB stack */
    add     sp, t0, t1

    /* ------------------------------------------------------------------ */
    /* Zero-fill the BSS section.                                          */
    /* ------------------------------------------------------------------ */
    la      t0, _bss_start
    la      t1, _bss_end
_bss_clean_start:
    bgeu    t0, t1, _bss_clean_end
    sb      zero, 0(t0)
    addi    t0, t0, 1
    j       _bss_clean_start
_bss_clean_end:

    /* ------------------------------------------------------------------ */
    /* Jump to C main().                                                   */
    /* ------------------------------------------------------------------ */
    call    main

    /* Should not reach here; halt if main() returns. */
_halt:
    wfi
    j       _halt

_hart_park:
    wfi
    j       _hart_park
