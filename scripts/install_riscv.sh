#!/bin/bash
# Install RISC-V bare-metal cross-compiler toolchain and QEMU for CI.
set -e

RELEASE_TAG="2026.04.26"
BASE_URL="https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/${RELEASE_TAG}"
# Use ubuntu-24.04 binaries to match ubuntu-latest runners.
RV32_TARBALL="riscv32-glibc-ubuntu-22.04-gcc.tar.xz"
RV64_TARBALL="riscv64-glibc-ubuntu-22.04-gcc.tar.xz"

echo "=== Installing QEMU and build tools ==="
sudo apt-get update -qq
sudo apt-get install -y -qq qemu-system-misc ninja-build cmake

echo "=== Downloading RISC-V GCC toolchain (${RELEASE_TAG}) ==="
sudo mkdir -p /opt/riscv

# Both tarballs extract into riscv/ with non-overlapping prefixes
# (riscv32-unknown-elf-* and riscv64-unknown-elf-*).
wget -q "${BASE_URL}/${RV32_TARBALL}" -O /tmp/riscv32-gcc.tar.xz
sudo tar xJf /tmp/riscv32-gcc.tar.xz -C /opt --strip-components=0
rm /tmp/riscv32-gcc.tar.xz

wget -q "${BASE_URL}/${RV64_TARBALL}" -O /tmp/riscv64-gcc.tar.xz
sudo tar xJf /tmp/riscv64-gcc.tar.xz -C /opt --strip-components=0
rm /tmp/riscv64-gcc.tar.xz

TOOLCHAIN_BIN=/opt/riscv/bin
echo "$TOOLCHAIN_BIN" >> "$GITHUB_PATH"

echo "=== Verifying installation ==="
"$TOOLCHAIN_BIN/riscv32-unknown-elf-gcc" --version | head -1
"$TOOLCHAIN_BIN/riscv64-unknown-elf-gcc" --version | head -1
qemu-system-riscv64 --version | head -1
