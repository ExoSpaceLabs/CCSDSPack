#ifndef TESTS_H
#define TESTS_H

#include "TestManager.h"

/**
 * testGroupBasic : A group of unit tests that perform basic functionalities of the system
 *
 * @param tester
 * @param description
 */
void testGroupBasic(TestManager *tester, const std::string& description);

/**
 * testGroupManagement : A group of unit tests that perform packet validation functionalities of the system.
 *
 * @param tester
 * @param description
 */
void testGroupValidator(TestManager *tester, const std::string& description);

/**
 * testGroupManagement : A group of unit tests that perform packet management functionalities of the system.
 *
 * @param tester
 * @param description
 */
void testGroupManagement(TestManager *tester, const std::string& description);


#endif //TESTS_H
