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
#include "tests.h"

namespace {
  class TestSecondaryHeader final : public ccsds::SecondaryHeaderAbstract {
  public:
    TestSecondaryHeader() { variableLength = true; }
    explicit TestSecondaryHeader(std::vector<std::uint8_t> data) : m_data(std::move(data)) {
      variableLength = true;
    }

    [[nodiscard]] ccsds::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }
    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }
    [[nodiscard]] std::string getType() const override { return "TestSecondaryHeader"; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
    void update(ccsds::DataField *) override {}
    ccsds::ResultBool loadFromConfig(const ccsds::Config &) override { return true; }

  private:
    std::vector<std::uint8_t> m_data{};
  };

  class UpdatingSecondaryHeader final : public ccsds::SecondaryHeaderAbstract {
  public:
    UpdatingSecondaryHeader() : m_data{0x10} {}
    [[nodiscard]] ccsds::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }
    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }
    [[nodiscard]] std::string getType() const override { return "UpdatingSecondaryHeader"; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
    void update(ccsds::DataField *) override {
      if (!m_data.empty()) {
        ++m_data[0];
      }
    }
    ccsds::ResultBool loadFromConfig(const ccsds::Config &) override { return true; }

  private:
    std::vector<std::uint8_t> m_data{};
  };

  class FailingSecondaryHeader final : public ccsds::SecondaryHeaderAbstract {
  public:
    [[nodiscard]] ccsds::ResultBool deserialize(
        const std::vector<std::uint8_t> &) override { return true; }
    [[nodiscard]] std::uint16_t getSize() const override { return 1U; }
    [[nodiscard]] std::string getType() const override { return "FailingSecondaryHeader"; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return {}; }
    void update(ccsds::DataField *) override {}
    ccsds::ResultBool loadFromConfig(const ccsds::Config &) override { return true; }
  };
}

