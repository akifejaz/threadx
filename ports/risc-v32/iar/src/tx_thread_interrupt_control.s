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


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** ThreadX Component                                                     */
/**                                                                       */
/**   Thread                                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* #define TX_SOURCE_CODE  */


/* Include necessary system files.  */

/*  #include "tx_api.h"
    #include "tx_thread.h"  */

MSTATUS_MIE     DEFINE          0x00000008
RETURN_MASK     DEFINE          0x00000008

    SECTION `.text`:CODE:REORDER:NOROOT(2)
    CODE
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_interrupt_control                       RISC-V32/IAR     */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*    Tom van Leeuwen, Technolution B.V.                                  */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is responsible for changing the interrupt lockout     */
/*    posture of the system.                                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    new_posture                           New interrupt lockout posture */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    old_posture                           Old interrupt lockout posture */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/**************************************************************************/
/* UINT   _tx_thread_interrupt_control(UINT new_posture)
{  */
    PUBLIC  _tx_thread_interrupt_control
_tx_thread_interrupt_control:
    /* Pickup current interrupt lockout posture.  */

    csrr    t0, mstatus                                 ; Pickup mstatus
    andi    t0, t0, RETURN_MASK                         ; Mask out all but MIE (bit 3)

    /* Apply the new interrupt posture.  One CSR instruction does the
       read-modify-write atomically.  An interrupt between a separate read
       and write can no longer make the write restore a stale mstatus.  */

    andi    t1, a0, MSTATUS_MIE                         ; Isolate MIE in the new posture
    beqz    t1, _tx_thread_interrupt_disable            ; If 0, disable interrupts

    csrsi   mstatus, MSTATUS_MIE                        ; Enable interrupts (MIE, bit 3)
    j       _tx_thread_interrupt_control_exit           ; Return to caller

_tx_thread_interrupt_disable:

    csrci   mstatus, MSTATUS_MIE                        ; Disable interrupts (MIE, bit 3)

_tx_thread_interrupt_control_exit:

    /* Return the previous interrupt posture.  */

    mv      a0, t0                                      ; Setup return value
    ret
/* }  */
    END
