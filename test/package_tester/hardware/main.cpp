// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack_hardware_test.h"

#include <iostream>

int main() {
  const int result = CCSDSPackHardwareTest::run();
  if (result != CCSDSPackHardwareTest::Pass) {
    std::cerr << "CCSDSPACK_HARDWARE_TEST:FAIL:" << result << '\n';
    return result;
  }

  std::cout << "CCSDSPACK_HARDWARE_TEST:PASS\n";
  return 0;
}
