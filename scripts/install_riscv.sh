#!/bin/bash
# Install RISC-V bare-metal cross-compiler toolchain and QEMU for CI.
set -e

XPACK_VERSION="14.2.0-3"
XPACK_URL="https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v${XPACK_VERSION}/xpack-riscv-none-elf-gcc-${XPACK_VERSION}-linux-x64.tar.gz"

echo "=== Installing QEMU and build tools ==="
sudo apt-get update -qq
sudo apt-get install -y -qq qemu-system-misc ninja-build cmake

echo "=== Downloading xPack RISC-V Embedded GCC ${XPACK_VERSION} ==="
wget -q "$XPACK_URL" -O /tmp/riscv-gcc.tar.gz
sudo mkdir -p /opt/riscv
sudo tar xzf /tmp/riscv-gcc.tar.gz -C /opt/riscv --strip-components=1
rm /tmp/riscv-gcc.tar.gz

# The xPack toolchain provides riscv-none-elf-* binaries (multilib).
# Create symlinks matching the names our CMake toolchain files expect.
TOOLCHAIN_BIN=/opt/riscv/bin
for tool in gcc g++ ar as objcopy objdump size ld gdb; do
    [ -f "$TOOLCHAIN_BIN/riscv-none-elf-$tool" ] || continue
    sudo ln -sf "riscv-none-elf-$tool" "$TOOLCHAIN_BIN/riscv32-unknown-elf-$tool"
    sudo ln -sf "riscv-none-elf-$tool" "$TOOLCHAIN_BIN/riscv64-unknown-elf-$tool"
done

echo "$TOOLCHAIN_BIN" >> "$GITHUB_PATH"

echo "=== Verifying installation ==="
"$TOOLCHAIN_BIN/riscv-none-elf-gcc" --version | head -1
qemu-system-riscv64 --version | head -1
