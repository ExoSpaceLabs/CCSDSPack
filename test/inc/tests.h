// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef TESTS_H
#define TESTS_H

#include <CCSDSManager.h>
#include <iostream>
#include <vector>
#include "TestManager.h"

inline std::vector<std::uint8_t> serializedPacket(ccsds::Packet &packet) {
  const auto result = packet.serialize();
  if (!result) {
    std::cerr << "[ Error ]: Code [" << result.error().code()
              << "]: " << result.error().message() << '\n';
    return {};
  }
  return result.value();
}

inline std::vector<std::uint8_t> serializedPackets(const ccsds::Manager &manager) {
  const auto result = manager.getPacketsBuffer();
  if (!result) {
    std::cerr << "[ Error ]: Code [" << result.error().code()
              << "]: " << result.error().message() << '\n';
    return {};
  }
  return result.value();
}

void testGroupCore(TestManager *tester, const std::string &description);
void testGroupValidator(TestManager *tester, const std::string &description);
void testGroupManagement(TestManager *tester, const std::string &description);
void testGroupEdgeCases(TestManager *tester, const std::string &description);
void testGroupParsing(TestManager *tester, const std::string &description);
void testGroupConformance(TestManager *tester, const std::string &description);
void testGroupPus(TestManager *tester, const std::string &description);
void testGroupBuffer(TestManager *tester, const std::string &description);
void testGroupEvidence(TestManager *tester, const std::string &description);

#endif // TESTS_H
