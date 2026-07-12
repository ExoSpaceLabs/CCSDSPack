// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <iostream>
#include <vector>
#include "tests.h"

namespace {
  CCSDS::Packet finalizedPacket(const std::uint16_t apid,
                                const CCSDS::ESequenceFlag flags,
                                const std::uint16_t count,
                                const std::vector<std::uint8_t> &data = {1, 2, 3},
                                const CCSDS::PacketErrorControlMode mode = CCSDS::PacketErrorControlMode::CRC16) {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(mode);
    const auto headerResult = packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{0, 0, 0, apid, flags, count, 0});
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

  tester->unitTest("Validator accepts a coherent unsegmented packet.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, CCSDS::UNSEGMENTED, 0));
  });

  tester->unitTest("Validator rejects a non-zero count on the current unsegmented policy.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return !validator.validate(rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 5, 2));
  });

  tester->unitTest("Validator accepts a coherent first segment.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1));
  });

  tester->unitTest("Validator rejects a segmented packet with count zero.", [] {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return !validator.validate(rawPacketWithoutCRC(1, CCSDS::FIRST_SEGMENT, 0, 2));
  });

  tester->unitTest("Validator tracks a coherent segmented sequence.", [] {
    const auto packet1 = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1);
    const auto packet2 = finalizedPacket(1, CCSDS::CONTINUING_SEGMENT, 2);
    auto validator = validatorWithTemplate(packet1);
    return validator.validate(packet1) && validator.validate(packet2);
  });

  tester->unitTest("Validator rejects a discontinuous segmented sequence.", [] {
    const auto packet1 = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1);
    const auto packet2 = finalizedPacket(1, CCSDS::CONTINUING_SEGMENT, 3);
    auto validator = validatorWithTemplate(packet1);
    return validator.validate(packet1) && !validator.validate(packet2);
  });

  tester->unitTest("Validator accepts a packet matching its unsegmented template.", [] {
    const auto packet = finalizedPacket(0x123, CCSDS::UNSEGMENTED, 0);
    auto validator = validatorWithTemplate(packet, false, true);
    return validator.validate(packet);
  });

  tester->unitTest("Validator rejects a packet with a different template APID.", [] {
    const auto templatePacket = finalizedPacket(0x123, CCSDS::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(0x124, CCSDS::UNSEGMENTED, 0);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    return !validator.validate(packet);
  });

  tester->unitTest("Validator rejects segmented state against an unsegmented template.", [] {
    const auto templatePacket = finalizedPacket(1, CCSDS::UNSEGMENTED, 0);
    const auto packet = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1);
    auto validator = validatorWithTemplate(templatePacket, false, true);
    return !validator.validate(packet);
  });

  tester->unitTest("Validator accepts packets without packet error control.", [] {
    auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 0, {1, 2, 3},
                                  CCSDS::PacketErrorControlMode::None);
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(packet);
  });

  tester->unitTest("Validator report is fully true for a valid packet.", [] {
    const auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 0);
    auto validator = validatorWithTemplate(packet);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, true, true});
  });

  tester->unitTest("Validator report isolates a Packet Data Length failure.", [] {
    auto packet = rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 0, 7);
    auto templatePacket = rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 0, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({false, true, true, true, true, true});
  });

  tester->unitTest("Validator report isolates a CRC failure.", [] {
    auto packet = finalizedPacket(1, CCSDS::UNSEGMENTED, 0, {1, 2, 3});
    packet.setApplicationData({1, 2, 4});
    auto templatePacket = finalizedPacket(1, CCSDS::UNSEGMENTED, 0, {1, 2, 3});
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, false, true, true, true, true});
  });

  tester->unitTest("Validator report isolates sequence-control coherence.", [] {
    auto packet = rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 5, 2);
    auto templatePacket = rawPacketWithoutCRC(1, CCSDS::UNSEGMENTED, 0, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, false, true, true, true});
  });

  tester->unitTest("Validator report isolates the first-segment count rule.", [] {
    auto packet = rawPacketWithoutCRC(1, CCSDS::FIRST_SEGMENT, 5, 2);
    auto templatePacket = rawPacketWithoutCRC(1, CCSDS::FIRST_SEGMENT, 1, 2);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
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
    const auto packet = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1);
    auto validator = validatorWithTemplate(templatePacket);
    validator.validate(packet);
    return validator.getReport() == std::vector<bool>({true, true, true, true, true, false});
  });
}
