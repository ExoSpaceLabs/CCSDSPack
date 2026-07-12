// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <memory>
#include "CCSDSManager.h"
#include "CCSDSUtils.h"
#include "CCSDSResult.h"
#include "tests.h"
#include "PusServices.h"

namespace {
  CCSDS::Packet makeTemplate(const std::vector<std::uint8_t> &header) {
    CCSDS::Packet packet;
    const auto result = packet.setPrimaryHeader(header);
    if (!result) {
      std::cerr << "[ Error ]: Code [" << result.error().code() << "]: " << result.error().message() << '\n';
    }
    return packet;
  }

  std::vector<std::uint8_t> serializePackets(const std::vector<CCSDS::Packet> &packets) {
    std::vector<std::uint8_t> buffer;
    for (auto packet : packets) {
      const auto serialized = packet.serialize();
      buffer.insert(buffer.end(), serialized.begin(), serialized.end());
    }
    return buffer;
  }

  bool hasValidDefaultCRC(const std::vector<std::uint8_t> &packet) {
    if (packet.size() < 8U) return false;
    const auto expected = crc16(std::vector<std::uint8_t>(packet.begin(), packet.end() - 2));
    const auto received = static_cast<std::uint16_t>(packet[packet.size() - 2]) << 8 | packet.back();
    return expected == received;
  }
}

