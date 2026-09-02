# RISC-V Port Review Commands

This file records the main review, build, and test commands.

## Paths

```sh
ROOT=/home/akif-10xe/projects/threadx-work/threadx
WORK=/home/akif-10xe/projects/threadx-work
UDB=/home/akif-10xe/projects/threadx-work/riscv-unified-db
DOCS=/home/akif-10xe/projects/threadx-work/rtos-docs
ASCIIDOC=/home/akif-10xe/projects/threadx-work/rtos-docs-asciidoc
ART=/home/akif-10xe/.copilot/session-state/fa52efb4-0d93-4228-953c-57ba26abe404/files
RISCV_SPEC=/home/akif-10xe/.claude/skills/riscv-spec
```

## Environment and revisions

```sh
source "$UDB/.venv/bin/activate"
python --version

git -C "$ROOT" branch --show-current
git -C "$ROOT" rev-parse HEAD
git -C "$ROOT" rev-parse dev
git -C "$ROOT" status --short
git -C "$ROOT" diff --stat dev
git -C "$ROOT" diff --check dev

git -C "$UDB" rev-parse HEAD
git -C "$DOCS" rev-parse HEAD
git -C "$ASCIIDOC" rev-parse HEAD
```

Results:

- Branch: `rv-ports-complience`
- `HEAD` and `dev`: `8c681c188ee748c777185e229024255c054a49d9`
- UDB: `6960b4ac416ea5c9ff6946c0c071550232f61f1e`
- ThreadX Markdown docs: `1e36a62618de3771c973196e7648cba50d19cf1f`
- ThreadX AsciiDoc docs: `d26c42cbceb52c7855e2984caf02518d61c29f1d`
- Final tracked diff: 89 files, 1,945 insertions, and 1,090 deletions
- Final `git diff --check dev`: passed

## Specification queries

```sh
python3 "$RISCV_SPEC/scripts/riscv_docs.py" status
python3 "$RISCV_SPEC/scripts/riscv_docs.py" search \
  'stack pointer.*aligned|128-bit stack|128-bit boundary' --slug abi
python3 "$RISCV_SPEC/scripts/riscv_docs.py" search \
  'claim.*zero|ID.*zero|interrupt ID.*0|value of zero' --slug plic
python3 "$RISCV_SPEC/scripts/riscv_docs.py" search \
  'set_timer|stime_value' --slug sbi

sed -n '416,520p' "$UDB/spec/std/isa/csr/mstatus.yaml"
sed -n '1,180p' "$UDB/spec/std/isa/csr/V/vstart.yaml"
sed -n '1,180p' "$UDB/spec/std/isa/csr/V/vlenb.yaml"
sed -n '1,180p' "$UDB/spec/std/isa/csr/mepc.yaml"
sed -n '1,180p' "$UDB/spec/std/isa/csr/mcause.yaml"
```

Cached specification versions:

- RISC-V ISA: `v20260120`
- RISC-V psABI: `v1.0`
- RISC-V PLIC: `v1.0.0`
- RISC-V SBI: `v3.0`

## RV32 and RV64 regression suites

```sh
cmake -S "$ROOT/test/tx/cmake/riscv" \
  -B "$WORK/verification/tests/regr-rv32" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=default_build \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv32_gnu.cmake"
cmake --build "$WORK/verification/tests/regr-rv32" --parallel 2
ctest --test-dir "$WORK/verification/tests/regr-rv32" \
  --output-on-failure --parallel 2

cmake -S "$ROOT/test/tx/cmake/riscv" \
  -B "$WORK/verification/tests/regr-rv64" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=default_build \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv64_gnu.cmake"
cmake --build "$WORK/verification/tests/regr-rv64" --parallel 2
ctest --test-dir "$WORK/verification/tests/regr-rv64" \
  --output-on-failure --parallel 2
```

Results:

- RV32: 95 of 95 tests passed.
- RV64: 95 of 95 tests passed.

## GNU QEMU builds

```sh
cmake -S "$ROOT" -B "$ART/build-rv32" -GNinja \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv32_gnu.cmake"
cmake --build "$ART/build-rv32" --parallel 2 --target kernel.elf

cmake -S "$ROOT" -B "$ART/build-rv32-soft" -GNinja \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv32_gnu.cmake" \
  -DSOFT_FLOAT=1
cmake --build "$ART/build-rv32-soft" --parallel 2 --target kernel.elf

cmake -S "$ROOT" -B "$ART/build-rv64" -GNinja \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv64_gnu.cmake"
cmake --build "$ART/build-rv64" --parallel 2 --target kernel.elf
```

