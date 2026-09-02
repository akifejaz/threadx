/***************************************************************************
 * Copyright (c) 2025 10xEngineers
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** ThreadX Component                                                     */
/**                                                                       */
/**   Thread                                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


    .section .text

/* Frame layouts (stack grows down, all totals mod 16 = 0):
   Trap frame, FP build: integer slots 0-31 at i*4 (0-127; slot 31 is pad),
   f0-f31 as doubles at 128 + i*8 (128-383), fcsr at 384, pad to 400.
   Trap frame, SOFT_FLOAT: integer slots 0-31 at i*4 (128 total).
   Solicited frame, FP build: slots 0-14 at i*4 (type, s11-s1, s0, ra,
   mstatus), fs0-fs11 as doubles at 64 + i*8 (64-159), fcsr at 160,
   pad to 176.  Solicited, SOFT_FLOAT: slots 0-14, top-padded to 128.  */
#if defined(__riscv_float_abi_double)
    .equ    TX_TRAP_FRAME_SIZE, 400
    .equ    TX_SOL_FRAME_SIZE, 176
#else
    .equ    TX_TRAP_FRAME_SIZE, 128
    .equ    TX_SOL_FRAME_SIZE, 128
#endif

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_schedule                                RISC-V32/GNU     */
/*                                                           6.4.x        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Akif Ejaz, 10xEngineers                                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function waits for a thread control block pointer to appear in */
/*    the _tx_thread_execute_ptr variable.  Once a thread pointer appears */
/*    in the variable, the corresponding thread is resumed.               */
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
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _tx_initialize_kernel_enter          ThreadX entry function         */
/*    _tx_thread_system_return             Return to system from thread   */
/*    _tx_thread_context_restore           Restore thread's context       */
/*                                                                        */
/**************************************************************************/
/* VOID   _tx_thread_schedule(VOID)
{  */
    .global  _tx_thread_schedule
_tx_thread_schedule:

    /* Enable interrupts.  */

    csrsi   mstatus, 0x08                               // Enable interrupts (MIE bit 3)

    /* Wait for a thread to execute.  */
    /* do
    {  */
_tx_thread_schedule_loop:

    la      t0, _tx_thread_execute_ptr                  // Pickup address of execute ptr
    lw      t1, 0(t0)                                   // Pickup execute pointer
    bnez    t1, _tx_thread_ready_to_run                 // If non-NULL, a thread is ready to run

#ifndef TX_NO_WFI
    wfi                                                 // Wait for interrupt
#endif
    j       _tx_thread_schedule_loop                    // Check again

    /* }
     while (_tx_thread_execute_ptr == TX_NULL);  */

_tx_thread_ready_to_run:

    /* At this point, t1 contains the pointer to the thread to execute.
       Lockout interrupts.  */

    csrci   mstatus, 0x08                               // Disable interrupts (MIE bit 3)

    /* Check _tx_thread_execute_ptr again, in case an interrupt occurred
       between the check and the disable.  */

    lw      t1, 0(t0)                                   // Pickup execute pointer
    beqz    t1, _tx_thread_schedule_loop                // If NULL, go back to wait loop

    /* Yes! We have a thread to execute.  */
    /* _tx_thread_current_ptr =  _tx_thread_execute_ptr;  */

    la      t0, _tx_thread_current_ptr                  // Pickup address of current thread
    sw      t1, 0(t0)                                   // Setup current thread pointer

    /* Increment the run count for this thread.  */
    /* _tx_thread_current_ptr -> tx_thread_run_count++;  */

    lw      t2, 4(t1)                                   // Pickup run count
    addi    t2, t2, 1                                   // Increment run count
    sw      t2, 4(t1)                                   // Store run count

    /* Setup time-slice values.  */
    /* _tx_timer_time_slice =  _tx_thread_current_ptr -> tx_thread_time_slice;  */

    lw      t2, 24(t1)                                  // Pickup thread time-slice
    la      t3, _tx_timer_time_slice                    // Pickup address of time-slice
    sw      t2, 0(t3)                                   // Setup time-slice

    /* Call the thread execution enter function if enabled.  */
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY

    call    _tx_execution_thread_enter                  // Call the thread execution enter function
#endif

    /* Switch to the thread's stack.  */
    /* sp =  _tx_thread_current_ptr -> tx_thread_stack_ptr;  */

    lw      sp, 8(t1)                                   // Switch to thread stack

    /* Determine the type of stack frame.  */
    /* if (*sp)
    {  */

    lw      t0, 0(sp)                                   // Pickup stack type
    beqz    t0, _tx_thread_solicited_return             // If 0, solicited return

    /* Recover floating point registers only if saved mstatus.FS was not Off.  */
#if defined(__riscv_float_abi_double)
    lw      t1, 29*4(sp)                                // Pickup saved mstatus
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_schedule_skip_fp_restore     // Skip if FS was Off
    fld     f0,  128(sp)                                // Recover ft0
    fld     f1,  136(sp)                                // Recover ft1
    fld     f2,  144(sp)                                // Recover ft2
    fld     f3,  152(sp)                                // Recover ft3
    fld     f4,  160(sp)                                // Recover ft4
    fld     f5,  168(sp)                                // Recover ft5
    fld     f6,  176(sp)                                // Recover ft6
    fld     f7,  184(sp)                                // Recover ft7
    fld     f8,  192(sp)                                // Recover fs0
    fld     f9,  200(sp)                                // Recover fs1
    fld     f10, 208(sp)                                // Recover fa0
    fld     f11, 216(sp)                                // Recover fa1
    fld     f12, 224(sp)                                // Recover fa2
    fld     f13, 232(sp)                                // Recover fa3
    fld     f14, 240(sp)                                // Recover fa4
    fld     f15, 248(sp)                                // Recover fa5
    fld     f16, 256(sp)                                // Recover fa6
    fld     f17, 264(sp)                                // Recover fa7
    fld     f18, 272(sp)                                // Recover fs2
    fld     f19, 280(sp)                                // Recover fs3
    fld     f20, 288(sp)                                // Recover fs4
    fld     f21, 296(sp)                                // Recover fs5
    fld     f22, 304(sp)                                // Recover fs6
    fld     f23, 312(sp)                                // Recover fs7
    fld     f24, 320(sp)                                // Recover fs8
    fld     f25, 328(sp)                                // Recover fs9
    fld     f26, 336(sp)                                // Recover fs10
    fld     f27, 344(sp)                                // Recover fs11
    fld     f28, 352(sp)                                // Recover ft8
    fld     f29, 360(sp)                                // Recover ft9
    fld     f30, 368(sp)                                // Recover ft10
    fld     f31, 376(sp)                                // Recover ft11
    lw      t0,  384(sp)                                // Recover fcsr
    csrw    fcsr, t0                                    // Restore fcsr
