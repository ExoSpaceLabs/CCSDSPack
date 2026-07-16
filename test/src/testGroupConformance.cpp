// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPacket.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include "tests.h"

void testGroupConformance(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupConformance: " << description << std::endl;

  tester->unitTest("Space Packet Version Number is fixed to encoded value zero.", [] {
    CCSDS::Header header;
    const auto result = header.setVersionNumber(1U);
    return !result
           && result.error().code() == CCSDS::INVALID_HEADER_DATA
           && header.getHeaderStatus() == CCSDS::INVALID
           && header.serialize().empty();
  });

  tester->unitTest("PrimaryHeader assignment rejects non-zero Space Packet versions.", [] {
    CCSDS::Packet packet;
    const auto result = packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{1, 0, 0, 1, CCSDS::UNSEGMENTED, 0, 0});
    return !result && packet.serialize().empty();
  });

#ifndef CCSDS_MCU
  tester->unitTest("Configuration rejects a non-zero Space Packet Version Number.", [] {
    const std::string path = "ccsdspack_invalid_space_packet_version.cfg";
    {
      std::ofstream file(path, std::ios::trunc);
      if (!file) return false;
      file << "ccsds_version_number:int=1\n"
           << "ccsds_type:bool=false\n"
           << "ccsds_data_field_header_flag:bool=false\n"
           << "ccsds_APID:int=1\n"
           << "ccsds_segmented:bool=false\n";
    }

    CCSDS::Packet packet;
    const auto result = packet.loadFromConfigFile(path);
    std::remove(path.c_str());
    return !result;
  });
#endif

  tester->unitTest("Idle APID rejects a previously selected secondary header.", [] {
    CCSDS::Header header;
    TEST_VOID(header.setDataFieldHeaderFlag(1U));
    const auto result = header.setAPID(CCSDS::IDLE_APID);
    return !result
           && header.getHeaderStatus() == CCSDS::INVALID
           && header.serialize().empty();
  });

  tester->unitTest("Idle Packet rejects a secondary header selected after the APID.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
    }));
    const auto result = packet.setDataFieldHeader({0x01, 0x02});
    return !result
           && packet.getPrimaryHeader().getHeaderStatus() == CCSDS::INVALID
           && packet.serialize().empty();
  });

  tester->unitTest("Valid Idle Packet carries mission-defined idle data without a secondary header.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 7, 0
    }));
    TEST_VOID(packet.setApplicationData({0x55, 0x55}));
    const auto bytes = packet.serialize();
    return !bytes.empty()
           && packet.getPrimaryHeader().getHeaderStatus() == CCSDS::IDLE
           && packet.getPrimaryHeader().getDataFieldHeaderFlag() == 0U;
  });

  tester->unitTest("Maximum Space Packet size is available without 16-bit overflow.", [] {
    CCSDS::Packet packet;
    packet.setDataFieldSize(0xFFFEU);
    TEST_VOID(packet.setApplicationData(std::vector<std::uint8_t>(0xFFFEU, 0x00)));
    const auto bytes = packet.serialize();
    return bytes.size() == 65542U
           && packet.getSerializedSize() == 65542U
           && packet.getPrimaryHeader().getDataLength() == 0xFFFFU;
  });

  tester->unitTest("Telecommand packets use Packet Sequence Count semantics.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(CCSDS::PrimaryHeader{
      0, 1, 0, 0x123, CCSDS::UNSEGMENTED, 0x123, 0
    }));
    TEST_VOID(packet.setApplicationData({0xA5}));
    const auto bytes = packet.serialize();
    if (bytes.empty()) return false;

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(bytes));
    return decoded.getPrimaryHeader().getType() == 1U
           && decoded.getPrimaryHeader().getSequenceCount() == 0x123U;
  });
}
