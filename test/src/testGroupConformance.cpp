// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "tests.h"

namespace {
  bool writeConfig(const std::string &path, const int version) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;

    file << "ccsds_version_number:int=" << version << '\n'
         << "ccsds_type:bool=false\n"
         << "ccsds_data_field_header_flag:bool=false\n"
         << "ccsds_APID:int=1\n"
         << "ccsds_segmented:bool=false\n"
         << "data_field_size:int=8\n";
    return static_cast<bool>(file);
  }
}

void testGroupConformance(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupConformance: " << description << std::endl;

  tester->unitTest("Packet serialization rejects non-zero Packet Version Number", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      1, 0, 0, 1, CCSDS::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setApplicationData({0x01}));
    return packet.serialize().empty();
  });

  tester->unitTest("Configuration accepts only Packet Version Number zero", []() {
    const std::string validPath = "ccsdspack_pvn_zero.cfg";
    const std::string invalidPath = "ccsdspack_pvn_one.cfg";
    if (!writeConfig(validPath, 0) || !writeConfig(invalidPath, 1)) return false;

    CCSDS::Packet valid;
    CCSDS::Packet invalid;
    const auto validResult = valid.loadFromConfigFile(validPath);
    const auto invalidResult = invalid.loadFromConfigFile(invalidPath);
    std::remove(validPath.c_str());
    std::remove(invalidPath.c_str());

    return validResult && !invalidResult;
  });

  tester->unitTest("Idle Packet rejects a secondary header", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setDataFieldHeader({0x10}));
    TEST_VOID(packet.setApplicationData({0x00}));
    return packet.serialize().empty();
  });

  tester->unitTest("Idle Packet rejects an asserted secondary-header flag", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 1, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setApplicationData({0x00}));
    return packet.serialize().empty();
  });

  tester->unitTest("Idle Packet requires mission-defined idle user data", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
    }));
    return packet.serialize().empty();
  });

  tester->unitTest("Valid Idle Packet serializes without a secondary header", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setApplicationData({0x00}));
    const auto bytes = packet.serialize();
    return bytes.size() == 9U
           && packet.getPrimaryHeader().getHeaderStatus() == CCSDS::IDLE
           && packet.getPrimaryHeader().getDataFieldHeaderFlag() == 0U;
  });

  tester->unitTest("Maximum serialized size is reported without uint16 overflow", []() {
    CCSDS::Packet packet;
    packet.setDataFieldSize(0xFFFEU);
    TEST_VOID(packet.setApplicationData(std::vector<std::uint8_t>(0xFFFEU, 0x00)));
    const auto bytes = packet.serialize();
    return bytes.size() == 65542U
           && packet.getSerializedSize() == 65542U
           && packet.getFullPacketLength() == 0xFFFFU;
  });

  tester->unitTest("Telecommand packets use modulo-16384 sequence counts", []() {
    CCSDS::Packet packetTemplate;
    TEST_VOID(packetTemplate.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 1, 0, 0x123, CCSDS::UNSEGMENTED, 0x3FFF, 0
    }));
    packetTemplate.setDataFieldSize(1U);

    CCSDS::Manager manager;
    TEST_VOID(manager.setPacketTemplate(packetTemplate));
    TEST_VOID(manager.setSequenceCount(0x3FFFU));
    TEST_VOID(manager.setApplicationData({0xAA, 0xBB}));

    const auto packets = manager.getPackets();
    return packets.size() == 2U
           && packets[0].getPrimaryHeader().getType() == 1U
           && packets[0].getPrimaryHeader().getSequenceCount() == 0x3FFFU
           && packets[1].getPrimaryHeader().getSequenceCount() == 0U;
  });
}