## Strict QEMU tests

```sh
cd "$ART/build-rv32/ports/risc-v32/gnu/example_build/qemu_virt"
python3 "$ROOT/ports/risc-v32/gnu/example_build/qemu_virt/test/threadx_test_tx_gnu_riscv32_qemu.py" \
  --elf ./kernel.elf \
  --gdb riscv32-unknown-elf-gdb

cd "$ART/build-rv32-soft/ports/risc-v32/gnu/example_build/qemu_virt"
python3 "$ROOT/ports/risc-v32/gnu/example_build/qemu_virt/test/threadx_test_tx_gnu_riscv32_qemu.py" \
  --elf ./kernel.elf \
  --gdb riscv32-unknown-elf-gdb \
  --skip-fpu

cd "$ART/build-rv64/ports/risc-v64/gnu/example_build/qemu_virt"
python3 "$ROOT/ports/risc-v64/gnu/example_build/qemu_virt/test/threadx_test_tx_gnu_riscv64_qemu.py" \
  --elf ./kernel.elf \
  --gdb riscv64-unknown-elf-gdb
```

Each test required these results:

- The timer interrupt occurred.
- The saved PC and `mepc` stayed equal.
- The time-slice handler ran.
- The ThreadX system clock increased.
- A higher-priority thread preempted the current thread.
- The FP check passed when the ABI used FP.

All applicable checks passed.

## Clang RV32 build and test

The repository names Clang 18. The host provides Clang 19.1.7.
Session-local aliases let the existing CMake file use that compiler.

```sh
mkdir -p "$ART/clang19-tools"
ln -s /home/akif-10xe/tools/spacemit-toolchain/bin/clang \
  "$ART/clang19-tools/clang-18"
ln -s /home/akif-10xe/tools/spacemit-toolchain/bin/clang++ \
  "$ART/clang19-tools/clang++-18"

PATH="$ART/clang19-tools:$PATH" \
GCC_INSTALL_PREFIX=/home/akif-10xe/tools/riscv32 \
cmake -S "$ROOT" -B "$ART/build-rv32-clang-wrapper2" -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv32_clang.cmake"

PATH="$ART/clang19-tools:$PATH" \
cmake --build "$ART/build-rv32-clang-wrapper2" --parallel 2 \
  --target threadx

cd "$ART/build-rv32-clang-sample"
python3 "$ROOT/ports/risc-v32/gnu/example_build/qemu_virt/test/threadx_test_tx_gnu_riscv32_qemu.py" \
  --elf ./demo_threadx.elf \
  --gdb riscv32-unknown-elf-gdb \
  --skip-fpu
```

The CMake build produced `libthreadx.a`.
The manually linked Clang sample passed the RV32 QEMU checks.

## True LP64F build and test

The session toolchain file uses `-march=rv64gc -mabi=lp64f`.

```sh
cmake -S "$ROOT" -B "$ART/build-rv64-true-lp64f" -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$ART/riscv64-lp64f.cmake"
cmake --build "$ART/build-rv64-true-lp64f" --parallel 2 \
  --target kernel.elf

cd "$ART/build-rv64-true-lp64f/ports/risc-v64/gnu/example_build/qemu_virt"
python3 "$ROOT/ports/risc-v64/gnu/example_build/qemu_virt/test/threadx_test_tx_gnu_riscv64_qemu.py" \
  --elf ./kernel.elf \
  --gdb riscv64-unknown-elf-gdb

riscv64-unknown-elf-readelf -h -A ./kernel.elf
```

The ELF flags were `0x3`, which select the single-float ABI.
The complete QEMU test passed.

The direct interrupt restore check used this GDB command file:

```sh
riscv64-unknown-elf-gdb --batch -x "$ART/lp64f-fs0.gdb"
grep LP64F_FS0_DIRECT_RESTORE_OK "$ART/lp64f-fs0-gdb.log"
```

The test set `fs0` to `1.0`, changed it to `2.0` in a stopped ISR, and resumed.
The restored value was `1.0`.

## RV64 vector build and smoke test

The session toolchain file uses `-march=rv64gcv -mabi=lp64d`.

