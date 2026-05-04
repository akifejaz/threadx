#!/bin/bash
# =============================================================================
# build_bpif3.sh – Build ThreadX kernel.elf for BananaPi BPI-F3 (SpacemiT K1)
# =============================================================================
#
# Target hardware:
#   Board   : BananaPi BPI-F3
#   SoC     : SpacemiT K1 (8-core RV64GCBv, SpacemiT X60)
#   DRAM    : 4 GB LPDDR4, mapped from 0x0000_0000 (DRAM_0)
#
# Required toolchain: riscv64-unknown-elf-gcc (newlib / bare-metal)
#   Install: see https://github.com/riscv-collab/riscv-gnu-toolchain
#   Tested with: riscv64-unknown-elf-gcc 13.x
#
# ---- M-mode execution note --------------------------------------------------
# This image uses Machine-mode (M-mode) CSRs (mtvec, mie, mstatus, mhartid).
# On stock BPI-F3, OpenSBI owns M-mode and U-Boot runs in Supervisor mode
# (S-mode).  Any code loaded by U-Boot `go`/`bootm`/`bootelf` therefore
# executes in S-mode – accessing M-mode CSRs causes an illegal instruction
# trap.  To run this BSP in M-mode, use Option A (flash as OpenSBI fw_payload)
# or Option B (JTAG, which bypasses U-Boot entirely).
#
# Option C (TFTP) is usable in two sub-modes:
#   C-1: load the fw_payload.bin (Option A output) instead of kernel.elf –
#        that binary already contains an OpenSBI header that re-enters M-mode
#        before handing off to ThreadX.
#   C-2: load kernel.uimg directly and `bootm` – NOT SUPPORTED with the
#        current M-mode BSP.  The ThreadX risc-v64 port assembly (mret,
#        mepc, mstatus) is M-mode-only; S-mode bootm will still fault at
#        the first mstatus/mtvec access after entry.s.  An S-mode ThreadX
#        port (new ports/risc-v64s/ directory) would be needed for this.
#        Use Option B (JTAG) for development iteration.
# -----------------------------------------------------------------------------
#
# How to run on hardware:
# ---------------------------------------------------------------------------
#
#   Option A – OpenSBI fw_payload (recommended for production):
#     1. Compile OpenSBI with this image as payload:
#          make PLATFORM=generic \
#               FW_PAYLOAD=y \
#               FW_PAYLOAD_PATH=$(pwd)/kernel.elf \
#               FW_PAYLOAD_OFFSET=0x200000
#     2. Flash the resulting fw_payload.bin to the BPI-F3 SPI flash using
#        the standard BPI-F3 flashing procedure (USB mass storage boot mode).
#
#   Option B – JTAG direct load (M-mode, no firmware required):
#     1. Connect OpenOCD to the BPI-F3 JTAG header.
#     2. Load kernel.elf at 0x0040_0000 and run:
#          openocd -f bpif3.cfg
#          (gdb) load kernel.elf
#          (gdb) continue
#
#   Option C – TFTP boot (fast development iteration):
#     Prerequisites:
#       • TFTP server running on your host (e.g. tftpd-hpa, dnsmasq)
#       • /srv/tftp/ writable; BPI-F3 connected to the same LAN
#       • U-Boot prompt accessible over UART console
#
#     Build artifacts placed in /srv/tftp/:
#       kernel.bin  – raw stripped binary (ELF headers removed)
#       kernel.uimg – U-Boot uImage (mkimage wrapper; required for bootm)
#
#     C-1 (recommended): load fw_payload.bin – full M-mode execution
#       Build OpenSBI fw_payload first (Option A step 1), then:
#         cp <opensbi-build>/platform/generic/firmware/fw_payload.bin /srv/tftp/
#       In U-Boot terminal:
#         setenv ipaddr    192.168.10.2
#         setenv serverip  192.168.10.5
#         setenv netmask   255.255.255.0
#         setenv gatewayip 192.168.1.1
#         setenv tftpblocksize 1024
#         tftpboot ${loadaddr} fw_payload.bin
#         go ${loadaddr}
#
#     C-2: kernel.uimg + bootm – NOT SUPPORTED with this BSP.
#       bootm launches code in S-mode (U-Boot's privilege level after OpenSBI).
#       The ThreadX risc-v64 port assembly hard-codes M-mode: mret, mepc,
#       mstatus with MPP field.  These fault in S-mode even after the entry.s
#       fix.  An S-mode ThreadX port would be required.
#       → For development iteration use Option B (JTAG) instead.
#
# Build outputs: kernel.elf  kernel.bin  kernel.uimg
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"

echo "==> Building ThreadX static library for risc-v64/gnu ..."
rm -rf "${BUILD_DIR}"
cmake -B "${BUILD_DIR}" -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/cmake/riscv64_gnu.cmake" \
      "${REPO_ROOT}"
cmake --build "${BUILD_DIR}"

echo "==> Compiling BPI-F3 K1 BSP and demo ..."
pushd "${SCRIPT_DIR}" > /dev/null

