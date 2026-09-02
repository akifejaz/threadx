# RISC-V ThreadX Port Compliance Review

## Result

I reviewed the current `rv-ports-complience` worktree against `dev`.
I found and repaired several ABI, trap, timer, PLIC, linker, FP, and vector defects.
A final independent review found no remaining high-confidence defect in the changed code.

This result is not a hardware release approval.
The IAR port and three physical board paths still need target tests.
RVV V1.0 received a Spike smoke test, but it did not receive the strict QEMU VLEN tests.

## Review scope

- Branch: `rv-ports-complience`
- Baseline: `dev`
- `HEAD` and `dev`: `8c681c188ee748c777185e229024255c054a49d9`
- Reviewed state: tracked and untracked worktree changes
- Final tracked diff: 89 files
- Final tracked size: 1,945 insertions and 1,090 deletions

The branch and `dev` point to the same commit.
Therefore, the review applies to the worktree diff, not to committed branch changes.

The review covered these areas:

- RV32 GNU, Clang, and IAR context code
- RV64 GNU context code
- M-mode and S-mode trap entry
- Direct, nested, and preempting interrupt paths
- Integer, FP, and vector context frames
- PLIC routing, claim, completion, and masks
- CLINT and SBI timer paths
- QEMU virt, CVA6, CORE-V MCU, and Banana Pi F3 BSPs
- Startup, linker, stack, heap, and ELF segments
- CMake toolchain selection
- QEMU test validity and process cleanup

## Sources

### RISC-V sources

- RISC-V ISA manual `v20260120`
- RISC-V psABI `v1.0`
- RISC-V PLIC `v1.0.0`
- RISC-V SBI `v3.0`
- RISC-V UDB commit `6960b4ac416ea5c9ff6946c0c071550232f61f1e`

Important references:

