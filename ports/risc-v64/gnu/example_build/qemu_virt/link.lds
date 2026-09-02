OUTPUT_ARCH( "riscv" )
ENTRY( _start )

PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;

  .text : {
    KEEP(*(.text.entry))
    *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(etext = .);
  } :text

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  } :text

  . = ALIGN(0x1000);
  .data : {
    . = ALIGN(16);
    /* Centre __global_pointer$ in the small-data window so the +/-2 KiB
       reach of GP-relative addressing covers the .sdata/.sbss area. */
    PROVIDE( __global_pointer$ = . + 0x800 );
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  } :data

  .bss (NOLOAD) : {
    . = ALIGN(16);
    _bss_start = .;
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
    _bss_end = .;
  } :data

  .stack (NOLOAD) : {
    . = ALIGN(4096);
    _sysstack_start = .;
    . += 0x1000;
    . += 0x4000; /* Vector headroom, reserved unconditionally.  0x4000 = 16384 bytes does
                    NOT cover the vector save area on its own: the worst case at VLEN=4096
                    (vlenb = 512) is 32*512 + 4*8 = 16416 bytes (0x4020), so this line is
                    32 bytes short and the 0x1000 above absorbs the remainder.  The total
                    _sysstack_start.._sysstack_end span stays 0x1000 + 0x4000 = 0x5000
                    (20480 bytes), which is what entry.S adds to _sysstack_start for a
                    vector build: 20480 - (16416 + 528 trap frame) = 3536 bytes remain for
                    ordinary stack use.  Do not shrink either line. */
    _sysstack_end = .;
  } :data

  PROVIDE(_end = .);
}