void testGroupCore(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupCore: " << description << std::endl;

  tester->unitTest("Primary header can be assigned from packed data.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(0x0000C0000001ULL));
    return packet.getPrimaryHeader64bit() == 0x0000C0000001ULL;
  });

  tester->unitTest("Primary header can be assigned from a PrimaryHeader structure.", [] {
    ccsds::Packet packet;
    const ccsds::PrimaryHeader header{0, 1, 1, 0x123, ccsds::FIRST_SEGMENT, 7, 4};
    TEST_VOID(packet.setPrimaryHeader(header));
    const auto &stored = packet.getPrimaryHeader();
    return stored.getVersionNumber() == 0
           && stored.getType() == 1
           && stored.getSecondaryHeaderFlag() == 1
           && stored.getAPID() == 0x123
           && stored.getSequenceFlags() == ccsds::FIRST_SEGMENT
           && stored.getSequenceCount() == 7
           && stored.getDataLength() == 4;
  });

  tester->unitTest("CRC is finalized only by explicit update or serialization.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setApplicationData({1, 2, 3, 4, 5}));
    if (packet.getCRC() != 0U) return false;
    TEST_VOID(packet.update());
    return packet.getApplicationDataBytes() == std::vector<std::uint8_t>({1, 2, 3, 4, 5})
           && packet.getSecondaryHeaderBytes().empty()
           && packet.getCRC() == 0x3B8D;
  });

  tester->unitTest("Application data set from a pointer round-trips.", [] {
    const std::uint8_t input[]{1, 2, 3, 4, 5};
    ccsds::Packet packet;
    TEST_VOID(packet.setApplicationData(input, 5));
    return packet.getApplicationDataBytes() == std::vector<std::uint8_t>({1, 2, 3, 4, 5});
  });

  tester->unitTest("Buffer secondary header inspection preserves current bytes.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setSecondaryHeader({1, 2}));
    TEST_VOID(packet.setApplicationData({3, 4, 5}));
    const auto dataField = packet.getFullDataFieldBytes();
    if (dataField != std::vector<std::uint8_t>({1, 2, 3, 4, 5})) return false;
    if (packet.getCRC() != 0U) return false;
    TEST_VOID(packet.update());
    return packet.getCRC() == 0x9903;
  });

  tester->unitTest("Custom secondary-header types remain registerable.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.RegisterSecondaryHeader<TestSecondaryHeader>());
    TEST_VOID(packet.setSecondaryHeader({0xAA, 0xBB, 0xCC}, "TestSecondaryHeader"));
    const ccsds::Packet &view = packet;
    const auto header = view.getSecondaryHeader();
    return header && header->getType() == "TestSecondaryHeader"
           && view.getSecondaryHeaderBytes()
              == std::vector<std::uint8_t>({0xAA, 0xBB, 0xCC});
  });

  tester->unitTest("Packet getters do not finalize dirty state.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(
      ccsds::PrimaryHeader{0, 0, 0, 1, ccsds::UNSEGMENTED, 7, 0}));
    TEST_VOID(packet.RegisterSecondaryHeader<UpdatingSecondaryHeader>());
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<UpdatingSecondaryHeader>()));
    TEST_VOID(packet.setApplicationData({0xAA}));

    const ccsds::Packet &view = packet;
    const auto headerBefore = view.getPrimaryHeaderBytes();
    const auto secondaryBefore = view.getSecondaryHeaderBytes();
    const auto crcBefore = view.getCRC();

    (void)view.getPrimaryHeader64bit();
    (void)view.getFullPacketLength();
    (void)view.getSecondaryHeaderFlag();
    (void)view.getDataField();
    (void)view.getPrimaryHeader();
    (void)view.getApplicationDataBytes();
    (void)view.getFullDataFieldBytes();
    (void)view.getCRCVectorBytes();

    return view.getPrimaryHeaderBytes() == headerBefore
           && view.getSecondaryHeaderBytes() == secondaryBefore
           && view.getCRC() == crcBefore
           && view.getPrimaryHeader().getSequenceCount() == 7U
           && view.getPrimaryHeader().getDataLength() == 0U
           && secondaryBefore == std::vector<std::uint8_t>({0x10})
           && crcBefore == 0U;
  });

  tester->unitTest("Serialization remains the explicit finalization path.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(
      ccsds::PrimaryHeader{0, 0, 0, 1, ccsds::UNSEGMENTED, 9, 0}));
    TEST_VOID(packet.RegisterSecondaryHeader<UpdatingSecondaryHeader>());
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<UpdatingSecondaryHeader>()));
    TEST_VOID(packet.setApplicationData({0xAA}));

    const auto encoded = serializedPacket(packet);
    return !encoded.empty()
           && packet.getPrimaryHeader().getSequenceCount() == 9U
           && packet.getPrimaryHeader().getDataLength() == 3U
           && packet.getSecondaryHeaderBytes() == std::vector<std::uint8_t>({0x11})
           && packet.getCRC() != 0U;
  });

  tester->unitTest("Finalization and serialization return secondary-header errors.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setSecondaryHeader(std::make_shared<FailingSecondaryHeader>()));
    TEST_VOID(packet.setApplicationData({0xAAU}));

    const auto updateResult = packet.update();
    const auto serializeResult = packet.serialize();
    return !updateResult
           && updateResult.error().code() == ccsds::INVALID_SECONDARY_HEADER_DATA
           && !serializeResult
           && serializeResult.error().code() == ccsds::INVALID_SECONDARY_HEADER_DATA;
  });

  tester->unitTest("Serialization reports a stale manual Packet Data Length.", [] {
    ccsds::Packet packet;
    packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    TEST_VOID(packet.setPrimaryHeader(
      ccsds::PrimaryHeader{0U, 0U, 0U, 1U, ccsds::UNSEGMENTED, 0U, 7U}));
    TEST_VOID(packet.setApplicationData({0xAAU}));
    packet.setUpdatePacketEnable(false);

    const auto result = packet.serialize();
    return !result && result.error().code() == ccsds::INVALID_DATA
           && result.error().message().find("Packet Data Length") != std::string::npos;
  });

  tester->unitTest("Parsed packet inspection preserves received sequence and CRC.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setPrimaryHeader(
      ccsds::PrimaryHeader{0, 0, 0, 0x123, ccsds::UNSEGMENTED, 123, 0}));
    TEST_VOID(source.setApplicationData({0xDE, 0xAD}));
    const auto encoded = serializedPacket(source);

    ccsds::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded));
    const ccsds::Packet &view = decoded;
    const auto headerBefore = view.getPrimaryHeaderBytes();
    const auto crcBefore = view.getCRC();
    (void)view.getApplicationDataBytes();
    (void)view.getFullDataFieldBytes();
    (void)view.getSecondaryHeaderBytes();

    return view.getPrimaryHeaderBytes() == headerBefore
           && view.getPrimaryHeader().getSequenceCount() == 123U
           && view.getCRC() == crcBefore
           && crcBefore != 0U;
  });

  tester->unitTest("Disabling automatic updates preserves a valid parsed packet.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setSecondaryHeader({1, 2}));
    TEST_VOID(source.setApplicationData({3, 4, 5}));
    const auto encoded = serializedPacket(source);

    ccsds::Packet decoded;
    TEST_VOID(decoded.deserialize(encoded, 2));
    decoded.setUpdatePacketEnable(false);
    return serializedPacket(decoded) == encoded;
  });

  tester->unitTest("Binary file helpers round-trip packet bytes.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setApplicationData({0xDE, 0xAD, 0xBE, 0xEF}));
    const auto encoded = serializedPacket(packet);
    const std::string path = "test_resources/core_packet.bin";
    TEST_VOID(ccsds::writeBinaryFile(encoded, path));
    std::vector<std::uint8_t> decoded;
    TEST_RET(decoded, ccsds::readBinaryFile(path));
    std::remove(path.c_str());
    return decoded == encoded;
  });
}
