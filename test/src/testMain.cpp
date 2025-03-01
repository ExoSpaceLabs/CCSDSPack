
#include "tests.h"

int main() {


    std::cout << std::endl;
    std::cout <<"Running Tests..." << std::endl;
    TestManager tester{};

    /// Perform unit tests:
    // perform basic packet related tests on the library
    testGroupBasic(&tester, "Basic CCSDS Packet feature tests.");

    // perform packet management tests on the library
    testGroupManagement(&tester, "management of CCSDS packet tests.");

    // end tests and return number of tests failed (0 in case all passed)
    return tester.Result();

}

