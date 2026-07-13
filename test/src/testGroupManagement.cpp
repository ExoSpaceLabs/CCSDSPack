// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include "CCSDSManager.h"
#include "CCSDSResult.h"
#include "CCSDSUtils.h"
#include "PusServices.h"
#include "tests.h"

namespace {
  CCSDS::Packet makePacket(
      const std::uint16_t apid = 1U,
      const CCSDS::ESequenceFlag flags = CCSDS::UNSEGMENTED,
      const std::uint16_t count = 0U,
      const CCSDS::PacketErrorControlMode mode = CCSDS::PacketErrorControlMode::CRC16,
      const std::uint8_t version = 0U,
      const std::uint8_t type = 0U,
      const std::uint8_t dataFieldHeaderFlag = 0U) {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(mode);
    const auto result = packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{version, type, dataFieldHeaderFlag, apid, flags, count, 0});
    if (!result) {
      std::cerr << "[ Error ]: " << result.error().message() << '\n';
    }
    return packet;
  }

  CCSDS::Packet finalizedPacket(const std::uint16_t apid,
                                const CCSDS::ESequenceFlag flags,
                                const std::uint16_t count,
                                const std::uint8_t type = 0U) {
    auto packet = makePacket(apid, flags, count, CCSDS::PacketErrorControlMode::CRC16,
                             0U, type, 0U);
    const auto result = packet.setApplicationData({1, 2});
    if (!result || packet.serialize().empty()) {
      return {};
    }
    packet.setUpdatePacketEnable(false);
    return packet;
  }

  bool hasValidDefaultCRC(const std::vector<std::uint8_t> &packet) {
    if (packet.size() < 8U) return false;
    const auto expected = crc16(std::vector<std::uint8_t>(packet.begin(), packet.end() - 2));
    const auto received = static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(packet[packet.size() - 2]) << 8U) | packet.back());
    return expected == received;
  }
}

