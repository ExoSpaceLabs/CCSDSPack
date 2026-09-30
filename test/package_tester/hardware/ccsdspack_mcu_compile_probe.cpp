// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_hardware_test.h"

// Compiled and relocatably linked by the generic arm-none-eabi package build.
// The same board-independent acceptance body is executed by the DAS H755 harness.
extern "C" int ccsdspack_mcu_compile_probe() {
  return CCSDSPackHardwareTest::run();
}
