#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
HARNESS_DIR="$ROOT_DIR/test/package_tester/stm32h7xx"
BUILD_ROOT="${CCSDSPACK_H755_BUILD_DIR:-$ROOT_DIR/build/h755-validation}"
OPENOCD_SCRIPTS="${OPENOCD_SCRIPTS:-/usr/share/openocd/scripts}"
UART_TIMEOUT="${CCSDSPACK_UART_TIMEOUT:-30}"

DAS_REPOSITORY="https://github.com/Inczert/device-abstraction-stack.git"
DAS_COMMIT="45d93cc016909fd132cfcaffcab493119775574b"
STM32_CUBE_H7_REPOSITORY="https://github.com/STMicroelectronics/STM32CubeH7.git"
STM32_CUBE_H7_COMMIT="f5c0b7a2b1f6eb26fde150f72edb2d7deb647066"

PACKAGE=""
SOURCE_SHA=""
UART_DEVICE=""
BUILD_ONLY=0
KEEP_BUILD=0
UART_PID=""

usage() {
  cat <<'USAGE'
Usage:
  test/package_tester/stm32h7xx/run_h755_validation.sh \
    --package /path/to/ccsdspack-v2.1.0-Generic-arm.tar.gz [options]

Required:
  --package FILE       CCSDSPack MCU package to link and execute.

Options:
  --source-sha SHA     Source SHA represented by the package.
                       Defaults to the current repository HEAD.
  --uart DEVICE        ST-LINK virtual COM device, for example /dev/ttyACM0.
                       Auto-detected when omitted.
  --build-dir DIR      Build/evidence directory.
  --build-only         Cross-build and size the firmware, but do not flash.
  --keep-build         Reuse existing external checkouts/build directories.
  -h, --help           Show this help.

Environment:
  OPENOCD_SCRIPTS      OpenOCD scripts root (default /usr/share/openocd/scripts).
  CCSDSPACK_UART_TIMEOUT
                       UART result timeout in seconds (default 30).

The runner pins:
  DAS:       45d93cc016909fd132cfcaffcab493119775574b
  STM32CubeH7 CMSIS:
             f5c0b7a2b1f6eb26fde150f72edb2d7deb647066

Only CMSIS headers are fetched from STM32CubeH7. No HAL, BSP, CubeIDE,
generated IDE project, or vendor makefile is used.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --package) PACKAGE="$2"; shift 2 ;;
    --source-sha) SOURCE_SHA="$2"; shift 2 ;;
    --uart) UART_DEVICE="$2"; shift 2 ;;
    --build-dir) BUILD_ROOT="$2"; shift 2 ;;
    --build-only) BUILD_ONLY=1; shift ;;
    --keep-build) KEEP_BUILD=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

[[ -n "$PACKAGE" ]] || { echo "--package is required" >&2; usage >&2; exit 2; }
PACKAGE="$(realpath "$PACKAGE")"
[[ -f "$PACKAGE" ]] || { echo "Package not found: $PACKAGE" >&2; exit 2; }

for command in cmake git tar sha256sum arm-none-eabi-gcc arm-none-eabi-g++ arm-none-eabi-size arm-none-eabi-nm; do
  command -v "$command" >/dev/null 2>&1 || {
    echo "Missing command: $command" >&2
    exit 2
  }
done

if [[ -z "$SOURCE_SHA" ]]; then
  SOURCE_SHA="$(git -C "$ROOT_DIR" rev-parse HEAD 2>/dev/null || echo unknown)"
fi

