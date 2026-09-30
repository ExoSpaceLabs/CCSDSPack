#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/stm32h755-das-validation"
ELF="$BUILD_DIR/firmware/ccsdspack_stm32h755_validation.elf"
UART=""
TIMEOUT_SECONDS=20
LOG="$BUILD_DIR/hardware-uart.log"

usage() {
  cat <<'USAGE'
Usage:
  run.sh [--elf FILE] [--uart DEVICE] [--timeout SECONDS] [--log FILE]

Flashes the CM7 ELF through OpenOCD/ST-LINK, captures the DAS ST-LINK VCP
UART stream at 115200 8N1, and returns success only when
CCSDSPACK_HARDWARE_TEST:PASS is observed.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --elf) ELF="$2"; shift 2 ;;
    --uart) UART="$2"; shift 2 ;;
    --timeout) TIMEOUT_SECONDS="$2"; shift 2 ;;
    --log) LOG="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

for cmd in openocd stty grep arm-none-eabi-size; do
  command -v "$cmd" >/dev/null 2>&1 || {
    echo "Missing required command: $cmd" >&2
    exit 2
  }
done
[[ -s "$ELF" ]] || {
  echo "ELF not found: $ELF" >&2
  echo "Run $SCRIPT_DIR/build.sh first." >&2
  exit 2
}

if [[ -z "$UART" ]]; then
  if [[ -d /dev/serial/by-id ]]; then
    mapfile -t candidates < <(
      find /dev/serial/by-id -maxdepth 1 -type l -printf '%p\n' 2>/dev/null         | grep -Ei 'STMicroelectronics|ST.?LINK' || true
    )
    if (( ${#candidates[@]} == 1 )); then
      UART="${candidates[0]}"
    fi
  fi

  if [[ -z "$UART" ]]; then
    mapfile -t acm < <(compgen -G '/dev/ttyACM*' || true)
    if (( ${#acm[@]} == 1 )); then
      UART="${acm[0]}"
    fi
  fi
fi

[[ -n "$UART" && -e "$UART" ]] || {
  echo "Could not uniquely determine the ST-LINK VCP UART." >&2
  echo "Pass --uart /dev/serial/by-id/... or --uart /dev/ttyACM0." >&2
  exit 2
}

mkdir -p "$(dirname "$LOG")"
: > "$LOG"

echo "ELF : $ELF"
echo "UART: $UART"
echo "LOG : $LOG"
arm-none-eabi-size "$ELF"

stty -F "$UART" 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo

set +e
stdbuf -o0 cat "$UART" | tee "$LOG" &
CAPTURE_PID=$!
set -e
trap 'kill "$CAPTURE_PID" 2>/dev/null || true' EXIT

sleep 0.2

openocd   -f "$SCRIPT_DIR/openocd_h755.cfg"   -c "init"   -c "reset halt"   -c "program $ELF verify"   -c "reset run"   -c "shutdown"

deadline=$((SECONDS + TIMEOUT_SECONDS))
while (( SECONDS < deadline )); do
  if grep -q '^CCSDSPACK_HARDWARE_TEST:PASS' "$LOG"; then
    sleep 0.2
    kill "$CAPTURE_PID" 2>/dev/null || true
    wait "$CAPTURE_PID" 2>/dev/null || true
    trap - EXIT
    echo
    echo "CCSDSPack STM32H755 validation: PASS"
    exit 0
  fi

  if grep -q '^CCSDSPACK_HARDWARE_TEST:FAIL:' "$LOG"      || grep -q '^CCSDSPACK_HARDWARE_TEST:FAULT:' "$LOG"; then
    sleep 0.2
    kill "$CAPTURE_PID" 2>/dev/null || true
    wait "$CAPTURE_PID" 2>/dev/null || true
    trap - EXIT
    echo
    echo "CCSDSPack STM32H755 validation: FAIL" >&2
    exit 1
  fi

  sleep 0.1
done

kill "$CAPTURE_PID" 2>/dev/null || true
wait "$CAPTURE_PID" 2>/dev/null || true
trap - EXIT

echo
echo "Timed out after ${TIMEOUT_SECONDS}s waiting for UART PASS/FAIL." >&2
echo "Captured UART log: $LOG" >&2
exit 1
