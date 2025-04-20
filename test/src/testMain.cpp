
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

  return tester.Result();
}
