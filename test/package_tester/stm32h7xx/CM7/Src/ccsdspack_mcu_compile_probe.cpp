// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_mcu_test.h"

// Compiled and relocatably linked by the generic arm-none-eabi package build.
// The exact same acceptance body is then executed on the physical STM32H755.
extern "C" int ccsdspack_mcu_compile_probe() {
  return CCSDSPackMcuTest::run();
}