void testGroupManagement(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupManagement: " << description << std::endl;

  tester->unitTest("Manager shall preserve a disabled-update packet template.", []() {
    auto packet = makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00});
    CCSDS::Manager manager(packet);
    std::vector<std::uint8_t> serialized;
    TEST_RET(serialized, manager.getPacketTemplate());
    const std::vector<std::uint8_t> expected{0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
    return serialized == expected;
  });

  tester->unitTest("Manager shall create one compliant unsegmented packet.", []() {
    CCSDS::Manager manager(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    manager.setDataFieldSize(5);
    TEST_VOID(manager.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05}));

    std::vector<std::uint8_t> packet;
    TEST_RET(packet, manager.getPacketBufferAtIndex(0));
    if (manager.getTotalPackets() != 1U || packet.size() != 13U) return false;
    if (packet[4] != 0x00 || packet[5] != 0x06) return false;
    if (!hasValidDefaultCRC(packet)) return false;

    std::vector<std::uint8_t> applicationData;
    TEST_RET(applicationData, manager.getApplicationDataBuffer());
    return applicationData == std::vector<std::uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05});
  });

  tester->unitTest("Manager shall segment data and preserve sequence metadata.", []() {
    CCSDS::Manager manager;
    TEST_VOID(manager.setPacketTemplate(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00})));
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(5);
    const std::vector<std::uint8_t> input{
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x06, 0x07
    };
    TEST_VOID(manager.setApplicationData(input));

    const auto packets = manager.getPackets();
    if (packets.size() != 3U) return false;
    const auto h0 = packets[0].getPrimaryHeader();
    const auto h1 = packets[1].getPrimaryHeader();
    const auto h2 = packets[2].getPrimaryHeader();
    if (h0.getSequenceFlags() != CCSDS::FIRST_SEGMENT || h0.getSequenceCount() != 1U) return false;
    if (h1.getSequenceFlags() != CCSDS::CONTINUING_SEGMENT || h1.getSequenceCount() != 2U) return false;
    if (h2.getSequenceFlags() != CCSDS::LAST_SEGMENT || h2.getSequenceCount() != 3U) return false;
    if (h0.getDataLength() != 6U || h1.getDataLength() != 6U || h2.getDataLength() != 3U) return false;

    std::vector<std::uint8_t> reassembled;
    TEST_RET(reassembled, manager.getApplicationDataBuffer());
    return reassembled == input;
  });

  tester->unitTest("Manager shall load concatenated compliant packets.", []() {
    CCSDS::Manager producer;
    TEST_VOID(producer.setPacketTemplate(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00})));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(5);
    const std::vector<std::uint8_t> input{
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x06, 0x07
    };
    TEST_VOID(producer.setApplicationData(input));
    const auto buffer = producer.getPacketsBuffer();

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    TEST_VOID(consumer.load(buffer));
    if (consumer.getTotalPackets() != 3U) return false;

    std::vector<std::uint8_t> reassembled;
    TEST_RET(reassembled, consumer.getApplicationDataBuffer());
    return reassembled == input && consumer.getPacketsBuffer() == buffer;
  });

  tester->unitTest("Manager shall reject a truncated concatenated packet buffer.", []() {
    CCSDS::Manager producer(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    producer.setAutoValidateEnable(false);
    TEST_VOID(producer.setApplicationData({0x01, 0x02, 0x03}));
    auto buffer = producer.getPacketsBuffer();
    buffer.pop_back();

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    return !consumer.load(buffer).has_value();
  });

  tester->unitTest("Manager shall add packets as objects and buffers.", []() {
    CCSDS::Packet packet = makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00});
    TEST_VOID(packet.setApplicationData({0x10, 0x20, 0x30}));
    const auto bytes = packet.serialize();

    CCSDS::Manager manager;
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.addPacket(packet));
    TEST_VOID(manager.addPacketFromBuffer(bytes));
    return manager.getTotalPackets() == 2U
           && manager.getPackets()[0].serialize() == bytes
           && manager.getPackets()[1].serialize() == bytes;
  });

  tester->unitTest("Manager shall load a vector of packets.", []() {
    CCSDS::Packet packet1 = makeTemplate({0xF7, 0xFF, 0x40, 0x01, 0x00, 0x00});
    CCSDS::Packet packet2 = makeTemplate({0xF7, 0xFF, 0x80, 0x02, 0x00, 0x00});
    TEST_VOID(packet1.setApplicationData({0x01, 0x02}));
    TEST_VOID(packet2.setApplicationData({0x03, 0x04}));

    CCSDS::Manager manager;
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.load(std::vector<CCSDS::Packet>{packet1, packet2}));
    return manager.getTotalPackets() == 2U
           && manager.getPacketsBuffer() == serializePackets({packet1, packet2});
  });

  tester->unitTest("Manager shall insert and consume sync patterns.", []() {
    CCSDS::Manager producer;
    TEST_VOID(producer.setPacketTemplate(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00})));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(3);
    producer.setSyncPatternEnable(true);
    TEST_VOID(producer.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05}));
    const auto framed = producer.getPacketsBuffer();
    if (framed.size() < 4U || framed[0] != 0x1A || framed[1] != 0xCF
        || framed[2] != 0xFC || framed[3] != 0x1D) return false;

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    consumer.setSyncPatternEnable(true);
    TEST_VOID(consumer.load(framed));
    std::vector<std::uint8_t> data;
    TEST_RET(data, consumer.getApplicationDataBuffer());
    return data == std::vector<std::uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05});
  });

  tester->unitTest("Manager shall clear packets and template state.", []() {
    CCSDS::Manager manager(makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00}));
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.setApplicationData({0x01, 0x02, 0x03}));
    manager.clearPackets();
    if (manager.getTotalPackets() != 0U || !manager.getPackets().empty()) return false;

    TEST_VOID(manager.setApplicationData({0x04, 0x05}));
    manager.clear();
    return manager.getPackets().empty()
           && !manager.setApplicationData({0x06}).has_value();
  });

  tester->unitTest("Manager shall write and read compliant packet buffers.", []() {
    CCSDS::Manager producer;
    TEST_VOID(producer.setPacketTemplate(makeTemplate({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00})));
    producer.setAutoValidateEnable(false);
    producer.setDataFieldSize(4);
    TEST_VOID(producer.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05, 0x06}));
    const auto expected = producer.getPacketsBuffer();
    TEST_VOID(producer.write("test_resources/myPackets.bin"));

    CCSDS::Manager consumer;
    consumer.setAutoValidateEnable(false);
    TEST_VOID(consumer.read("test_resources/myPackets.bin"));
    return consumer.getPacketsBuffer() == expected;
  });

  tester->unitTest("Manager shall read a generated binary packet template.", []() {
    CCSDS::Packet packet = makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00});
    TEST_VOID(packet.setApplicationData({0xAA, 0x55}));
    const auto expected = packet.serialize();
    TEST_VOID(writeBinaryFile(expected, "test_resources/runtimeTemplate.bin"));

    CCSDS::Manager manager;
    TEST_VOID(manager.readTemplate("test_resources/runtimeTemplate.bin"));
    std::vector<std::uint8_t> actual;
    TEST_RET(actual, manager.getPacketTemplate());
    return actual == expected;
  });

