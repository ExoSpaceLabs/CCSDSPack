// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <iostream>
#include "CCSDSUtils.h"
#include "CCSDSResult.h"
#include "tests.h"

namespace {
  const std::vector<std::uint8_t> VALIDATOR_PAYLOAD{
    0x0B, 0x01, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x00, 0x03, 0x03, 0x04, 0x05
  };

  std::vector<std::uint8_t> makePacket(const std::uint8_t identificationMSB,
                                       const std::uint8_t identificationLSB,
                                       const std::uint8_t sequenceMSB,
                                       const std::uint8_t sequenceLSB,
                                       const std::vector<std::uint8_t> &payload = VALIDATOR_PAYLOAD) {
    const auto packetDataLength = static_cast<std::uint16_t>(payload.size() + 1U);
    std::vector<std::uint8_t> packet{
      identificationMSB,
      identificationLSB,
      sequenceMSB,
      sequenceLSB,
      static_cast<std::uint8_t>((packetDataLength >> 8) & 0xFFU),
      static_cast<std::uint8_t>(packetDataLength & 0xFFU)
    };
    packet.insert(packet.end(), payload.begin(), payload.end());
    const auto checksum = crc16(packet);
    packet.push_back(static_cast<std::uint8_t>((checksum >> 8) & 0xFFU));
    packet.push_back(static_cast<std::uint8_t>(checksum & 0xFFU));
    return packet;
  }

  void setDeclaredLengthAndRefreshCRC(std::vector<std::uint8_t> &packet, const std::uint16_t length) {
    packet[4] = static_cast<std::uint8_t>((length >> 8) & 0xFFU);
    packet[5] = static_cast<std::uint8_t>(length & 0xFFU);
    packet.resize(packet.size() - 2U);
    const auto checksum = crc16(packet);
    packet.push_back(static_cast<std::uint8_t>((checksum >> 8) & 0xFFU));
    packet.push_back(static_cast<std::uint8_t>(checksum & 0xFFU));
  }

  CCSDS::Packet deserializePacket(const std::vector<std::uint8_t> &bytes) {
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    const auto result = packet.deserialize(bytes);
    if (!result) {
      std::cerr << "[ Error ]: Code [" << result.error().code() << "]: " << result.error().message() << '\n';
    }
    return packet;
  }

  CCSDS::Packet makeTemplate(const std::vector<std::uint8_t> &header) {
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    const auto result = packet.setPrimaryHeader(header);
    if (!result) {
      std::cerr << "[ Error ]: Code [" << result.error().code() << "]: " << result.error().message() << '\n';
    }
    return packet;
  }
}