rm -f kernel.elf

riscv64-unknown-elf-gcc \
    -march=rv64gc -mabi=lp64d \
    -mcmodel=medany \
    -O0 -g3 -Wall \
    -ffunction-sections -fdata-sections \
    -I"${REPO_ROOT}/common/inc" \
    -I"${REPO_ROOT}/ports/risc-v64/gnu/inc" \
    entry.s \
    tx_initialize_low_level.S \
    board.c \
    uart.c \
    hwtimer.c \
    plic.c \
    trap.c \
    demo_threadx.c \
    -L"${BUILD_DIR}" -lthreadx \
    -T link.lds -nostartfiles \
    -Wl,--no-warn-rwx-segments \
    -o kernel.elf

popd > /dev/null

# ---------------------------------------------------------------------------
# Post-build: generate kernel.bin and kernel.uimg for TFTP boot (Option C).
# ---------------------------------------------------------------------------
pushd "${SCRIPT_DIR}" > /dev/null

echo "==> Generating raw binary kernel.bin ..."
riscv64-unknown-elf-objcopy -O binary kernel.elf kernel.bin

# Generate kernel.uimg (U-Boot image) if mkimage is available.
# Load address and entry point must match the linker script (0x00400000).
LOAD_ADDR="0x00400000"
ENTRY_ADDR="0x00400000"
if command -v mkimage > /dev/null 2>&1; then
    echo "==> Generating U-Boot image kernel.uimg ..."
    mkimage -A riscv \
            -O linux \
            -T kernel \
            -C none \
            -a "${LOAD_ADDR}" \
            -e "${ENTRY_ADDR}" \
            -n "ThreadX BPI-F3 K1" \
            -d kernel.bin \
            kernel.uimg
else
    echo "==> mkimage not found – skipping kernel.uimg generation."
    echo "    Install u-boot-tools (apt install u-boot-tools) to enable."
fi

# Copy artifacts to TFTP server directory if it is writable.
TFTP_DIR="/srv/tftp"
if [ -d "${TFTP_DIR}" ] && [ -w "${TFTP_DIR}" ]; then
    echo "==> Copying kernel.bin and kernel.uimg to ${TFTP_DIR} ..."
    cp kernel.bin "${TFTP_DIR}/"
    [ -f kernel.uimg ] && cp kernel.uimg "${TFTP_DIR}/"
else
    echo "==> ${TFTP_DIR} not writable – skipping TFTP copy."
    echo "    Run manually: sudo cp kernel.bin kernel.uimg ${TFTP_DIR}/"
fi

popd > /dev/null

echo ""
echo "==> Build complete: ${SCRIPT_DIR}/kernel.elf"
echo ""
echo "ELF info:"
riscv64-unknown-elf-size "${SCRIPT_DIR}/kernel.elf" 2>/dev/null || true
riscv64-unknown-elf-objdump -f "${SCRIPT_DIR}/kernel.elf" 2>/dev/null | head -6 || true
echo ""
echo "Artifacts:"
echo "  kernel.elf   – ELF with debug symbols (JTAG / Option B)"
echo "  kernel.bin   – raw stripped binary     (TFTP raw / Option C)"
[ -f "${SCRIPT_DIR}/kernel.uimg" ] && \
echo "  kernel.uimg  – U-Boot uImage           (TFTP bootm / Option C-2)"
echo ""
echo "Next steps:"
echo ""
echo "  Option A (OpenSBI fw_payload – M-mode, recommended):"
echo "    make PLATFORM=generic FW_PAYLOAD=y \\"
echo "         FW_PAYLOAD_PATH=\$(pwd)/kernel.elf \\"
echo "         FW_PAYLOAD_OFFSET=0x200000"
echo ""
echo "  Option B (JTAG – M-mode, no firmware needed):"
echo "    openocd -f bpif3.cfg  # then: (gdb) load kernel.elf && continue"
echo ""
echo "  Option C-1 (TFTP + fw_payload.bin – M-mode):"
echo "    # After building Option A, copy fw_payload.bin to /srv/tftp/"
echo "    # In U-Boot:"
echo "    #   setenv ipaddr 192.168.10.2 ; setenv serverip 192.168.10.5"
echo "    #   setenv netmask 255.255.255.0 ; setenv gatewayip 192.168.1.1"
echo "    #   setenv tftpblocksize 1024"
echo "    #   tftpboot \${loadaddr} fw_payload.bin"
echo "    #   go \${loadaddr}"
echo ""
echo "  Option C-2 (TFTP + kernel.uimg + bootm) – NOT SUPPORTED with this BSP:"
echo "    # The ThreadX risc-v64 port uses mret/mepc/mstatus (M-mode only)."
echo "    # bootm launches code in S-mode (U-Boot context) → faults on"
echo "    # mstatus/mtvec/mret after entry.s.  A full S-mode ThreadX port"
echo "    # (ports/risc-v64s/) is required to make bootm work."
echo "    # Use Option B (JTAG) for development iteration on this BSP."
