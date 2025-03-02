
#include "tests.h"
#include <iostream>

int main() {

    std::cout << std::endl;
    std::cout <<"Running Tests..." << std::endl;
    TestManager tester{};

    /// Perform unit tests:
    // perform basic packet related tests on the library
    testGroupBasic(&tester, "Basic CCSDS Packet feature tests.");

    // perform packet validation related tests on the library
    testGroupValidator(&tester, "Validation of CCSDS Packet tests.");

    // perform packet management tests on the library
    testGroupManagement(&tester, "management of CCSDS packet tests.");

    // end tests and return number of tests failed (0 in case all passed)
    return tester.Result();
}

