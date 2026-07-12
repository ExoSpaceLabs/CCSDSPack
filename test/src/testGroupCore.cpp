// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include "CCSDSResult.h"
#include "CCSDSUtils.h"
#include "PusServices.h"
#include "tests.h"

namespace {
  class TestSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
  public:
    TestSecondaryHeader() { variableLength = true; }
    explicit TestSecondaryHeader(std::vector<std::uint8_t> data) : m_data(std::move(data)) {
      variableLength = true;
    }

    [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }
    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }
    [[nodiscard]] std::string getType() const override { return "TestSecondaryHeader"; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
    void update(CCSDS::DataField *) override {}
    CCSDS::ResultBool loadFromConfig(const Config &) override { return true; }

  private:
    std::vector<std::uint8_t> m_data{};
  };

  class UpdatingSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
  public:
    UpdatingSecondaryHeader() : m_data{0x10} {}
    [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }
    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }
    [[nodiscard]] std::string getType() const override { return "UpdatingSecondaryHeader"; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
    void update(CCSDS::DataField *) override {
      if (!m_data.empty()) {
        ++m_data[0];
      }
    }
    CCSDS::ResultBool loadFromConfig(const Config &) override { return true; }

  private:
    std::vector<std::uint8_t> m_data{};
  };
}

void testGroupCore(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupCore: " << description << std::endl;

  tester->unitTest("Primary header can be assigned from packed data.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(0x0000C0000001ULL));
    return packet.getPrimaryHeader64bit() == 0x0000C0000001ULL;
  });

  tester->unitTest("Primary header can be assigned from a PrimaryHeader structure.", [] {
    CCSDS::Packet packet;
    const CCSDS::PrimaryHeader header{0, 1, 1, 0x123, CCSDS::FIRST_SEGMENT, 7, 4};
    TEST_VOID(packet.setPrimaryHeader(header));
    const auto &stored = packet.getPrimaryHeader();
    return stored.getVersionNumber() == 0
           && stored.getType() == 1
           && stored.getDataFieldHeaderFlag() == 1
           && stored.getAPID() == 0x123
           && stored.getSequenceFlags() == CCSDS::FIRST_SEGMENT
           && stored.getSequenceCount() == 7
           && stored.getDataLength() == 4;
  });

  tester->unitTest("CRC is finalized only by explicit update or serialization.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setApplicationData({1, 2, 3, 4, 5}));
    if (packet.getCRC() != 0U) return false;
    packet.update();
    return packet.getApplicationDataBytes() == std::vector<std::uint8_t>({1, 2, 3, 4, 5})
           && packet.getDataFieldHeaderBytes().empty()
           && packet.getCRC() == 0x3B8D;
  });

  tester->unitTest("Application data set from a pointer round-trips.", [] {
    const std::uint8_t input[]{1, 2, 3, 4, 5};
    CCSDS::Packet packet;
    TEST_VOID(packet.setApplicationData(input, 5));
    return packet.getApplicationDataBytes() == std::vector<std::uint8_t>({1, 2, 3, 4, 5});
  });

  tester->unitTest("Buffer secondary header inspection preserves current bytes.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setDataFieldHeader({1, 2}));
    TEST_VOID(packet.setApplicationData({3, 4, 5}));
    const auto dataField = packet.getFullDataFieldBytes();
    if (dataField != std::vector<std::uint8_t>({1, 2, 3, 4, 5})) return false;
    if (packet.getCRC() != 0U) return false;
    packet.update();
    return packet.getCRC() == 0x9903;
  });

  tester->unitTest("Custom secondary-header types remain registerable.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.RegisterSecondaryHeader<TestSecondaryHeader>());
    TEST_VOID(packet.setDataFieldHeader({0xAA, 0xBB, 0xCC}, "TestSecondaryHeader"));
    return packet.getDataFieldHeaderBytes() == std::vector<std::uint8_t>({0xAA, 0xBB, 0xCC});
  });

  tester->unitTest("Packet getters do not finalize dirty state.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{0, 0, 0, 1, CCSDS::UNSEGMENTED, 7, 0}));
    packet.setDataFieldHeader(std::make_shared<UpdatingSecondaryHeader>());
    TEST_VOID(packet.setApplicationData({0xAA}));

    const CCSDS::Packet &view = packet;
    const auto headerBefore = view.getPrimaryHeaderBytes();
    const auto secondaryBefore = view.getDataFieldHeaderBytes();
    const auto crcBefore = view.getCRC();

    (void)view.getPrimaryHeader64bit();
    (void)view.getFullPacketLength();
    (void)view.getDataFieldHeaderFlag();
    (void)view.getDataField();
    (void)view.getPrimaryHeader();
    (void)view.getApplicationDataBytes();
    (void)view.getFullDataFieldBytes();
    (void)view.getCRCVectorBytes();

    return view.getPrimaryHeaderBytes() == headerBefore
           && view.getDataFieldHeaderBytes() == secondaryBefore
           && view.getCRC() == crcBefore
           && view.getPrimaryHeader().getSequenceCount() == 7U
           && view.getPrimaryHeader().getDataLength() == 0U
           && secondaryBefore == std::vector<std::uint8_t>({0x10})
           && crcBefore == 0U;
  });

  tester->unitTest("Serialization remains the explicit finalization path.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(
      CCSDS::PrimaryHeader{0, 0, 0, 1, CCSDS::UNSEGMENTED, 9, 0}));
    packet.setDataFieldHeader(std::make_shared<UpdatingSecondaryHeader>());
    TEST_VOID(packet.setApplicationData({0xAA}));

    const auto encoded = packet.serialize();
    return !encoded.empty()
           && packet.getPrimaryHeader().getSequenceCount() == 9U
           && packet.getPrimaryHeader().getDataLength() == 3U
           && packet.getDataFieldHeaderBytes() == std::vector<std::uint8_t>({0x11})
           && packet.getCRC() != 0U;
  });

  tester->unitTest("Parsed packet inspection preserves received sequence and CRC.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setPrimaryHeader(
      CCSDS::PrimaryHeader{0, 0, 0, 0x123, CCSDS::UNSEGMENTED, 123, 0}));
    TEST_VOID(source.setApplicationData({0xDE, 0xAD}));
    const auto encoded = source.serialize();

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded));
    const CCSDS::Packet &view = decoded;
    const auto headerBefore = view.getPrimaryHeaderBytes();
    const auto crcBefore = view.getCRC();
    (void)view.getApplicationDataBytes();
    (void)view.getFullDataFieldBytes();
    (void)view.getDataFieldHeaderBytes();

    return view.getPrimaryHeaderBytes() == headerBefore
           && view.getPrimaryHeader().getSequenceCount() == 123U
           && view.getCRC() == crcBefore
           && crcBefore != 0U;
  });

  tester->unitTest("PUS-A packet uses a valid generated CRC during round-trip.", [] {
    CCSDS::Packet source;
    source.setDataFieldHeader(std::make_shared<PusA>(1, 2, 3, 4, 5));
    TEST_VOID(source.setApplicationData({0x10, 0x20}));
    const auto encoded = source.serialize();

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded, "PusA"));
    const auto secondary = std::dynamic_pointer_cast<PusA>(decoded.getDataField().getSecondaryHeader());
    return secondary != nullptr
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x10, 0x20})
           && decoded.serialize() == encoded;
  });

  tester->unitTest("PUS-B packet uses a valid generated CRC during round-trip.", [] {
    CCSDS::Packet source;
    source.setDataFieldHeader(std::make_shared<PusB>(1, 2, 3, 4, 5, 6));
    TEST_VOID(source.setApplicationData({0x30, 0x40}));
    const auto encoded = source.serialize();

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded, "PusB"));
    const auto secondary = std::dynamic_pointer_cast<PusB>(decoded.getDataField().getSecondaryHeader());
    return secondary != nullptr
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x30, 0x40})
           && decoded.serialize() == encoded;
  });

  tester->unitTest("PUS-C packet uses a valid generated CRC during round-trip.", [] {
    CCSDS::Packet source;
    source.setDataFieldHeader(std::make_shared<PusC>(1, 2, 3, 4,
                                                      std::vector<std::uint8_t>{0x00, 0xBF, 0x00, 0xBF}, 6));
    TEST_VOID(source.setApplicationData({0x50, 0x60}));
    const auto encoded = source.serialize();

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded, "PusC", 10));
    const auto secondary = std::dynamic_pointer_cast<PusC>(decoded.getDataField().getSecondaryHeader());
    return secondary != nullptr
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x50, 0x60})
           && decoded.serialize() == encoded;
  });

  tester->unitTest("Disabling automatic updates preserves a valid parsed packet.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setDataFieldHeader({1, 2}));
    TEST_VOID(source.setApplicationData({3, 4, 5}));
    const auto encoded = source.serialize();

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded, 2));
    decoded.setUpdatePacketEnable(false);
    return decoded.serialize() == encoded;
  });

  tester->unitTest("Binary file helpers round-trip packet bytes.", [] {
    CCSDS::Packet packet;
    TEST_VOID(packet.setApplicationData({0xDE, 0xAD, 0xBE, 0xEF}));
    const auto encoded = packet.serialize();
    const std::string path = "test_resources/core_packet.bin";
    TEST_VOID(writeBinaryFile(encoded, path));
    std::vector<std::uint8_t> decoded;
    TEST_RET(decoded, readBinaryFile(path));
    std::remove(path.c_str());
    return decoded == encoded;
  });
}