_tx_thread_schedule_skip_fp_restore:
#endif

    /* Recover standard registers.  */
    lw      t0, 30*4(sp)                                // Recover mepc
    csrw    mepc, t0                                    // Setup mepc

    li      t0, 0x1880                                  // Prepare mstatus: MPP=Machine(0x1800) | MPIE(0x80)
#if defined(__riscv_float_abi_double)
    li      t1, 0x2000                                  // Set FS bits for FP state
    or      t0, t0, t1
#endif
    csrw    mstatus, t0                                 // Set mstatus

    lw      ra,  28*4(sp)                               // Recover return address
    lw      t0,  19*4(sp)                               // Recover t0
    lw      t1,  18*4(sp)                               // Recover t1
    lw      t2,  17*4(sp)                               // Recover t2
    lw      s0,  12*4(sp)                               // Recover s0
    lw      x9,  11*4(sp)                               // Recover s1
    lw      a0,  27*4(sp)                               // Recover a0
    lw      a1,  26*4(sp)                               // Recover a1
    lw      a2,  25*4(sp)                               // Recover a2
    lw      a3,  24*4(sp)                               // Recover a3
    lw      a4,  23*4(sp)                               // Recover a4
    lw      a5,  22*4(sp)                               // Recover a5
    lw      a6,  21*4(sp)                               // Recover a6
    lw      a7,  20*4(sp)                               // Recover a7
    lw      t3,  16*4(sp)                               // Recover t3
    lw      t4,  15*4(sp)                               // Recover t4
    lw      t5,  14*4(sp)                               // Recover t5
    lw      t6,  13*4(sp)                               // Recover t6
    lw      x18, 10*4(sp)                               // Recover s2
    lw      x19,  9*4(sp)                               // Recover s3
    lw      x20,  8*4(sp)                               // Recover s4
    lw      x21,  7*4(sp)                               // Recover s5
    lw      x22,  6*4(sp)                               // Recover s6
    lw      x23,  5*4(sp)                               // Recover s7
    lw      x24,  4*4(sp)                               // Recover s8
    lw      x25,  3*4(sp)                               // Recover s9
    lw      x26,  2*4(sp)                               // Recover s10
    lw      x27,  1*4(sp)                               // Recover s11

    addi    sp, sp, TX_TRAP_FRAME_SIZE                  // Recover stack frame
    mret                                                // Return to thread

_tx_thread_solicited_return:

    /* Recover floating point registers only if saved mstatus.FS was not Off.  */
#if defined(__riscv_float_abi_double)
    lw      t1, 14*4(sp)                                // Pickup saved mstatus
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_schedule_solicited_skip_fp_restore   // Skip if FS was Off
    fld     f8,  64(sp)                                 // Recover fs0
    fld     f9,  72(sp)                                 // Recover fs1
    fld     f18, 80(sp)                                 // Recover fs2
    fld     f19, 88(sp)                                 // Recover fs3
    fld     f20, 96(sp)                                 // Recover fs4
    fld     f21, 104(sp)                                // Recover fs5
    fld     f22, 112(sp)                                // Recover fs6
    fld     f23, 120(sp)                                // Recover fs7
    fld     f24, 128(sp)                                // Recover fs8
    fld     f25, 136(sp)                                // Recover fs9
    fld     f26, 144(sp)                                // Recover fs10
    fld     f27, 152(sp)                                // Recover fs11
    lw      t0,  160(sp)                                // Recover fcsr
    csrw    fcsr, t0                                    // Restore fcsr
_tx_thread_schedule_solicited_skip_fp_restore:
#endif

    /* Recover standard registers.  */
    lw      t0, 14*4(sp)                                // Reload saved mstatus (t0 may hold fcsr if FP was restored)
    csrw    mstatus, t0                                 // Restore mstatus

    lw      ra,  13*4(sp)                               // Recover return address
    lw      s0,  12*4(sp)                               // Recover s0
    lw      s1,  11*4(sp)                               // Recover s1
    lw      x18, 10*4(sp)                               // Recover s2
    lw      x19,  9*4(sp)                               // Recover s3
    lw      x20,  8*4(sp)                               // Recover s4
    lw      x21,  7*4(sp)                               // Recover s5
    lw      x22,  6*4(sp)                               // Recover s6
    lw      x23,  5*4(sp)                               // Recover s7
    lw      x24,  4*4(sp)                               // Recover s8
    lw      x25,  3*4(sp)                               // Recover s9
    lw      x26,  2*4(sp)                               // Recover s10
    lw      x27,  1*4(sp)                               // Recover s11

    addi    sp, sp, TX_SOL_FRAME_SIZE                   // Recover stack frame
    ret                                                 // Return to thread

/* }  */