void testGroupManagement(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupManagement: " << description << std::endl;

  tester->unitTest("Manager advances sequence count for unsegmented packets.", [] {
    CCSDS::Manager manager(makePacket(1, CCSDS::UNSEGMENTED, 5));
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(5);

    TEST_VOID(manager.setApplicationData({1, 2, 3}));
    auto packets = manager.getPackets();
    if (packets.size() != 1U
        || packets[0].getPrimaryHeader().getSequenceFlags() != CCSDS::UNSEGMENTED
        || packets[0].getPrimaryHeader().getSequenceCount() != 5U
        || manager.getSequenceCount() != 6U) {
      return false;
    }

    TEST_VOID(manager.setApplicationData({4, 5}));
    packets = manager.getPackets();
    return packets.size() == 1U
           && packets[0].getPrimaryHeader().getSequenceCount() == 6U
           && manager.getSequenceCount() == 7U;
  });

  tester->unitTest("Unsegmented packets support explicit non-zero sequence counts.", [] {
    CCSDS::Packet packet;
    packet.setSequenceFlags(CCSDS::UNSEGMENTED);
    TEST_VOID(packet.setSequenceCount(123));
    TEST_VOID(packet.setApplicationData({0xAA}));
    const auto encoded = packet.serialize();
    return !encoded.empty()
           && packet.getPrimaryHeader().getSequenceCount() == 123U
           && (static_cast<std::uint16_t>(encoded[2] & 0x3FU) << 8U | encoded[3]) == 123U;
  });

  tester->unitTest("Manager segments data with consecutive sequence counts.", [] {
    CCSDS::Manager manager(makePacket(1, CCSDS::FIRST_SEGMENT, 10));
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(5);
    TEST_VOID(manager.setApplicationData({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));

    const auto packets = manager.getPackets();
    if (packets.size() != 3U) return false;
    const auto &h0 = packets[0].getPrimaryHeader();
    const auto &h1 = packets[1].getPrimaryHeader();
    const auto &h2 = packets[2].getPrimaryHeader();
    return h0.getSequenceFlags() == CCSDS::FIRST_SEGMENT
           && h1.getSequenceFlags() == CCSDS::CONTINUING_SEGMENT
           && h2.getSequenceFlags() == CCSDS::LAST_SEGMENT
           && h0.getSequenceCount() == 10U
           && h1.getSequenceCount() == 11U
           && h2.getSequenceCount() == 12U
           && manager.getSequenceCount() == 13U;
  });

  tester->unitTest("Manager sequence count rolls over modulo 16384.", [] {
    CCSDS::Manager manager(makePacket(1, CCSDS::FIRST_SEGMENT, 0x3FFFU));
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(2);
    TEST_VOID(manager.setApplicationData({1, 2, 3}));

    const auto packets = manager.getPackets();
    return packets.size() == 2U
           && packets[0].getPrimaryHeader().getSequenceCount() == 0x3FFFU
           && packets[1].getPrimaryHeader().getSequenceCount() == 0U
           && manager.getSequenceCount() == 1U;
  });

  tester->unitTest("Manager manual sequence mode preserves the configured count.", [] {
    CCSDS::Manager manager(makePacket());
    manager.setAutoValidateEnable(false);
    manager.setAutoSequenceCountEnable(false);
    TEST_VOID(manager.setSequenceCount(42));
    manager.setDataFieldSize(2);
    TEST_VOID(manager.setApplicationData({1, 2, 3, 4, 5}));

    const auto packets = manager.getPackets();
    if (packets.size() != 3U || manager.getSequenceCount() != 42U
        || manager.getAutoSequenceCountEnable()) {
      return false;
    }
    for (const auto &packet : packets) {
      if (packet.getPrimaryHeader().getSequenceCount() != 42U) return false;
    }
    return true;
  });

  tester->unitTest("Clearing packets resets the stream counter.", [] {
    CCSDS::Manager manager(makePacket(1, CCSDS::UNSEGMENTED, 8));
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.setApplicationData({1}));
    if (manager.getSequenceCount() != 9U) return false;
    manager.clearPackets();
    return manager.getSequenceCount() == 0U && manager.getPackets().empty();
  });

  tester->unitTest("Template-bound Manager enforces the complete packet identifier.", [] {
    CCSDS::Manager manager(makePacket(0x123, CCSDS::UNSEGMENTED, 0,
                                      CCSDS::PacketErrorControlMode::CRC16,
                                      0, 1, 1));
    manager.setAutoValidateEnable(false);

    if (!manager.addPacket(makePacket(0x123, CCSDS::FIRST_SEGMENT, 7,
                                      CCSDS::PacketErrorControlMode::CRC16,
                                      0, 1, 1))) {
      return false;
    }
    if (manager.addPacket(makePacket(0x124, CCSDS::UNSEGMENTED, 0,
                                     CCSDS::PacketErrorControlMode::CRC16,
                                     0, 1, 1))) {
      return false;
    }
    if (manager.addPacket(makePacket(0x123, CCSDS::UNSEGMENTED, 0,
                                     CCSDS::PacketErrorControlMode::CRC16,
                                     0, 0, 1))) {
      return false;
    }
    if (manager.addPacket(makePacket(0x123, CCSDS::UNSEGMENTED, 0,
                                     CCSDS::PacketErrorControlMode::CRC16,
                                     0, 1, 0))) {
      return false;
    }
    return !manager.addPacket(makePacket(0x123, CCSDS::UNSEGMENTED, 0,
                                         CCSDS::PacketErrorControlMode::CRC16,
                                         1, 1, 1));
  });

  tester->unitTest("Manager without a template binds to the first packet identifier.", [] {
    CCSDS::Manager manager;
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.addPacket(makePacket(1, CCSDS::FIRST_SEGMENT, 1,
                                           CCSDS::PacketErrorControlMode::CRC16,
                                           0, 0, 0)));
    TEST_VOID(manager.addPacket(makePacket(1, CCSDS::LAST_SEGMENT, 2,
                                           CCSDS::PacketErrorControlMode::CRC16,
                                           0, 0, 0)));
    if (manager.addPacket(makePacket(2, CCSDS::UNSEGMENTED, 3))) return false;

    manager.clear();
    TEST_VOID(manager.addPacket(makePacket(2, CCSDS::UNSEGMENTED, 0)));
    return manager.getTotalPackets() == 1U
           && manager.getPackets()[0].getPrimaryHeader().getAPID() == 2U;
  });

  tester->unitTest("Packet collection loads reject mixed identifiers transactionally.", [] {
    CCSDS::Manager manager;
    manager.setAutoValidateEnable(false);
    const std::vector<CCSDS::Packet> packets{
      makePacket(1, CCSDS::FIRST_SEGMENT, 1),
      makePacket(2, CCSDS::LAST_SEGMENT, 2)
    };
    return !manager.load(packets)
           && manager.getTotalPackets() == 0U
           && manager.getSequenceCount() == 0U;
  });

  tester->unitTest("Concatenated buffer loads reject mixed identifiers transactionally.", [] {
    auto packet1 = finalizedPacket(1, CCSDS::FIRST_SEGMENT, 1, 0);
    auto packet2 = finalizedPacket(1, CCSDS::LAST_SEGMENT, 2, 1);
    auto buffer = packet1.serialize();
    const auto bytes2 = packet2.serialize();
    buffer.insert(buffer.end(), bytes2.begin(), bytes2.end());

    CCSDS::Manager manager;
    manager.setAutoValidateEnable(false);
    return !manager.load(buffer)
           && manager.getTotalPackets() == 0U
           && manager.getSequenceCount() == 0U;
  });

  tester->unitTest("Manager creates one compliant unsegmented packet.", [] {
    CCSDS::Manager manager(makePacket());
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(5);
    TEST_VOID(manager.setApplicationData({1, 2, 3, 4, 5}));

    std::vector<std::uint8_t> packet;
    TEST_RET(packet, manager.getPacketBufferAtIndex(0));
    return manager.getTotalPackets() == 1U
           && packet.size() == 13U
           && packet[4] == 0x00
           && packet[5] == 0x06
           && hasValidDefaultCRC(packet);
  });

  tester->unitTest("Manager loads concatenated compliant packets and rejects truncation.", [] {
    CCSDS::Manager producer(makePacket(1, CCSDS::FIRST_SEGMENT, 1));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(5);
    const std::vector<std::uint8_t> input{1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6, 7};
    TEST_VOID(producer.setApplicationData(input));
    const auto buffer = producer.getPacketsBuffer();

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    TEST_VOID(consumer.load(buffer));
    std::vector<std::uint8_t> reassembled;
    TEST_RET(reassembled, consumer.getApplicationDataBuffer());
    if (consumer.getTotalPackets() != 3U || reassembled != input
        || consumer.getPacketsBuffer() != buffer) {
      return false;
    }

    auto truncated = buffer;
    truncated.pop_back();
    CCSDS::Manager invalidConsumer;
    invalidConsumer.setAutoValidateEnable(false);
    return !invalidConsumer.load(truncated);
  });

  tester->unitTest("Manager inserts and consumes sync patterns.", [] {
    CCSDS::Manager producer(makePacket(1, CCSDS::FIRST_SEGMENT, 1));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(3);
    producer.setSyncPatternEnable(true);
    TEST_VOID(producer.setApplicationData({1, 2, 3, 4, 5}));
    const auto framed = producer.getPacketsBuffer();
    if (framed.size() < 4U || framed[0] != 0x1A || framed[1] != 0xCF
        || framed[2] != 0xFC || framed[3] != 0x1D) {
      return false;
    }

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    consumer.setSyncPatternEnable(true);
    TEST_VOID(consumer.load(framed));
    std::vector<std::uint8_t> data;
    TEST_RET(data, consumer.getApplicationDataBuffer());
    return data == std::vector<std::uint8_t>({1, 2, 3, 4, 5});
  });

  tester->unitTest("Manager writes and reads compliant packet buffers.", [] {
    CCSDS::Manager producer(makePacket(1, CCSDS::FIRST_SEGMENT, 1));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(4);
    TEST_VOID(producer.setApplicationData({1, 2, 3, 4, 5, 6}));
    const auto expected = producer.getPacketsBuffer();
    const std::string path = "test_resources/myPackets.bin";
    TEST_VOID(producer.write(path));

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    TEST_VOID(consumer.read(path));
    std::remove(path.c_str());
    return consumer.getPacketsBuffer() == expected;
  });

  tester->unitTest("Manager preserves secondary headers while segmenting.", [] {
    CCSDS::Packet packet = makePacket(1, CCSDS::FIRST_SEGMENT, 1);
    PusC secondaryHeader;
    TEST_VOID(secondaryHeader.deserialize({0x02, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x00, 0x00}));
    packet.setDataFieldHeader(std::make_shared<PusC>(secondaryHeader));

    CCSDS::Manager manager(packet);
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(13);
    TEST_VOID(manager.setApplicationData({1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6, 7}));
    const auto packets = manager.getPackets();
    return packets.size() == 3U
           && packets[0].getApplicationDataBytes().size() == 5U
           && packets[1].getApplicationDataBytes().size() == 5U
           && packets[2].getApplicationDataBytes().size() == 2U
           && packets[0].getDataFieldHeaderFlag()
           && packets[1].getDataFieldHeaderFlag()
           && packets[2].getDataFieldHeaderFlag();
  });

  tester->unitTest("Manager supports CRC-disabled templates.", [] {
    CCSDS::Manager manager(makePacket(1, CCSDS::UNSEGMENTED, 0,
                                      CCSDS::PacketErrorControlMode::None));
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.setApplicationData({0xAA, 0x55}));

    std::vector<std::uint8_t> serialized;
    TEST_RET(serialized, manager.getPacketBufferAtIndex(0));
    return serialized == std::vector<std::uint8_t>({0x00, 0x01, 0xC0, 0x00,
                                                     0x00, 0x01, 0xAA, 0x55});
  });

  tester->unitTest("Manager rejects data without a template or with an empty payload.", [] {
    CCSDS::Manager manager;
    if (manager.setApplicationData({1})) return false;
    TEST_VOID(manager.setPacketTemplate(makePacket()));
    return !manager.setApplicationData({});
  });

  std::cout << std::endl;
}
