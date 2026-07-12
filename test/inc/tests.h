// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef TESTS_H
#define TESTS_H

#include "TestManager.h"

void testGroupCore(TestManager *tester, const std::string &description);
void testGroupValidator(TestManager *tester, const std::string &description);
void testGroupManagement(TestManager *tester, const std::string &description);
void testGroupEdgeCases(TestManager *tester, const std::string &description);
void testGroupParsing(TestManager *tester, const std::string &description);

#endif // TESTS_H