#ifndef CCSDS_MCU
  tester->unitTest("Manager shall load the same template as Packet from config.", []() {
    CCSDS::Packet packet;
    TEST_VOID(packet.loadFromConfigFile("test_resources/templatePacket.cfg"));
    const auto expected = packet.serialize();

    CCSDS::Manager manager;
    TEST_VOID(manager.loadTemplateConfigFile("test_resources/templatePacket.cfg"));
    const auto actual = manager.getTemplate().serialize();
    return actual == expected;
  });
#endif

  tester->unitTest("Manager shall preserve a PUS-A secondary header.", []() {
    CCSDS::Packet packet = makeTemplate({0xF7, 0xFF, 0xC0, 0x00, 0x00, 0x00});
    PusA secondaryHeader;
    TEST_VOID(secondaryHeader.deserialize({0x02, 0x04, 0x05, 0x06, 0x0B, 0x0C}));
    packet.setDataFieldHeader(std::make_shared<PusA>(secondaryHeader));

    CCSDS::Manager manager(packet);
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.setApplicationData({0x07, 0x0A}));
    const auto packets = manager.getPackets();
    if (packets.size() != 1U) return false;
    const auto serialized = packets[0].serialize();
    return packets[0].getDataFieldHeaderFlag()
           && packets[0].getApplicationDataBytes() == std::vector<std::uint8_t>({0x07, 0x0A})
           && serialized[5] == 0x09
           && hasValidDefaultCRC(serialized);
  });

  tester->unitTest("Manager shall segment data around a PUS-C secondary header.", []() {
    CCSDS::Packet packet = makeTemplate({0xCF, 0xF4, 0x40, 0x00, 0x00, 0x00});
    PusC secondaryHeader;
    TEST_VOID(secondaryHeader.deserialize({0x02, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x00, 0x00}));
    packet.setDataFieldHeader(std::make_shared<PusC>(secondaryHeader));

    CCSDS::Manager manager(packet);
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(13);
    TEST_VOID(manager.setApplicationData({
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x01, 0x02, 0x03, 0x04, 0x05,
      0x06, 0x07
    }));
    const auto packets = manager.getPackets();
    if (packets.size() != 3U) return false;
    return packets[0].getApplicationDataBytes().size() == 5U
           && packets[1].getApplicationDataBytes().size() == 5U
           && packets[2].getApplicationDataBytes().size() == 2U
           && packets[0].getDataFieldHeaderFlag()
           && packets[1].getDataFieldHeaderFlag()
           && packets[2].getDataFieldHeaderFlag();
  });

  tester->unitTest("Manager shall support CRC-disabled packet templates.", []() {
    CCSDS::Packet packet = makeTemplate({0x00, 0x01, 0xC0, 0x00, 0x00, 0x00});
    packet.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    CCSDS::Manager manager(packet);
    manager.setAutoValidateEnable(false);
    TEST_VOID(manager.setApplicationData({0xAA, 0x55}));

    std::vector<std::uint8_t> serialized;
    TEST_RET(serialized, manager.getPacketBufferAtIndex(0));
    return serialized.size() == 8U
           && serialized[4] == 0x00
           && serialized[5] == 0x01
           && serialized[6] == 0xAA
           && serialized[7] == 0x55;
  });

  tester->unitTest("Manager shall reject data without a template or with an empty payload.", []() {
    CCSDS::Manager manager;
    if (manager.setApplicationData({0x01}).has_value()) return false;
    TEST_VOID(manager.setPacketTemplate(makeTemplate({0x00, 0x01, 0xC0, 0x00, 0x00, 0x00})));
    return !manager.setApplicationData({}).has_value();
  });

  std::cout << std::endl;
}
