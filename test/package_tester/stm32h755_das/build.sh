#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/stm32h755-das-validation"
PACKAGE=""
SOURCE_SHA=""
STM32_CUBE_H7_DIR="${STM32_CUBE_H7_DIR:-}"

DAS_REPO="https://github.com/Inczert/device-abstraction-stack.git"
DAS_REF="bf0af2a0c53d361ed8bb271b4160dd3967b28af8"
STM32_CUBE_REPO="https://github.com/STMicroelectronics/STM32CubeH7.git"
STM32_CUBE_REF="f5c0b7a2b1f6eb26fde150f72edb2d7deb647066"
STM32_CMSIS_REPO="https://github.com/STMicroelectronics/cmsis-device-h7.git"
STM32_CMSIS_REF="e8d40ae6e2fa06afe5b46d24756f141b363342a7"

usage() {
  cat <<'USAGE'
Usage:
  build.sh [--package FILE] [--source-sha SHA] [--stm32h7-root DIR] [--build-dir DIR]

Builds the CCSDSPack NUCLEO-H755ZI-Q validation ELF using:
  - the exact CCSDSPack MCU package archive;
  - DAS for startup, clocks, board resources and UART;
  - CMSIS headers only from STM32CubeH7;
  - arm-none-eabi GCC/G++.

If --stm32h7-root is omitted, the qualified CMSIS revisions are fetched
under build/stm32h755-das-deps/.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --package) PACKAGE="$2"; shift 2 ;;
    --source-sha) SOURCE_SHA="$2"; shift 2 ;;
    --stm32h7-root) STM32_CUBE_H7_DIR="$2"; shift 2 ;;
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

for cmd in git cmake tar sha256sum arm-none-eabi-gcc arm-none-eabi-g++ arm-none-eabi-size; do
  command -v "$cmd" >/dev/null 2>&1 || {
    echo "Missing required command: $cmd" >&2
    exit 2
  }
done

if [[ -z "$PACKAGE" ]]; then
  PACKAGE="$(find "$ROOT_DIR/packages" -maxdepth 1 -type f     -name 'ccsdspack-v*-Generic-arm.tar.gz' -print 2>/dev/null | sort | tail -n 1 || true)"
fi
[[ -n "$PACKAGE" && -f "$PACKAGE" ]] || {
  echo "CCSDSPack MCU package not found. Pass --package <...Generic-arm.tar.gz>." >&2
  exit 2
}
PACKAGE="$(cd "$(dirname "$PACKAGE")" && pwd)/$(basename "$PACKAGE")"

if [[ -z "$SOURCE_SHA" ]]; then
  SOURCE_SHA="$(git -C "$ROOT_DIR" rev-parse HEAD)"
fi

DEPS_DIR="$ROOT_DIR/build/stm32h755-das-deps"
DAS_DIR="$DEPS_DIR/device-abstraction-stack"

prepare_checkout() {
  local url="$1" ref="$2" dir="$3"
  if [[ ! -d "$dir/.git" ]]; then
    rm -rf "$dir"
    git clone --filter=blob:none --no-checkout "$url" "$dir"
  fi
  git -C "$dir" fetch --depth 1 origin "$ref"
  git -C "$dir" checkout --detach --force FETCH_HEAD
  test "$(git -C "$dir" rev-parse HEAD)" = "$ref"
}

mkdir -p "$DEPS_DIR"
prepare_checkout "$DAS_REPO" "$DAS_REF" "$DAS_DIR"

