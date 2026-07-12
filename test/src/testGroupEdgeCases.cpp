// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <iostream>
#include <memory>
#include "CCSDSUtils.h"
#include "CCSDSConfig.h"
#include "CCSDSResult.h"
#include "tests.h"
#include "PusServices.h"

void testGroupEdgeCases(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupEdgeCases: " << description << std::endl;

  tester->unitTest("PusB detailed deserialization check", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);
    auto& primary = pkt.getPrimaryHeader();
    primary.setAPID(0x123);
    primary.setDataFieldHeaderFlag(1);

    uint16_t eventId = 0xABCD;
    uint8_t svc = 17;
    uint8_t ssvc = 1;
    uint8_t srcId = 0x55;

    auto pusB = std::make_shared<PusB>(1, svc, ssvc, srcId, eventId, 0);
    pkt.setDataFieldHeader(pusB);
    TEST_VOID(pkt.setApplicationData({0xDE, 0xAD}));

    auto buffer = pkt.serialize();

    CCSDS::Packet pktDec;
    TEST_VOID(pktDec.deserialize(buffer, "PusB"));

    auto decSecondary = std::dynamic_pointer_cast<PusB>(pktDec.getDataField().getSecondaryHeader());
    if (!decSecondary) return false;

    if (decSecondary->getServiceType() != svc) return false;
    if (decSecondary->getServiceSubtype() != ssvc) return false;
    if (decSecondary->getSourceID() != srcId) return false;
    if (decSecondary->getEventID() != eventId) return false;
    if (decSecondary->getDataLength() != 2) return false;

    return true;
  });

  tester->unitTest("PusC variable length time code check", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);

    std::vector<uint8_t> timeCode = {0x11, 0x22, 0x33, 0x44, 0x55};
    auto pusC = std::make_shared<PusC>(1, 18, 1, 0x66, timeCode, 0);
    pkt.setDataFieldHeader(pusC);
    TEST_VOID(pkt.setApplicationData({0xAA, 0xBB}));

    auto buffer = pkt.serialize();

    CCSDS::Packet pktDec;
    // PusC size is 6 (fixed) + timeCode.size() = 11
    TEST_VOID(pktDec.deserialize(buffer, "PusC", 11));

    auto decSecondary = std::dynamic_pointer_cast<PusC>(pktDec.getDataField().getSecondaryHeader());
    if (!decSecondary) return false;

    if (decSecondary->getTimeCode() != timeCode) return false;
    if (decSecondary->getDataLength() != 2) return false;

    return true;
  });

  tester->unitTest("Large application data check (approx 1KB)", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);
    std::vector<uint8_t> largeData(1024, 0x5A);
    TEST_VOID(pkt.setApplicationData(largeData));

    auto buffer = pkt.serialize();
    if (buffer.size() < 1024 + 6 + 2) return false;

    CCSDS::Packet pktDec;
    TEST_VOID(pktDec.deserialize(buffer));

    auto decData = pktDec.getApplicationDataBytes();
    return decData == largeData;
  });

  tester->unitTest("Deserialization of too short data", []() {
    CCSDS::Packet pkt;
    std::vector<uint8_t> shortData = {0x00, 0x01, 0x02, 0x03, 0x04}; // Only 5 bytes
    auto res = pkt.deserialize(shortData);
    return !res; // Should fail
  });

  tester->unitTest("Packet error control defaults to CRC16", []() {
    CCSDS::Packet packet;
    if (packet.getPacketErrorControlMode() != CCSDS::PacketErrorControlMode::CRC16) return false;
    if (packet.getPacketErrorControlSize() != 2) return false;

    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));
    const auto serialized = packet.serialize();
    return serialized.size() == 10 && packet.getFullPacketLength() == 10;
  });

  tester->unitTest("Packet error control can be disabled", []() {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));

    const auto serialized = packet.serialize();
    if (serialized.size() != 8) return false;
    if (packet.getFullPacketLength() != 8) return false;
    if (!packet.getCRCVectorBytes().empty()) return false;
    if (packet.getCRC() != 0) return false;

    CCSDS::Packet decoded;
    decoded.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    TEST_VOID(decoded.deserialize(serialized));
    return decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0xAA, 0x55});
  });

  tester->unitTest("CRC16 mode still strips the received error control field", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setApplicationData({0x10, 0x20, 0x30}));
    const auto serialized = packet.serialize();

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(serialized));
    if (decoded.getPacketErrorControlMode() != CCSDS::PacketErrorControlMode::CRC16) return false;
    return decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x10, 0x20, 0x30});
  });

  tester->unitTest("Packet Data Length and CRC16 match independent reference vector", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));

    const std::vector<std::uint8_t> expected{
      0x00, 0x00, 0xC0, 0x00, 0x00, 0x03, 0xAA, 0x55, 0x2E, 0xBB
    };

    const auto serialized = packet.serialize();
    return serialized == expected && packet.getPrimaryHeader().getDataLength() == 3U;
  });

  tester->unitTest("Packet Data Length excludes absent packet error control", []() {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));

    const std::vector<std::uint8_t> expected{
      0x00, 0x00, 0xC0, 0x00, 0x00, 0x01, 0xAA, 0x55
    };

    const auto serialized = packet.serialize();
    return serialized == expected && packet.getPrimaryHeader().getDataLength() == 1U;
  });

  tester->unitTest("CRC16 covers primary header, secondary header and application data", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setDataFieldHeader({0x01, 0x02, 0x03}));
    TEST_VOID(packet.setApplicationData({0x04, 0x05}));

    const std::vector<std::uint8_t> expected{
      0x08, 0x00, 0xC0, 0x00, 0x00, 0x06,
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x99, 0x03
    };

    return packet.serialize() == expected;
  });

  tester->unitTest("Configured CRC16 parameters remain effective", []() {
    CCSDS::Packet packet;
    packet.setCrcConfig(0x1021, 0x0000, 0x0000);
    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));

    const std::vector<std::uint8_t> expected{
      0x00, 0x00, 0xC0, 0x00, 0x00, 0x03, 0xAA, 0x55, 0x1F, 0x85
    };

    return packet.serialize() == expected;
  });

  tester->unitTest("Packet Data Length overflow is rejected", []() {
    CCSDS::Packet packet;
    packet.setDataFieldSize(0xFFFFU);
    TEST_VOID(packet.setApplicationData(std::vector<std::uint8_t>(0xFFFFU, 0x00)));
    return packet.serialize().empty();
  });
}
