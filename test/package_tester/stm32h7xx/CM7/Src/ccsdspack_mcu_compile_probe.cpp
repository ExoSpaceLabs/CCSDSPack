// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_mcu_test.h"

// This target is compiled, but not executed, by the generic arm-none-eabi build.
// It catches public-header, CCSDS_MCU, C++17, and consumer API regressions before
// the same validation core is built and run inside the STM32CubeIDE application.
extern "C" int ccsdspack_mcu_compile_probe() {
  return CCSDSPackMcuTest::run();
}
