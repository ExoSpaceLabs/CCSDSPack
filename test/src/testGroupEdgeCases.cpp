// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include "CCSDSUtils.h"
#include "CCSDSConfig.h"
#include "CCSDSResult.h"
#include "tests.h"
#include "PusServices.h"

namespace {
  bool writePacketConfig(const std::string &path, const int apid) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
      return false;
    }
    file << "ccsds_version_number:int=0\n"
         << "ccsds_type:bool=false\n"
         << "ccsds_data_field_header_flag:bool=false\n"
         << "ccsds_APID:int=" << apid << "\n"
         << "ccsds_segmented:bool=false\n";
    return static_cast<bool>(file);
  }
}

void testGroupEdgeCases(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupEdgeCases: " << description << std::endl;

  tester->unitTest("PusB detailed deserialization check", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);
    auto& primary = pkt.getPrimaryHeader();
    TEST_VOID(primary.setAPID(0x123));
    TEST_VOID(primary.setDataFieldHeaderFlag(1));

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
    std::vector<uint8_t> shortData = {0x00, 0x01, 0x02, 0x03, 0x04};
    auto res = pkt.deserialize(shortData);
    return !res;
  });

  tester->unitTest("APID range and header status are handled explicitly", []() {
    CCSDS::Header header;
    for (const std::uint16_t apid : {0U, 255U, 256U, 2046U}) {
      TEST_VOID(header.setAPID(apid));
      if (header.getAPID() != apid || header.getHeaderStatus() != CCSDS::NORMAL) {
        return false;
      }
    }

    TEST_VOID(header.setAPID(CCSDS::IDLE_APID));
    if (header.getHeaderStatus() != CCSDS::IDLE || header.serialize().size() != 6U) {
      return false;
    }

    const auto invalid = header.setAPID(CCSDS::IDLE_APID + 1U);
    if (invalid || header.getHeaderStatus() != CCSDS::INVALID || !header.serialize().empty()) {
      return false;
    }

    TEST_VOID(header.setAPID(256U));
    return header.getHeaderStatus() == CCSDS::NORMAL && header.serialize().size() == 6U;
  });

  tester->unitTest("Packed primary header derives idle APID status", []() {
    CCSDS::Header header;
    TEST_VOID(header.setData(static_cast<std::uint64_t>(CCSDS::IDLE_APID) << 32));
    return header.getAPID() == CCSDS::IDLE_APID
           && header.getHeaderStatus() == CCSDS::IDLE
           && header.serialize().size() == 6U;
  });

  tester->unitTest("Invalid PrimaryHeader assignment cannot be serialized", []() {
    CCSDS::Packet packet;
    const auto result = packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 0, static_cast<std::uint16_t>(CCSDS::IDLE_APID + 1U),
      CCSDS::UNSEGMENTED, 0, 0
    });
    return !result && packet.serialize().empty() && packet.getPrimaryHeaderBytes().empty();
  });

  tester->unitTest("Configuration supports the complete APID range and rejects overflow", []() {
    const std::vector<int> validAPIDs{0, 255, 256, 2046, 2047};
    for (const auto apid : validAPIDs) {
      const std::string path = "ccsdspack_apid_" + std::to_string(apid) + ".cfg";
      if (!writePacketConfig(path, apid)) {
        return false;
      }
      CCSDS::Packet packet;
      const auto result = packet.loadFromConfigFile(path);
      std::remove(path.c_str());
      if (!result || packet.getPrimaryHeader().getAPID() != static_cast<std::uint16_t>(apid)) {
        return false;
      }
      const auto expectedStatus = apid == 2047 ? CCSDS::IDLE : CCSDS::NORMAL;
      if (packet.getPrimaryHeader().getHeaderStatus() != expectedStatus) {
        return false;
      }
    }

    for (const auto apid : {-1, 2048, 65536}) {
      const std::string path = "ccsdspack_invalid_apid_" + std::to_string(apid) + ".cfg";
      if (!writePacketConfig(path, apid)) {
        return false;
      }
      CCSDS::Packet packet;
      const auto result = packet.loadFromConfigFile(path);
      std::remove(path.c_str());
      if (result) {
        return false;
      }
    }
    return true;
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

  tester->unitTest("Maximum Packet Data Length value is encoded", []() {
    CCSDS::Packet packet;
    packet.setDataFieldSize(0xFFFEU);
    TEST_VOID(packet.setApplicationData(std::vector<std::uint8_t>(0xFFFEU, 0x00)));

    const auto serialized = packet.serialize();
    return serialized.size() == 0x10006U
           && serialized[4] == 0xFF
           && serialized[5] == 0xFF
           && packet.getPrimaryHeader().getDataLength() == 0xFFFFU;
  });

  tester->unitTest("Packet Data Length overflow is rejected", []() {
    CCSDS::Packet packet;
    packet.setDataFieldSize(0xFFFFU);
    TEST_VOID(packet.setApplicationData(std::vector<std::uint8_t>(0xFFFFU, 0x00)));
    return packet.serialize().empty();
  });
}