cleanup() {
  if [[ -n "$UART_PID" ]] && kill -0 "$UART_PID" >/dev/null 2>&1; then
    kill "$UART_PID" >/dev/null 2>&1 || true
    wait "$UART_PID" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT INT TERM

if (( KEEP_BUILD == 0 )); then
  rm -rf -- "$BUILD_ROOT"
fi
mkdir -p "$BUILD_ROOT/external" "$BUILD_ROOT/logs" "$BUILD_ROOT/package"

PACKAGE_SHA="$(sha256sum "$PACKAGE" | awk '{print $1}')"
printf 'CCSDSPack source SHA: %s\n' "$SOURCE_SHA" | tee "$BUILD_ROOT/logs/metadata.log"
printf 'CCSDSPack package: %s\n' "$PACKAGE" | tee -a "$BUILD_ROOT/logs/metadata.log"
printf 'CCSDSPack package SHA-256: %s\n' "$PACKAGE_SHA" | tee -a "$BUILD_ROOT/logs/metadata.log"
printf 'DAS SHA: %s\n' "$DAS_COMMIT" | tee -a "$BUILD_ROOT/logs/metadata.log"
printf 'STM32CubeH7 CMSIS SHA: %s\n' "$STM32_CUBE_H7_COMMIT" | tee -a "$BUILD_ROOT/logs/metadata.log"

if [[ ! -d "$BUILD_ROOT/external/das/.git" ]]; then
  git clone --filter=blob:none "$DAS_REPOSITORY" "$BUILD_ROOT/external/das"
fi
git -C "$BUILD_ROOT/external/das" fetch --depth 1 origin "$DAS_COMMIT"
git -C "$BUILD_ROOT/external/das" checkout --detach "$DAS_COMMIT"
test "$(git -C "$BUILD_ROOT/external/das" rev-parse HEAD)" = "$DAS_COMMIT"

CUBE_ROOT="$BUILD_ROOT/external/STM32CubeH7"
if [[ ! -d "$CUBE_ROOT/.git" ]]; then
  git clone --filter=blob:none --no-checkout "$STM32_CUBE_H7_REPOSITORY" "$CUBE_ROOT"
  git -C "$CUBE_ROOT" sparse-checkout init --cone
  git -C "$CUBE_ROOT" sparse-checkout set \
    Drivers/CMSIS/Include \
    Drivers/CMSIS/Device/ST/STM32H7xx
fi
git -C "$CUBE_ROOT" fetch --depth 1 origin "$STM32_CUBE_H7_COMMIT"
git -C "$CUBE_ROOT" checkout --detach "$STM32_CUBE_H7_COMMIT"
git -C "$CUBE_ROOT" submodule update --init --depth 1 \
  Drivers/CMSIS/Device/ST/STM32H7xx
test -f "$CUBE_ROOT/Drivers/CMSIS/Include/core_cm7.h"
test -f "$CUBE_ROOT/Drivers/CMSIS/Device/ST/STM32H7xx/Include/stm32h755xx.h"

rm -rf "$BUILD_ROOT/package/extracted"
mkdir -p "$BUILD_ROOT/package/extracted"
tar -xzf "$PACKAGE" -C "$BUILD_ROOT/package/extracted"
CCSDSPACK_PREFIX="$(find "$BUILD_ROOT/package/extracted" -mindepth 1 -maxdepth 1 \
  -type d -name 'ccsdspack-*' -print -quit)"
[[ -n "$CCSDSPACK_PREFIX" ]] || {
  echo "Could not locate extracted CCSDSPack package root" >&2
  exit 1
}
test -f "$CCSDSPACK_PREFIX/lib/libccsdspack.a"
test -f "$CCSDSPACK_PREFIX/lib/cmake/CCSDSPack/CCSDSPackConfig.cmake"
LIBRARY_SHA="$(sha256sum "$CCSDSPACK_PREFIX/lib/libccsdspack.a" | awk '{print $1}')"
printf 'libccsdspack.a SHA-256: %s\n' "$LIBRARY_SHA" | tee -a "$BUILD_ROOT/logs/metadata.log"

MCU_FLAGS="-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
TOOLCHAIN="$ROOT_DIR/cmake/toolchains/arm-none-eabi.cmake"
DAS_INSTALL="$BUILD_ROOT/das-install"

cmake -S "$BUILD_ROOT/external/das" -B "$BUILD_ROOT/das-build" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DMCU_FLAGS="$MCU_FLAGS" \
  -DCMAKE_BUILD_TYPE=Release \
  -DDAS_CORE=cm7 \
  -DDAS_DEVICE=nucleo_h755zi_q \
  -DSTM32_CUBE_H7_DIR="$CUBE_ROOT" \
  -DDAS_BUILD_HARDWARE_TESTS=OFF \
  -DDAS_BUILD_LINK_TESTS=OFF
cmake --build "$BUILD_ROOT/das-build" --parallel
cmake --install "$BUILD_ROOT/das-build" --prefix "$DAS_INSTALL"

cmake -S "$HARNESS_DIR" -B "$BUILD_ROOT/firmware" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DMCU_FLAGS="$MCU_FLAGS" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$CCSDSPACK_PREFIX;$DAS_INSTALL" \
  -DCCSDSPACK_VALIDATION_SOURCE_SHA="$SOURCE_SHA" \
  -DCCSDSPACK_VALIDATION_DAS_SHA="$DAS_COMMIT" \
  -DCCSDSPACK_VALIDATION_PACKAGE_SHA256="$PACKAGE_SHA" \
  -DCCSDSPACK_VALIDATION_LIBRARY_SHA256="$LIBRARY_SHA"
cmake --build "$BUILD_ROOT/firmware" --parallel

ELF="$BUILD_ROOT/firmware/ccsdspack_h755_validation.elf"
[[ -s "$ELF" ]] || { echo "Firmware ELF not found: $ELF" >&2; exit 1; }

arm-none-eabi-size "$ELF" | tee "$BUILD_ROOT/logs/size.log"
sha256sum "$ELF" | tee "$BUILD_ROOT/logs/elf.sha256"

if arm-none-eabi-nm -u "$ELF" | grep -Eq '(^|[[:space:]])(HAL_|BSP_)'; then
  echo "Unexpected STM32 HAL/BSP dependency in validation firmware" >&2
  arm-none-eabi-nm -u "$ELF" >&2
  exit 1
fi

if (( BUILD_ONLY != 0 )); then
  echo "H755 validation firmware build: PASS"
  echo "ELF: $ELF"
  exit 0
fi

for command in openocd stty timeout grep; do
  command -v "$command" >/dev/null 2>&1 || {
    echo "Missing runtime command: $command" >&2
    exit 2
  }
done

detect_uart() {
  local candidate
  for candidate in \
    /dev/serial/by-id/*STMicroelectronics*STLink* \
    /dev/serial/by-id/*STMicroelectronics*ST-LINK* \
    /dev/ttyACM*; do
    [[ -e "$candidate" ]] || continue
    realpath "$candidate"
    return 0
  done
  return 1
}

if [[ -z "$UART_DEVICE" ]]; then
  UART_DEVICE="$(detect_uart || true)"
fi
[[ -n "$UART_DEVICE" && -e "$UART_DEVICE" ]] || {
  echo "Could not auto-detect ST-LINK VCP. Pass --uart /dev/ttyACM<N>." >&2
  exit 2
}

echo "Using UART: $UART_DEVICE"
stty -F "$UART_DEVICE" 115200 cs8 -parenb -cstopb -ixon -ixoff -crtscts raw -echo

UART_LOG="$BUILD_ROOT/logs/uart.log"
OPENOCD_LOG="$BUILD_ROOT/logs/openocd.log"
: > "$UART_LOG"

timeout "${UART_TIMEOUT}s" cat "$UART_DEVICE" > >(tee "$UART_LOG") &
UART_PID=$!
sleep 0.2

openocd -s "$OPENOCD_SCRIPTS" \
  -f "$HARNESS_DIR/openocd_h755.cfg" \
  -c "init; reset halt; program $ELF verify; reset run; shutdown" \
  2>&1 | tee "$OPENOCD_LOG"

deadline=$((SECONDS + UART_TIMEOUT))
while (( SECONDS < deadline )); do
  if grep -Fq 'CCSDSPACK_HARDWARE_TEST:PASS' "$UART_LOG"; then
    break
  fi
  if grep -Eq 'CCSDSPACK_HARDWARE_TEST:(FAIL|FAULT)' "$UART_LOG"; then
    break
  fi
  sleep 0.1
done

cleanup
UART_PID=""

if grep -Eq 'CCSDSPACK_HARDWARE_TEST:(FAIL|FAULT)' "$UART_LOG"; then
  echo "STM32H755 hardware validation: FAIL"
  exit 1
fi
if ! grep -Fq 'CCSDSPACK_HARDWARE_TEST:PASS' "$UART_LOG"; then
  echo "STM32H755 hardware validation: TIMEOUT/NO PASS MARKER"
  exit 1
fi

echo
echo "STM32H755 hardware validation: PASS"
echo "CCSDSPack source SHA: $SOURCE_SHA"
echo "CCSDSPack package SHA-256: $PACKAGE_SHA"
echo "libccsdspack.a SHA-256: $LIBRARY_SHA"
echo "DAS SHA: $DAS_COMMIT"
echo "ELF: $ELF"
echo "UART log: $UART_LOG"
echo "OpenOCD log: $OPENOCD_LOG"
