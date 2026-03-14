#!/bin/bash

pushd ../../../../../
cmake -Bbuild -GNinja -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64_gnu.cmake .
cmake --build ./build/
popd


riscv64-unknown-elf-gcc \
-march=rv64gc -mabi=lp64d \
-mcmodel=medany -O0 -g3 -Wall \
-ffunction-sections -fdata-sections \
-I../../../../../common/inc \
-I../../inc \
entry.s \
tx_initialize_low_level.S \
board.c uart.c hwtimer.c plic.c trap.c demo_threadx.c \
-L../../../../../build -lthreadx \
-T link.lds -nostartfiles -nodefaultlibs -lgcc \
-o kernel.elf