if [[ -z "$STM32_CUBE_H7_DIR" ]]; then
  STM32_CUBE_H7_DIR="$DEPS_DIR/STM32CubeH7"
  CMSIS_DEVICE_DIR="$DEPS_DIR/cmsis-device-h7"

  if [[ ! -d "$STM32_CUBE_H7_DIR/.git" ]]; then
    rm -rf "$STM32_CUBE_H7_DIR"
    git clone --filter=blob:none --no-checkout "$STM32_CUBE_REPO" "$STM32_CUBE_H7_DIR"
    git -C "$STM32_CUBE_H7_DIR" sparse-checkout init --cone
    git -C "$STM32_CUBE_H7_DIR" sparse-checkout set Drivers/CMSIS/Core/Include Drivers/CMSIS/Include
  fi
  git -C "$STM32_CUBE_H7_DIR" fetch --depth 1 origin "$STM32_CUBE_REF"
  git -C "$STM32_CUBE_H7_DIR" checkout --detach --force FETCH_HEAD
  test "$(git -C "$STM32_CUBE_H7_DIR" rev-parse HEAD)" = "$STM32_CUBE_REF"

  prepare_checkout "$STM32_CMSIS_REPO" "$STM32_CMSIS_REF" "$CMSIS_DEVICE_DIR"
  mkdir -p "$STM32_CUBE_H7_DIR/Drivers/CMSIS/Device/ST/STM32H7xx"
  rm -rf "$STM32_CUBE_H7_DIR/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
  ln -s "$CMSIS_DEVICE_DIR/Include"     "$STM32_CUBE_H7_DIR/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
fi

[[ -f "$STM32_CUBE_H7_DIR/Drivers/CMSIS/Core/Include/core_cm7.h"    || -f "$STM32_CUBE_H7_DIR/Drivers/CMSIS/Include/core_cm7.h" ]] || {
  echo "CMSIS core_cm7.h not found below $STM32_CUBE_H7_DIR" >&2
  exit 2
}
[[ -f "$STM32_CUBE_H7_DIR/Drivers/CMSIS/Device/ST/STM32H7xx/Include/stm32h755xx.h" ]] || {
  echo "STM32H755 CMSIS device header not found below $STM32_CUBE_H7_DIR" >&2
  exit 2
}

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/package"
tar -xzf "$PACKAGE" -C "$BUILD_DIR/package"

CONFIG="$(find "$BUILD_DIR/package" -type f   -path '*/lib/cmake/CCSDSPack/CCSDSPackConfig.cmake' -print -quit)"
[[ -n "$CONFIG" ]] || {
  echo "CCSDSPackConfig.cmake not found in package $PACKAGE" >&2
  exit 3
}
PREFIX="${CONFIG%/lib/cmake/CCSDSPack/CCSDSPackConfig.cmake}"
LIBRARY="$PREFIX/lib/libccsdspack.a"
[[ -f "$LIBRARY" ]] || {
  echo "libccsdspack.a not found in extracted package" >&2
  exit 3
}

PACKAGE_SHA="$(sha256sum "$PACKAGE" | awk '{print $1}')"
LIBRARY_SHA="$(sha256sum "$LIBRARY" | awk '{print $1}')"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR/firmware"   -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/cmake/toolchains/arm-none-eabi.cmake"   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_PREFIX_PATH="$PREFIX"   -DSTM32_CUBE_H7_DIR="$STM32_CUBE_H7_DIR"   -DCCSDSPACK_DAS_SOURCE_DIR="$DAS_DIR"   -DCCSDSPACK_VALIDATION_SOURCE_SHA="$SOURCE_SHA"   -DCCSDSPACK_VALIDATION_PACKAGE_SHA256="$PACKAGE_SHA"   -DCCSDSPACK_VALIDATION_LIBRARY_SHA256="$LIBRARY_SHA"

cmake --build "$BUILD_DIR/firmware" --parallel

ELF="$BUILD_DIR/firmware/ccsdspack_stm32h755_validation.elf"
MAP="$BUILD_DIR/firmware/ccsdspack_stm32h755_validation.map"
[[ -s "$ELF" ]] || {
  echo "Expected ELF was not produced: $ELF" >&2
  exit 4
}

{
  echo "source_sha=$SOURCE_SHA"
  echo "package=$PACKAGE"
  echo "package_sha256=$PACKAGE_SHA"
  echo "libccsdspack_sha256=$LIBRARY_SHA"
  echo "das_sha=$DAS_REF"
  echo "stm32cubeh7_sha=$STM32_CUBE_REF"
  echo "cmsis_device_h7_sha=$STM32_CMSIS_REF"
  arm-none-eabi-size "$ELF"
} | tee "$BUILD_DIR/firmware/size-and-identity.txt"

echo
echo "ELF: $ELF"
echo "MAP: $MAP"
echo "Identity: $BUILD_DIR/firmware/size-and-identity.txt"
