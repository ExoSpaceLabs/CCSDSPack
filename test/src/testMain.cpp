// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0


#include "tests.h"
#include <iostream>

int main() {
  std::cout << std::endl;
  std::cout << "Running Tests..." << std::endl;
  TestManager tester{};

  /// Perform unit tests:
  // perform basic packet related tests on the library
  testGroupCore(&tester, "Core CCSDS Packet features tests.");

  // perform packet validation related tests on the library
  testGroupValidator(&tester, "Validation of CCSDS Packet tests.");

  // perform packet management tests on the library
  testGroupManagement(&tester, "Management of CCSDS packet tests.");

  // perform edge cases tests on the library
  testGroupEdgeCases(&tester, "Edge cases and detailed PUS checks.");

  return tester.Result();
}
