// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <iostream>
#include <vector>
#include "tests.h"

namespace {
  ccsds::Packet finalizedPacket(
      const std::uint16_t apid,
      const ccsds::ESequenceFlag flags,
      const std::uint16_t count,
      const std::vector<std::uint8_t> &data = {1, 2, 3},
      const ccsds::PacketErrorControlMode mode = ccsds::PacketErrorControlMode::CRC16,
      const std::uint8_t version = 0U,
      const std::uint8_t type = 0U,
      const std::uint8_t secondaryHeaderFlag = 0U) {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(mode);
    const auto headerResult = packet.setPrimaryHeader(
      ccsds::PrimaryHeader{version, type, secondaryHeaderFlag, apid, flags, count, 0});
    if (!headerResult) {
      return {};
    }
    const auto dataResult = packet.setApplicationData(data);
    if (!dataResult || !packet.serialize()) {
      return {};
    }
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  ccsds::Packet rawPacketWithoutCRC(const std::uint16_t apid,
                                    const ccsds::ESequenceFlag flags,
                                    const std::uint16_t count,
                                    const std::uint16_t dataLength,
                                    const std::vector<std::uint8_t> &data = {1, 2, 3}) {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    if (!packet.setPrimaryHeader(
          ccsds::PrimaryHeader{0, 0, 0, apid, flags, count, dataLength})
        || !packet.setApplicationData(data)) {
      return {};
    }
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  ccsds::Validator validatorWithTemplate(const ccsds::Packet &packet,
                                          const bool coherence = true,
                                          const bool sequence = true) {
    ccsds::Validator validator;
    validator.configure(coherence, sequence, true);
    validator.setTemplatePacket(packet);
    return validator;
  }
}

void testGroupValidator(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupValidator: " << description << std::endl;

  tester->unitTest("Validator accepts a non-zero unsegmented sequence count.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 5));
  });

  tester->unitTest("Validator accepts first segment sequence count zero.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, ccsds::FIRST_SEGMENT, 0));
  });

  tester->unitTest("Validator tracks a coherent segmented sequence.", [] {
    const auto first = finalizedPacket(1, ccsds::FIRST_SEGMENT, 10);
    const auto continuing = finalizedPacket(1, ccsds::CONTINUING_SEGMENT, 11);
    const auto last = finalizedPacket(1, ccsds::LAST_SEGMENT, 12);
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(first)
           && validator.validate(continuing)
           && validator.validate(last);
  });

  tester->unitTest("Validator rejects continuation without an open segmented sequence.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    const auto packet = finalizedPacket(1, ccsds::CONTINUING_SEGMENT, 7);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && !report[2] && report[3];
  });

  tester->unitTest("Validator rejects an unsegmented packet before a segmented sequence is closed.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, ccsds::FIRST_SEGMENT, 3))) return false;
    const auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 4);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && !report[2] && report[3];
  });

  tester->unitTest("Validator rejects a discontinuous sequence count.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 20))) return false;
    const auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 22);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && report[2] && !report[3];
  });

  tester->unitTest("Validator sequence count rolls over modulo 16384.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 0x3FFFU))
           && validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 0U));
  });

  tester->unitTest("Validator accepts a packet matching its template identifier.", [] {
    const auto packet = finalizedPacket(0x123, ccsds::UNSEGMENTED, 4,
                                        {1, 2, 3}, ccsds::PacketErrorControlMode::CRC16,
                                        0, 1, 0);
    auto validator = validatorWithTemplate(packet, false, true);
    return validator.validate(packet);
  });

  tester->unitTest("Validator rejects a packet with a different template identifier.", [] {
    const auto templatePacket = finalizedPacket(0x123, ccsds::UNSEGMENTED, 4,
                                                {1, 2, 3}, ccsds::PacketErrorControlMode::CRC16,
                                                0, 1, 0);
    const auto packet = finalizedPacket(0x124, ccsds::UNSEGMENTED, 4,
                                        {1, 2, 3}, ccsds::PacketErrorControlMode::CRC16,
                                        0, 1, 0);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    if (validator.validate(packet)) return false;
    const auto report = validator.getReport();
    return report.size() == 6U && !report[4];
  });

  tester->unitTest("Validator rejects segmented state against an unsegmented template.", [] {
    const auto templatePacket = finalizedPacket(1, ccsds::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(1, ccsds::FIRST_SEGMENT, 1);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    return !validator.validate(packet);
  });

  tester->unitTest("Validator accepts packets without packet error control.", [] {
    auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 9, {1, 2, 3},
                                  ccsds::PacketErrorControlMode::None);
    ccsds::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(packet);
  });

  tester->unitTest("Validator report is fully true for a valid packet.", [] {
    const auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 9);
    auto validator = validatorWithTemplate(packet);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, true, true});
  });

  tester->unitTest("Validator report isolates a Packet Data Length failure.", [] {
    auto packet = rawPacketWithoutCRC(1, ccsds::UNSEGMENTED, 9, 7);
    auto templatePacket = rawPacketWithoutCRC(1, ccsds::UNSEGMENTED, 9, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({false, true, true, true, true, true});
  });

  tester->unitTest("Validator report isolates a CRC failure.", [] {
    auto packet = finalizedPacket(1, ccsds::UNSEGMENTED, 9, {1, 2, 3});
    TEST_VOID(packet.setApplicationData({1, 2, 4}));
    auto templatePacket = finalizedPacket(1, ccsds::UNSEGMENTED, 9, {1, 2, 3});
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, false, true, true, true, true});
  });

  tester->unitTest("Validator report isolates sequence-flag coherence.", [] {
    auto packet = rawPacketWithoutCRC(1, ccsds::CONTINUING_SEGMENT, 5, 2);
    auto templatePacket = rawPacketWithoutCRC(1, ccsds::FIRST_SEGMENT, 5, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, false, true, true, true});
  });

  tester->unitTest("Validator report isolates sequence-count continuity.", [] {
    ccsds::Validator validator;
    validator.configure(true, true, false);
    if (!validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 5))) return false;
    validator.validate(finalizedPacket(1, ccsds::UNSEGMENTED, 7));
    return validator.getReport() == std::vector<bool>({true, true, true, false, true, true});
  });

  tester->unitTest("Validator report isolates template identity.", [] {
    const auto templatePacket = finalizedPacket(1, ccsds::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(2, ccsds::UNSEGMENTED, 0);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, false, true});
  });

  tester->unitTest("Validator report isolates template sequence flags.", [] {
    const auto templatePacket = finalizedPacket(1, ccsds::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(1, ccsds::FIRST_SEGMENT, 0);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, true, false});
  });
}
