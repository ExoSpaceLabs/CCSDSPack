// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "tests.h"

namespace {
  bool writeConfig(const std::string &path, const int version) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;

    file << "ccsds_version_number:int=" << version << '\n'
         << "ccsds_packet_error_control:string=crc16\n"
         << "ccsds_type:bool=false\n"
         << "ccsds_APID:int=1\n"
         << "ccsds_segmented:bool=false\n"
         << "data_field_size:int=8\n"
         << "define_secondary_header:bool=false\n";
    return static_cast<bool>(file);
  }
}

void testGroupConformance(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupConformance: " << description << std::endl;

  tester->unitTest("Packet serialization rejects non-zero Packet Version Number", []() {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(ccsds::PrimaryHeader{
      1, 0, 0, 1, ccsds::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setApplicationData({0x01}));
    const auto result = packet.serialize();
    return !result && result.error().code() == ccsds::INVALID_HEADER_DATA;
  });

  tester->unitTest("Configuration accepts only Packet Version Number zero", []() {
    const std::string validPath = "ccsdspack_pvn_zero.cfg";
    const std::string invalidPath = "ccsdspack_pvn_one.cfg";
    if (!writeConfig(validPath, 0) || !writeConfig(invalidPath, 1)) return false;

    ccsds::Packet valid;
    ccsds::Packet invalid;
    const auto validResult = valid.loadFromConfigFile(validPath);
    const auto invalidResult = invalid.loadFromConfigFile(invalidPath);
    std::remove(validPath.c_str());
    std::remove(invalidPath.c_str());

    return validResult && !invalidResult;
  });

  tester->unitTest("Configuration does not require a duplicated secondary-header flag", []() {
    const std::string path = "ccsdspack_inferred_secondary_header_flag.cfg";
    if (!writeConfig(path, 0)) return false;

    ccsds::Packet packet;
    const auto result = packet.loadFromConfigFile(path);
    std::remove(path.c_str());
    return result
           && packet.getPrimaryHeader().getSecondaryHeaderFlag() == 0U
           && !packet.getSecondaryHeader();
  });

  tester->unitTest("Directional PUS header synchronizes the secondary-header flag and Packet Type", []() {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, 1, ccsds::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setSecondaryHeader(
      std::make_shared<ccsds::pus::rev_c::TcHeader>()));
    return packet.getPrimaryHeader().getSecondaryHeaderFlag() == 1U
           && packet.getPrimaryHeader().getType() == 1U
           && packet.getDirection() == ccsds::PacketDirection::Telecommand;
  });

  tester->unitTest("Idle Packet rejects a secondary header", []() {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setSecondaryHeader({0x10}));
    TEST_VOID(packet.setApplicationData({0x00}));
    const auto result = packet.serialize();
    return !result && result.error().code() == ccsds::INVALID_HEADER_DATA;
  });

  tester->unitTest("Idle Packet rejects an asserted secondary-header flag", []() {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 1, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setApplicationData({0x00}));
    const auto result = packet.serialize();
    return !result && result.error().code() == ccsds::INVALID_HEADER_DATA;
  });

  tester->unitTest("Idle Packet requires mission-defined idle user data", []() {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
    }));
    const auto result = packet.serialize();
    return !result && result.error().code() == ccsds::INVALID_DATA;
  });

  tester->unitTest("Received Idle Packet rejects secondary-header flag", []() {
    const std::vector<std::uint8_t> bytes{
      0x0F, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00
    };
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    return !packet.deserialize(bytes);
  });

  tester->unitTest("Received Idle Packet requires idle user data", []() {
    const std::vector<std::uint8_t> header{
      0x07, 0xFF, 0xC0, 0x00, 0x00, 0x01
    };
    const auto checksum = ccsds::crc16(header);
    std::vector<std::uint8_t> bytes = header;
    bytes.push_back(static_cast<std::uint8_t>(checksum >> 8U));
    bytes.push_back(static_cast<std::uint8_t>(checksum & 0xFFU));

    ccsds::Packet packet;
    return !packet.deserialize(bytes);
  });

  tester->unitTest("Valid Idle Packet serializes without a secondary header", []() {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
    }));
    TEST_VOID(packet.setApplicationData({0x00}));
    const auto bytes = serializedPacket(packet);
    return bytes.size() == 9U
           && packet.getPrimaryHeader().getHeaderStatus() == ccsds::IDLE
           && packet.getPrimaryHeader().getSecondaryHeaderFlag() == 0U;
  });

  tester->unitTest("Maximum serialized size is reported without uint16 overflow", []() {
    ccsds::Packet packet;
    packet.setDataFieldSize(0xFFFEU);
    TEST_VOID(packet.setApplicationData(std::vector<std::uint8_t>(0xFFFEU, 0x00)));
    const auto bytes = serializedPacket(packet);
    return bytes.size() == 65542U
           && packet.getSerializedSize() == 65542U
           && packet.getFullPacketLength() == 0xFFFFU;
  });

  tester->unitTest("Telecommand packets use modulo-16384 sequence counts", []() {
    ccsds::Packet packetTemplate;
    TEST_VOID(packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
      0, 1, 0, 0x123, ccsds::UNSEGMENTED, 0x3FFF, 0
    }));
    packetTemplate.setDataFieldSize(1U);

    ccsds::Manager manager;
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