void testGroupValidator(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupValidator: " << description << std::endl;

  tester->unitTest("Validator UNSEGMENTED Packet coherence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0xC0, 0x00)));
  });

  tester->unitTest("Validator UNSEGMENTED Packet coherence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    auto bytes = makePacket(0xFF, 0xFF, 0xC0, 0x00);
    bytes.push_back(0x01);
    bytes.push_back(0x02);
    return !validator.validate(deserializePacket(bytes));
  });

  tester->unitTest("Validator SEGMENTED Packet coherence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0x40, 0x01)));
  });

  tester->unitTest("Validator SEGMENTED Packet coherence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return !validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0x40, 0x00)));
  });

  tester->unitTest("Validator SEGMENTED Packet count coherence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0x40, 0x01)));
  });

  tester->unitTest("Validator SEGMENTED Packet count coherence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return !validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0x40, 0x03)));
  });

  tester->unitTest("Validator UNSEGMENTED Packet against Template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    return validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0xC0, 0x00)));
  });

  tester->unitTest("Validator UNSEGMENTED Packet against Template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    return !validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0xC0, 0x00)));
  });

  tester->unitTest("Validator SEGMENTED Packet against Template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    return validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x05)));
  });

  tester->unitTest("Validator SEGMENTED Packet against Template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);
    validator.setTemplatePacket(makeTemplate({0xFF, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    return !validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0xC0, 0x00)));
  });

  tester->unitTest("Validator UNSEGMENTED Packet coherence & against template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    return validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0xC0, 0x00)));
  });

  tester->unitTest("Validator UNSEGMENTED Packet coherence & against template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    return !validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0xC0, 0x05)));
  });

  tester->unitTest("Validator SEGMENTED Packet coherence & against template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    return validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x01)));
  });

  tester->unitTest("Validator SEGMENTED Packet coherence & against template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    return !validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x00)));
  });

  tester->unitTest("Validator SEGMENTED Packets sequence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    const auto packet1 = deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x01));
    const auto packet2 = deserializePacket(makePacket(0xF7, 0xFF, 0x00, 0x02));
    return validator.validate(packet1) && validator.validate(packet2);
  });

  tester->unitTest("Validator SEGMENTED Packets sequence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    const auto packet1 = deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x01));
    const auto packet2 = deserializePacket(makePacket(0xF7, 0xFF, 0x00, 0x03));
    return validator.validate(packet1) && !validator.validate(packet2);
  });

  tester->unitTest("Validator supports packets without packet error control.", []() {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    TEST_VOID(packet.setApplicationData({0x01, 0x02, 0x03}));
    const auto bytes = packet.serialize();

    CCSDS::Packet decoded;
    decoded.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    decoded.setUpdatePacketEnable(false);
    TEST_VOID(decoded.deserialize(bytes));

    CCSDS::Validator validator;
    validator.configure(true, true, false);
    return validator.validate(decoded);
  });

  tester->unitTest("Validator report, shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    const auto packet = deserializePacket(makePacket(0xF7, 0xFF, 0xC0, 0x00));
    const std::vector<bool> expected{true, true, true, true, true, true};
    validator.validate(packet);
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fail on data field length.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    auto bytes = makePacket(0xF7, 0xFF, 0xC0, 0x00);
    setDeclaredLengthAndRefreshCRC(bytes, 0x000EU);
    const std::vector<bool> expected{false, true, true, true, true, true};
    validator.validate(deserializePacket(bytes));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fail on CRC.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    auto bytes = makePacket(0xF7, 0xFF, 0xC0, 0x00);
    bytes.back() ^= 0x01U;
    const std::vector<bool> expected{true, false, true, true, true, true};
    validator.validate(deserializePacket(bytes));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fail on sequence control.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    const std::vector<bool> expected{true, true, false, true, true, true};
    validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0xC0, 0x05)));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fail on sequence control counter.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    const std::vector<bool> expected{true, true, true, false, true, true};
    validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x05)));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fail on Identification and version.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xF7, 0x5F, 0xC0, 0x00, 0x00, 0x00}));
    const std::vector<bool> expected{true, true, true, true, false, true};
    validator.validate(deserializePacket(makePacket(0xF7, 0xFF, 0xC0, 0x00)));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fail on header flag.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xFF, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    const std::vector<bool> expected{true, true, true, true, true, false};
    validator.validate(deserializePacket(makePacket(0xFF, 0xFF, 0xC0, 0x00)));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator report, shall fully fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    validator.setTemplatePacket(makeTemplate({0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    auto bytes = makePacket(0xF7, 0xFF, 0x40, 0x00);
    bytes[5] = 0x05;
    bytes.back() ^= 0x01U;
    const std::vector<bool> expected{false, false, false, false, false, false};
    validator.validate(deserializePacket(bytes));
    return validator.getReport() == expected;
  });

  tester->unitTest("Validator shall clear its variables.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);
    const auto templatePacket = makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00});
    validator.setTemplatePacket(templatePacket);
    const auto packet1 = deserializePacket(makePacket(0xF7, 0xFF, 0x40, 0x01));
    const auto packet2 = deserializePacket(makePacket(0xF7, 0xFF, 0x00, 0x02));
    if (!validator.validate(packet1) || !validator.validate(packet2)) return false;
    validator.clear();
    validator.setTemplatePacket(templatePacket);
    return validator.validate(packet1) && validator.validate(packet2);
  });

  std::cout << std::endl;
}