- [psABI integer calling convention](https://docs.riscv.org/reference/abi/v1.0/riscv-cc-procedure-calling-convention.html#2-1-1-integer-calling-convention)
- [PLIC interrupt claim process](https://docs.riscv.org/reference/plic/v1.0.0/plic-claims.html#7-1-interrupt-claim-process)
- [PLIC interrupt enables](https://docs.riscv.org/reference/plic/v1.0.0/plic-enables.html#5-1-interrupt-enables)
- [SBI legacy set timer](https://docs.riscv.org/reference/sbi/v3.0/ext-legacy.html#5-1-1-extension-set-timer-eid-0x00)
- `riscv-unified-db/spec/std/isa/csr/mstatus.yaml`
- `riscv-unified-db/spec/std/isa/csr/mepc.yaml`
- `riscv-unified-db/spec/std/isa/csr/mcause.yaml`
- `riscv-unified-db/spec/std/isa/csr/V/vlenb.yaml`
- `riscv-unified-db/spec/std/isa/csr/V/vstart.yaml`

The psABI requires 16-byte stack alignment at procedure entry.
The UDB places `mstatus.FS` at bits 14 through 13.
The UDB places `mstatus.VS` at bits 10 through 9.
When either field is zero, its related instructions cause illegal-instruction exceptions.

The PLIC returns claim ID zero when no interrupt is pending.
Software must not treat ID zero as a device interrupt.
PLIC enable words use bit `N mod 32` in word `N / 32`.

### ThreadX sources

- ThreadX Markdown docs commit `1e36a62618de3771c973196e7648cba50d19cf1f`
- ThreadX AsciiDoc docs commit `d26c42cbceb52c7855e2984caf02518d61c29f1d`
- `rtos-docs-asciidoc/rtos-docs/threadx/modules/ROOT/pages/chapter2.adoc`
- `rtos-docs-asciidoc/rtos-docs/threadx/modules/ROOT/pages/chapter3.adoc`
- `rtos-docs-asciidoc/rtos-docs/threadx/modules/ROOT/pages/overview-threadx.adoc`

ThreadX uses the system stack during initialization and ISR processing.
ThreadX requires a preempted thread to resume at its exact interrupted location.
The timer tick rate comes from `TX_TIMER_TICKS_PER_SECOND`.

### Board sources

- SpacemiT K1 SoC manual in the review workspace
- Banana Pi F3 and K1 device-tree data in the review workspace
- CORE-V MCU upstream BSP definitions
- CVA6 example BSP definitions

## Corrected findings

### 1. Stack alignment and frame consistency

**Risk:** High

Several RV32 and RV64 paths did not keep `sp` aligned to 16 bytes before C calls.
That behavior violates the RISC-V psABI.

The repair aligned all affected trap, scheduler, restore, and startup paths.
The final frame sizes are consistent across save, restore, stack build, and schedule code.

- RV32 full frame: 400 bytes
- RV32 solicited frame: 176 bytes
- RV64 full frame: 528 bytes
- RV64 solicited frame: 240 bytes

ELF attributes now report 16-byte stack alignment.

### 2. Unsupported ISA and ABI combinations

**Risk:** High

The original changes could build unsupported ABI and extension combinations.
Those combinations could select a frame layout that did not match generated code.

The repair added compile-time checks for these cases:

- RV32E
- RV32 ILP32F
- RV32 vector builds
- Invalid RV32 soft-ABI and hardware-FP combinations
- Invalid RV64 ABI and FLEN combinations
- IAR FLEN 64, because the IAR frame stores 32-bit FP values

### 3. Global pointer and entry address

**Risk:** High

Startup did not reliably initialize `gp`.
Some linker layouts did not place the direct QEMU entry at `0x80000000`.

The repair initializes `gp` from `__global_pointer$`.
The QEMU images now enter at `0x80000000`.
The linker scripts define separate executable and writable load segments.

### 4. Synchronous trap handling

**Risk:** Critical

The original trap logic advanced past ECALL and EBREAK instructions.
It also allowed unsupported synchronous exceptions to continue.
That behavior could hide faults and resume with invalid state.

The repair keeps ECALL, EBREAK, and unsupported synchronous exceptions fatal.
Only supported asynchronous interrupts enter the device or timer paths.

### 5. PLIC claim and completion

**Risk:** High

The original logic could complete claim ID zero.
Banana Pi startup also drained enabled level interrupts.
An asserted level interrupt can make such a drain loop run forever.

The repair treats claim ID zero as the no-pending result.
It completes only a valid claimed source.
It leaves PLIC sources masked until a driver enables each source.

### 6. RV32 timer rollover

**Risk:** High

RV32 cannot read or write a 64-bit memory-mapped timer atomically.
Simple 64-bit accesses can read a torn `mtime` value.
They can also program a transient early `mtimecmp` value.

The repair uses a high-low-high read sequence.
It writes `mtimecmp` low as all ones, then high, then final low.

### 7. Timer frequency

**Risk:** High

Several BSPs used a fixed 10 Hz assumption.
That value did not follow the ThreadX tick configuration.

The repair derives each period from `TX_TIMER_TICKS_PER_SECOND`.
It also uses each board's correct timer source.

- QEMU virt uses the platform timebase.
- Banana Pi F3 uses the K1 24 MHz timebase.
- CORE-V MCU uses the 32.768 kHz reference timer.
- CORE-V UART timing uses the measured SOC FLL clock.

### 8. FP and vector enablement

**Risk:** Critical

FP and vector instructions can trap when `mstatus.FS` or `mstatus.VS` is zero.
Some original paths used extension instructions before enabling their state.

The repair enables the required state before the first extension instruction.
It saves and restores the related status and register state.

### 9. LP64F on FLEN 64 hardware

**Risk:** Critical

The LP64F ABI preserves only the low 32 bits of callee-saved FP values.
However, an interrupt can occur while non-ABI code uses full 64-bit FP registers.
C ISR code can then preserve only the ABI-visible low halves.

The repair saves full FLEN 64 values for `fs0` through `fs11` before any C ISR code.
The direct, nested, and preempting restore paths reload those values.
The preemption path does not replace the original interrupt-time values.

A forced ISR test changed `fs0` from `1.0` to `2.0`.
The direct restore path returned `fs0` to `1.0`.
A true LP64F ELF also passed the complete QEMU test.

### 10. Vector trap stack

**Risk:** Critical

The Banana Pi S-mode trap used a fixed stack frame for vector state.
Vector register size depends on the hardware `vlenb` value.
A fixed frame can overwrite adjacent stack data.

The repair reserves `32 * vlenb + 32` bytes dynamically.
Save and restore code preserves vector registers and vector CSRs.
It reads `vstart` before any vector instruction.
It restores `vstart` after the load sequence.

The port defaults assume VLEN is not more than 4096 bits.
The comments state that hardware with a larger VLEN needs larger stack settings.

### 11. Linker and memory layout

**Risk:** High

Some linker scripts mixed executable and writable content.
Some scripts also had incomplete `gp`, stack, or heap placement.

The repair separates RX and RW load segments.
It defines `__global_pointer$`.
It aligns stack and heap boundaries.
It preserves the expected QEMU and board load addresses.

### 12. QEMU test false passes

**Risk:** High

The original runners could ignore failed time-slice and clock checks.
They could also leave QEMU processes after failures.

The repair requires explicit evidence for every check.
It returns failure when any required marker is absent.
It stops and reaps QEMU on success, failure, and timeout.
The RV32 soft-float target now supplies `--skip-fpu`.

### 13. Clang RV32 path

**Risk:** Medium

The Clang toolchain file tested the environment variable incorrectly.
The timer path also had the RV32 torn-access problem.

The repair checks `ENV{GCC_INSTALL_PREFIX}` correctly.
It uses the safe RV32 timer sequences.
The Clang image passed the strict QEMU test.

### 14. CVA6 and common C support

**Risk:** Medium

The CVA6 support used invalid C99 assembly tokens in some contexts.
The UART functions did not always return their declared values.
One transmit path did not pass an explicit byte.

The repair uses valid inline assembly syntax.
It returns the declared values and sends the requested byte.
The complete CVA6 image linked without warnings.

## Board and routing review

### QEMU virt

- Entry address: `0x80000000`
- Machine timer: CLINT path
- Interrupt controller: PLIC path
- Privilege mode: M-mode
- Result: RV32, RV32 soft-float, RV32 Clang, RV64 LP64D, and RV64 LP64F passed

### Banana Pi F3 and SpacemiT K1

- UART0 base: `0xD4017000`
- UART0 interrupt: 42
- PLIC base: `0xE0000000`
- PLIC sources: 159
- Timebase: 24 MHz
- Timer service: legacy RV64 SBI `set_timer`
- Privilege mode: S-mode
- Result: standard and vector images linked

The board was not available.
Therefore, UART, PLIC, SBI, and timer behavior received static review only.

### CORE-V MCU

- Timer source: 32.768 kHz reference
- `CCFG=1` selects the reference source
- UART source: measured SOC FLL clock
- Result: 2 of 2 host tests passed

The required xPack CORE-V compiler was not available.
Therefore, the complete board image did not link.

### CVA6

- Result: complete image linked without warnings
- QEMU trace reached `uart_init`

The FPGA UART model does not match QEMU virt.
Therefore, the CVA6 QEMU run did not provide a functional board test.

## Validation results

| Validation | Result |
|---|---|
| RV32 regression suite | 95 of 95 passed |
| RV64 regression suite after LP64F repair | 95 of 95 passed |
| RV32 GNU ILP32D QEMU | Passed |
| RV32 GNU soft-float QEMU | Passed |
| RV32 Clang QEMU | Passed |
| RV64 GNU LP64D QEMU | Passed |
| True RV64 LP64F QEMU | Passed |
| Forced LP64F `fs0` interrupt restore | Passed |
| RV64 RVV V1.0 build | Passed |
| RV64 RVV V1.0 Spike smoke test | Passed |
| Banana Pi F3 standard S-mode link | Passed |
| Banana Pi F3 vector S-mode link | Passed |
| CVA6 warning-free link | Passed |
| CORE-V MCU host tests | 2 of 2 passed |
| Changed Python syntax checks | Passed |
| Changed shell syntax checks | Passed |
| Final `git diff --check` | Passed |
| Final independent review | No high-confidence defect |

## ELF evidence

The checked QEMU ELF files have these properties:

| Image | ELF ABI flags | Entry | Stack attribute |
|---|---|---|---|
| RV32 ILP32D | `0x5`, double-float ABI | `0x80000000` | 16 bytes |
| RV64 LP64D | `0x5`, double-float ABI | `0x80000000` | 16 bytes |
| RV64 LP64F | `0x3`, single-float ABI | `0x80000000` | 16 bytes |
| RV64 vector LP64D | `0x5`, double-float ABI | `0x80000000` | 16 bytes |

Each ELF has separate executable and writable load segments.
The vector ELF reports extension V1.0.

## Remaining limits

1. The IAR toolchain was not available.
   The RV32 IAR assembly received static review only.
2. The K1 board was not available.
   Banana Pi F3 S-mode behavior received link and static checks only.
3. The CORE-V compiler and board were not available.
   Only host BSP tests and strict changed-file compiles ran.
4. The CVA6 FPGA target was not available.
   QEMU virt cannot model its UART path correctly.
5. QEMU 6.2 supports vector draft 0.7.1, not vector V1.0.
   Spike provided only a continuous-run smoke test for the V1.0 image.
6. Strict VLEN 128 and VLEN 256 context tests need a vector V1.0 simulator.
7. The default vector stack sizes assume VLEN is not more than 4096 bits.
   Larger hardware needs larger configured stacks.
8. The preemption runner proves timer-thread preemption.
   It does not separately prove application-to-application preemption.

## Final assessment

The repaired worktree meets the reviewed RISC-V ABI, CSR, PLIC, SBI, and ThreadX contracts.
The available regression, QEMU, Spike, link, ELF, and static checks pass.
The final independent review found no high-confidence defect.

Do not treat this review as final production approval for untested hardware.
Run IAR, K1, CORE-V MCU, CVA6, and strict RVV V1.0 target tests before release.
