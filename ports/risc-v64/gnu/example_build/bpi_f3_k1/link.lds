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
 * Linker script for BananaPi BPI-F3 (SpacemiT K1 SoC)
 *
 * K1 address map:
 *   DRAM_0 : 0x0000_0000 – 0x7FFF_FFFF  (2 GB)
 *   DRAM_1 : 0x1_0000_0000 – 0x4_7FFF_FFFF  (14 GB, BPI-F3 has 4 GB total)
 *
 * This image is designed to be loaded as an OpenSBI fw_payload so that it
 * executes in Machine mode (M-mode).  A typical load address for fw_payload
 * on K1 is 0x0040_0000 (4 MB into DRAM_0, safely above OpenSBI itself).
 *
 * To package with OpenSBI:
 *   make PLATFORM=generic \
 *        FW_PAYLOAD=y \
 *        FW_PAYLOAD_PATH=<path>/kernel.elf \
 *        FW_PAYLOAD_OFFSET=0x200000
 */

OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
    /* Load base: 4 MB into DRAM_0, well above the first-stage bootloader
       and OpenSBI image (typically resident at 0x0020_0000).            */
    . = 0x00400000;


    .text : {
        *(.text .text.*)
        . = ALIGN(0x1000);
        PROVIDE(etext = .);
    }

    .rodata : {
        . = ALIGN(16);
        *(.srodata .srodata.*)
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*)
        . = ALIGN(16);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        _bss_start = .;
        *(.sbss .sbss.*)
        . = ALIGN(16);
        *(.bss .bss.*)
        _bss_end = .;
    }

    /* 8 KB system stack for the pre-ThreadX boot phase. */
    .stack : {
        . = ALIGN(4096);
        _sysstack_start = .;
        . += 0x2000;
        _sysstack_end = .;
    }

    /* ThreadX allocates its heap from _end onwards. */
    PROVIDE(_end = .);
}
