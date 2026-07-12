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
  CCSDS::Packet makeTemplate(const std::uint16_t apid = 1U,
                             const CCSDS::ESequenceFlag flags = CCSDS::UNSEGMENTED,
                             const std::uint16_t count = 0U,
                             const CCSDS::PacketErrorControlMode mode = CCSDS::PacketErrorControlMode::CRC16) {
    CCSDS::Packet packet;
    packet.setPacketErrorControlMode(mode);
    const auto result = packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{0, 0, 0, apid, flags, count, 0});
    if (!result) {
      std::cerr << "[ Error ]: " << result.error().message() << '\n';
    }
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

  tester->unitTest("Manager preserves a disabled-update packet template.", [] {
    CCSDS::Manager manager(makeTemplate());
    std::vector<std::uint8_t> serialized;
    TEST_RET(serialized, manager.getPacketTemplate());
    return serialized.size() == 8U
           && serialized[0] == 0x00
           && serialized[1] == 0x01
           && serialized[2] == 0xC0
           && serialized[3] == 0x00;
  });

  tester->unitTest("Manager creates one compliant unsegmented packet.", [] {
    CCSDS::Manager manager(makeTemplate());
    manager.setDataFieldSize(5);
    TEST_VOID(manager.setApplicationData({1, 2, 3, 4, 5}));

    std::vector<std::uint8_t> packet;
    TEST_RET(packet, manager.getPacketBufferAtIndex(0));
    if (manager.getTotalPackets() != 1U || packet.size() != 13U) return false;
    if (packet[4] != 0x00 || packet[5] != 0x06 || !hasValidDefaultCRC(packet)) return false;

    std::vector<std::uint8_t> applicationData;
    TEST_RET(applicationData, manager.getApplicationDataBuffer());
    return applicationData == std::vector<std::uint8_t>({1, 2, 3, 4, 5});
  });

  tester->unitTest("Manager segments data and preserves sequence metadata.", [] {
    CCSDS::Manager manager;
    TEST_VOID(manager.setPacketTemplate(makeTemplate(1, CCSDS::FIRST_SEGMENT, 1)));
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(5);
    const std::vector<std::uint8_t> input{1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6, 7};
    TEST_VOID(manager.setApplicationData(input));

    auto packets = manager.getPackets();
    if (packets.size() != 3U) return false;
    const auto h0 = packets[0].getPrimaryHeader();
    const auto h1 = packets[1].getPrimaryHeader();
    const auto h2 = packets[2].getPrimaryHeader();
    if (h0.getSequenceFlags() != CCSDS::FIRST_SEGMENT || h0.getSequenceCount() != 1U) return false;
    if (h1.getSequenceFlags() != CCSDS::CONTINUING_SEGMENT || h1.getSequenceCount() != 2U) return false;
    if (h2.getSequenceFlags() != CCSDS::LAST_SEGMENT || h2.getSequenceCount() != 3U) return false;

    std::vector<std::uint8_t> reassembled;
    TEST_RET(reassembled, manager.getApplicationDataBuffer());
    return reassembled == input;
  });

  tester->unitTest("Manager loads concatenated compliant packets and rejects truncation.", [] {
    CCSDS::Manager producer;
    TEST_VOID(producer.setPacketTemplate(makeTemplate(1, CCSDS::FIRST_SEGMENT, 1)));
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
        || consumer.getPacketsBuffer() != buffer) return false;

    auto truncated = buffer;
    truncated.pop_back();
    CCSDS::Manager invalidConsumer;
    invalidConsumer.setAutoValidateEnable(false);
    return !invalidConsumer.load(truncated);
  });

  tester->unitTest("Manager adds and loads packet objects and buffers.", [] {
    CCSDS::Packet packet1 = makeTemplate(1, CCSDS::FIRST_SEGMENT, 1);
    CCSDS::Packet packet2 = makeTemplate(1, CCSDS::LAST_SEGMENT, 2);
    TEST_VOID(packet1.setApplicationData({1, 2}));
    TEST_VOID(packet2.setApplicationData({3, 4}));
    const auto packet1Bytes = packet1.serialize();

    CCSDS::Manager manager;
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.addPacket(packet1));
    TEST_VOID(manager.addPacketFromBuffer(packet1Bytes));
    TEST_VOID(manager.load(std::vector<CCSDS::Packet>{packet2}));
    auto packets = manager.getPackets();
    return packets.size() == 3U
           && packets[0].serialize() == packet1Bytes
           && packets[1].serialize() == packet1Bytes
           && packets[2].serialize() == packet2.serialize();
  });

  tester->unitTest("Manager inserts and consumes sync patterns.", [] {
    CCSDS::Manager producer;
    TEST_VOID(producer.setPacketTemplate(makeTemplate(1, CCSDS::FIRST_SEGMENT, 1)));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(3);
    producer.setSyncPatternEnable(true);
    TEST_VOID(producer.setApplicationData({1, 2, 3, 4, 5}));
    const auto framed = producer.getPacketsBuffer();
    if (framed.size() < 4U || framed[0] != 0x1A || framed[1] != 0xCF
        || framed[2] != 0xFC || framed[3] != 0x1D) return false;

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    consumer.setSyncPatternEnable(true);
    TEST_VOID(consumer.load(framed));
    std::vector<std::uint8_t> data;
    TEST_RET(data, consumer.getApplicationDataBuffer());
    return data == std::vector<std::uint8_t>({1, 2, 3, 4, 5});
  });

  tester->unitTest("Manager clears packet and template state.", [] {
    CCSDS::Manager manager(makeTemplate());
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.setApplicationData({1, 2, 3}));
    manager.clearPackets();
    if (manager.getTotalPackets() != 0U || !manager.getPackets().empty()) return false;
    TEST_VOID(manager.setApplicationData({4, 5}));
    manager.clear();
    return manager.getPackets().empty() && !manager.setApplicationData({6});
  });

  tester->unitTest("Manager writes and reads compliant packet buffers.", [] {
    CCSDS::Manager producer;
    TEST_VOID(producer.setPacketTemplate(makeTemplate(1, CCSDS::FIRST_SEGMENT, 1)));
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

  tester->unitTest("Manager reads generated binary and configured templates.", [] {
    CCSDS::Packet packet = makeTemplate();
    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));
    const auto expectedBinary = packet.serialize();
    const std::string path = "test_resources/runtimeTemplate.bin";
    TEST_VOID(writeBinaryFile(expectedBinary, path));

    CCSDS::Manager binaryManager;
    TEST_VOID(binaryManager.readTemplate(path));
    std::remove(path.c_str());
    std::vector<std::uint8_t> actualBinary;
    TEST_RET(actualBinary, binaryManager.getPacketTemplate());
    if (actualBinary != expectedBinary) return false;

#ifndef CCSDS_MCU
    CCSDS::Packet configuredPacket;
    TEST_VOID(configuredPacket.loadFromConfigFile("test_resources/templatePacket.cfg"));
    CCSDS::Manager configManager;
    TEST_VOID(configManager.loadTemplateConfigFile("test_resources/templatePacket.cfg"));
    return configManager.getTemplate().serialize() == configuredPacket.serialize();
#else
    return true;
#endif
  });

  tester->unitTest("Manager preserves secondary headers while segmenting.", [] {
    CCSDS::Packet packet = makeTemplate(1, CCSDS::FIRST_SEGMENT, 1);
    PusC secondaryHeader;
    TEST_VOID(secondaryHeader.deserialize({0x02, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x00, 0x00}));
    packet.setDataFieldHeader(std::make_shared<PusC>(secondaryHeader));

    CCSDS::Manager manager(packet);
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(13);
    TEST_VOID(manager.setApplicationData({1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6, 7}));
    auto packets = manager.getPackets();
    return packets.size() == 3U
           && packets[0].getApplicationDataBytes().size() == 5U
           && packets[1].getApplicationDataBytes().size() == 5U
           && packets[2].getApplicationDataBytes().size() == 2U
           && packets[0].getDataFieldHeaderFlag()
           && packets[1].getDataFieldHeaderFlag()
           && packets[2].getDataFieldHeaderFlag();
  });

  tester->unitTest("Manager supports CRC-disabled templates.", [] {
    CCSDS::Manager manager(makeTemplate(1, CCSDS::UNSEGMENTED, 0,
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
    TEST_VOID(manager.setPacketTemplate(makeTemplate()));
    return !manager.setApplicationData({});
  });

  std::cout << std::endl;
}
