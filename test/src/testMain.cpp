// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "tests.h"
#include <iostream>

int main() {
  std::cout << std::endl;
  std::cout << "Running Tests..." << std::endl;
  TestManager tester{};

  testGroupCore(&tester, "Core CCSDS Packet features tests.");
  testGroupValidator(&tester, "Validation of CCSDS Packet tests.");
  testGroupManagement(&tester, "Management of CCSDS packet tests.");
  testGroupEdgeCases(&tester, "Edge cases and detailed PUS checks.");
  testGroupParsing(&tester, "Bounded parsing, CRC validation, and header validation tests.");

  return tester.Result();
}
