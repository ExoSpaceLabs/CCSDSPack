// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <iostream>
#include <vector>
#include "tests.h"

namespace {
  CCSDS::Packet finalizedPacket(
      const std::uint16_t apid,
      const CCSDS::ESequenceFlag flags,
      const std::uint16_t count,
      const std::vector<std::uint8_t> &data = {1, 2, 3},
      const CCSDS::PacketErrorControlMode mode = CCSDS::PacketErrorControlMode::CRC16,
      const std::uint8_t version = 0U,
      const std::uint8_t type = 0U,
      const std::uint8_t dataFieldHeaderFlag = 0U) {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(mode);
    const auto headerResult = packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{version, type, dataFieldHeaderFlag, apid, flags, count, 0});
    if (!headerResult) {
      return {};
    }
    const auto dataResult = packet.setApplicationData(data);
    if (!dataResult || packet.serialize().empty()) {
      return {};
    }
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  CCSDS::Packet rawPacketWithoutCRC(const std::uint16_t apid,
                                    const CCSDS::ESequenceFlag flags,
                                    const std::uint16_t count,
                                    const std::uint16_t dataLength,
                                    const std::vector<std::uint8_t> &data = {1, 2, 3}) {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    packet.setPrimaryHeader(CCSDS::PrimaryHeader{0, 0, 0, apid, flags, count, dataLength});
    packet.setApplicationData(data);
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  CCSDS::Validator validatorWithTemplate(const CCSDS::Packet &packet,
                                          const bool coherence = true,
                                          const bool sequence = true) {
    CCSDS::Validator validator;
    validator.configure(coherence, sequence, true);
    validator.setTemplatePacket(packet);
    return validator;
  }
}

void testGroupValidator(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupValidator: " << description << std::endl;

  tester->unitTest("Validator accepts a non-zero unsegmented sequence count.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 5));
  });

  tester->unitTest("Validator accepts first segment sequence count zero.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, CCSDS::FIRST_SEGMENT, 0));
  });

  tester->unitTest("Validator tracks a coherent segmented sequence.", [] {
    const auto first = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 10);
    const auto continuing = finalizedPacket(1, CCSDS::CONTINUING_SEGMENT, 11);
    const auto last = finalizedPacket(1, CCSDS::LAST_SEGMENT, 12);
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(first)
           && validator.validate(continuing)
           && validator.validate(last);
  });

  tester->unitTest("Validator rejects continuation without an open segmented sequence.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const auto packet = finalizedPacket(1, CCSDS::CONTINUING_SEGMENT, 7);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && !report[2] && report[3];
  });

  tester->unitTest("Validator rejects an unsegmented packet before a segmented sequence is closed.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, CCSDS::FIRST_SEGMENT, 3))) return false;
    const auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 4);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && !report[2] && report[3];
  });

  tester->unitTest("Validator rejects a discontinuous sequence count.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 20))) return false;
    const auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 22);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && report[2] && !report[3];
  });

  tester->unitTest("Validator sequence count rolls over modulo 16384.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 0x3FFFU))
           && validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 0U));
  });

  tester->unitTest("Validator accepts a packet matching its template identifier.", [] {
    const auto packet = finalizedPacket(0x123, CCSDS::UNSEGMENTED, 4,
                                        {1, 2, 3}, CCSDS::PacketErrorControlMode::CRC16,
                                        0, 1, 0);
    auto validator = validatorWithTemplate(packet, false, true);
    return validator.validate(packet);
  });

  tester->unitTest("Validator rejects a packet with a different template identifier.", [] {
    const auto templatePacket = finalizedPacket(0x123, CCSDS::UNSEGMENTED, 4,
                                                {1, 2, 3}, CCSDS::PacketErrorControlMode::CRC16,
                                                0, 1, 0);
    const auto packet = finalizedPacket(0x124, CCSDS::UNSEGMENTED, 4,
                                        {1, 2, 3}, CCSDS::PacketErrorControlMode::CRC16,
                                        0, 1, 0);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && !report[4];
  });

  tester->unitTest("Validator rejects segmented state against an unsegmented template.", [] {
    const auto templatePacket = finalizedPacket(1, CCSDS::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    return !validator.validate(packet);
  });

  tester->unitTest("Validator accepts packets without packet error control.", [] {
    auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 9, {1, 2, 3},
                                  CCSDS::PacketErrorControlMode::None);
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(packet);
  });

  tester->unitTest("Validator report is fully true for a valid packet.", [] {
    const auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 9);
    auto validator = validatorWithTemplate(packet);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, true, true});
  });

  tester->unitTest("Validator report isolates a Packet Data Length failure.", [] {
    auto packet = rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 9, 7);
    auto templatePacket = rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 9, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({false, true, true, true, true, true});
  });

  tester->unitTest("Validator report isolates a CRC failure.", [] {
    auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 9, {1, 2, 3});
    packet.setApplicationData({1, 2, 4});
    auto templatePacket = finalizedPacket(1, CCSDS::UNSEGMENTED, 9, {1, 2, 3});
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, false, true, true, true, true});
  });

  tester->unitTest("Validator report isolates sequence-flag coherence.", [] {
    auto packet = rawPacketWithoutCRC(1, CCSDS::CONTINUING_SEGMENT, 5, 2);
    auto templatePacket = rawPacketWithoutCRC(1, CCSDS::FIRST_SEGMENT, 5, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, false, true, true, true});
  });

  tester->unitTest("Validator report isolates sequence-count continuity.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 5))) return false;
    validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 7));
    return validator.getReport() == std::vector<bool>({true, true, true, false, true, true});
  });

  tester->unitTest("Validator report isolates template identity.", [] {
    const auto templatePacket = finalizedPacket(1, CCSDS::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(2, CCSDS::UNSEGMENTED, 0);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, false, true});
  });

  tester->unitTest("Validator report isolates template sequence flags.", [] {
    const auto templatePacket = finalizedPacket(1, CCSDS::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 0);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, true, false});
  });
}