```sh
cmake -S "$ROOT" -B "$ART/build-rv64-true-vector" -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$ART/riscv64-vector.cmake"
cmake --build "$ART/build-rv64-true-vector" --parallel 2 \
  --target kernel.elf

riscv64-unknown-elf-readelf -h -A \
  "$ART/build-rv64-true-vector/ports/risc-v64/gnu/example_build/qemu_virt/kernel.elf"

timeout 3 spike --isa=rv64gcv \
  "$ART/build-rv64-true-vector/ports/risc-v64/gnu/example_build/qemu_virt/kernel.elf" \
  >"$ART/spike-rv64-vector-v1.log" 2>&1
```

The ELF reports vector extension V1.0.
Spike reached the UART, `thread_0_entry`, and `thread_6_and_7_entry`.
The timeout ended the continuous demonstration after three seconds.

The strict QEMU VLEN command was:

```sh
cd "$ART/build-rv64-true-vector/ports/risc-v64/gnu/example_build/qemu_virt"
python3 "$ROOT/ports/risc-v64/gnu/example_build/qemu_virt/test/threadx_test_tx_gnu_riscv64_qemu.py" \
  --elf ./kernel.elf \
  --gdb riscv64-unknown-elf-gdb \
  --cpu rv64,x-v=true,vlen=128
```

QEMU 6.2 selected vector draft 0.7.1 and did not run the V1.0 ELF.
This QEMU also rejected `vext_spec=v1.0`.
Therefore, VLEN 128 and VLEN 256 strict tests are not valid on this host.

## Board builds

```sh
cmake -S "$ROOT" -B "$ART/build-rv64-bananapi" -GNinja \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv64_gnu.cmake"
cmake --build "$ART/build-rv64-bananapi" --parallel 2 \
  --target kernel.elf

cmake -S "$ROOT" -B "$ART/build-rv64-bananapi-vector" -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$ART/riscv64-vector.cmake"
cmake --build "$ART/build-rv64-bananapi-vector" --parallel 2 \
  --target kernel.elf

cmake -S "$ROOT" -B "$ART/build-rv32-cva6" -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/riscv32_gnu.cmake"
cmake --build "$ART/build-rv32-cva6" --parallel 2 \
  --target kernel.elf

cmake -S \
  "$ROOT/ports/risc-v32/gnu/example_build/core_v_mcu/tests" \
  -B "$ART/test-core-v-mcu" -GNinja
cmake --build "$ART/test-core-v-mcu" --parallel 2
ctest --test-dir "$ART/test-core-v-mcu" --output-on-failure
```

Results:

- Banana Pi F3 standard S-mode image: passed.
- Banana Pi F3 vector S-mode image: passed.
- CVA6 image: passed without compiler warnings.
- CORE-V MCU host tests: 2 of 2 passed.

The complete CORE-V MCU image needs the missing xPack CORE-V compiler.
The IAR RV32 port needs the missing IAR toolchain.

## ELF checks

```sh
for elf in \
  "$ART/build-rv32/ports/risc-v32/gnu/example_build/qemu_virt/kernel.elf" \
  "$ART/build-rv64/ports/risc-v64/gnu/example_build/qemu_virt/kernel.elf" \
  "$ART/build-rv64-true-lp64f/ports/risc-v64/gnu/example_build/qemu_virt/kernel.elf" \
  "$ART/build-rv64-true-vector/ports/risc-v64/gnu/example_build/qemu_virt/kernel.elf"
do
  case "$elf" in
    *build-rv32/*) tool=riscv32-unknown-elf-readelf ;;
    *)             tool=riscv64-unknown-elf-readelf ;;
  esac
  "$tool" -h -l -A "$elf" |
    grep -E 'Entry point|LOAD|Flags:|Tag_RISCV_stack_align|Tag_RISCV_arch'
done
```

All four images use entry address `0x80000000`.
All four images report 16-byte stack alignment.
Each image has separate executable and writable load segments.

## Final static checks

```sh
cd "$ROOT"

mapfile -t pyfiles < <(
  git diff --name-only --diff-filter=ACMR dev -- '*.py'
)
python3 -m py_compile "${pyfiles[@]}"

mapfile -t shfiles < <(
  git diff --name-only --diff-filter=ACMR dev -- '*.sh'
)
bash -n "${shfiles[@]}"

git diff --check dev
ps -eo pid=,args= |
  grep -E '[q]emu-system-riscv(32|64)|[s]pike .*kernel\.elf' || true
```

Results:

- Two changed Python files passed syntax checks.
- One changed shell file passed its syntax check.
- The diff check passed.
- No QEMU or Spike process remained.
