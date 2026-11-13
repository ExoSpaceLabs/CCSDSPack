// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSValidator.h>
#include "PusSecondaryHeaders.h"
#include "ccsdspack/c/packet_view.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <limits>
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

  class GrowingSecondaryHeader final : public ccsds::SecondaryHeaderAbstract {
  public:
    [[nodiscard]] ccsds::ResultBool deserialize(
        const std::vector<std::uint8_t> &data) override {
      m_bytes = data;
      return true;
    }
    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_bytes.size());
    }
    [[nodiscard]] std::string getType() const override { return "GrowingSecondaryHeader"; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_bytes; }
    void update(ccsds::DataField *) override {
      m_bytes.push_back(0x42U);
    }
    ccsds::ResultBool loadFromConfig(const ccsds::Config &) override { return true; }
  private:
    std::vector<std::uint8_t> m_bytes{0x10U};
  };

  bool coreCrcValid(const std::vector<std::uint8_t> &bytes) {
    ccsds_packet_view_t view{};
    return ccsds_packet_view_parse(
      bytes.data(), bytes.size(), CCSDS_PACKET_ERROR_CONTROL_CRC16,
      nullptr, &view) == CCSDS_STATUS_OK && view.consumed == bytes.size();
  }

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

  tester->unitTest("Diagnostic bit formatting bounds shifts and spacing.", [] {
    return ccsds::getBinaryString(0xFFFFFFFFU, 33)
             == "1111 1111 1111 1111 1111 1111 1111 1111"
           && ccsds::getBinaryString(0xABU, 5) == "1010 1011"
           && ccsds::getBinaryString(0x1U, -1).empty()
           && ccsds::getBinaryString(0x1U, std::numeric_limits<std::int32_t>::max())
                == "0000 0000 0000 0000 0000 0000 0000 0001"
           && ccsds::getBitsSpaces(-1).empty()
           && ccsds::getBitsSpaces(3) == "   ";
  });

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

  tester->unitTest("Shrinking data-field capacity never wraps available bytes.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setApplicationData({1U, 2U, 3U, 4U}));
    packet.setDataFieldSize(2U);
    const auto &field = packet.getDataField();
    if (field.getDataFieldAbsoluteBytesSize() != 2U
        || field.getDataFieldUsedBytesSize() != 4U
        || field.getDataFieldAvailableBytesSize() != 0U
        || packet.getDataFieldMaximumSize() != 0U) return false;
    const auto serialized = packet.serialize();
    return !serialized;
  });

  tester->unitTest("Manager refuses zero remaining segmentation capacity.", [] {
    ccsds::Manager manager;
    ccsds::Packet templatePacket;
    TEST_VOID(templatePacket.setApplicationData({1U, 2U, 3U, 4U}));
    TEST_VOID(manager.setPacketTemplate(templatePacket));
    manager.setDataFieldSize(2U);
    if (manager.getDataFieldSize() != 0U) return false;
    const auto result = manager.setApplicationData({0xABU, 0xCDU});
    return !result && result.error().code() == ccsds::INVALID_APPLICATION_DATA;
  });

  tester->unitTest("Capacity below a secondary header also saturates.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setSecondaryHeader({0x01U, 0x02U, 0x03U}));
    TEST_VOID(packet.setApplicationData({0xAAU}));
    packet.setDataFieldSize(1U);
    return packet.getDataField().getDataFieldAvailableBytesSize() == 0U
           && packet.getDataFieldMaximumSize() == 0U;
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

  tester->unitTest("Unaliased Packet cache invalidates through normal setters.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setApplicationData({0x10U, 0x20U}));
    TEST_VOID(packet.update());
    const auto cached1 = packet.serialize();
    const auto cached2 = packet.serialize();
    if (!cached1 || !cached2 || cached1.value() != cached2.value()
        || !coreCrcValid(cached1.value())) return false;

    // No mutable reference has escaped. Invalidating through the owning
    // Packet setter must still force C11 re-finalization.
    TEST_VOID(packet.setApplicationData({0x10U, 0x21U}));
    const auto changed = packet.serialize();
    return changed && coreCrcValid(changed.value())
        && changed.value() != cached1.value();
  });

  tester->unitTest("Current mutable Header/DataField/PUS values always re-finalize CRC.", [] {
    ccsds::Packet packet;
    auto pusHeader = std::make_shared<ccsds::pus::rev_c::TcHeader>();
    TEST_VOID(packet.setSecondaryHeader(pusHeader));
    TEST_VOID(packet.setApplicationData({0xAAU}));
    const auto first = packet.serialize();
    if (!first || !coreCrcValid(first.value())) return false;

    TEST_VOID(packet.getPrimaryHeader().setAPID(0x123U));
    const auto second = packet.serialize();
    if (!second || !coreCrcValid(second.value())
        || first.value() == second.value()) return false;

    TEST_VOID(packet.getDataField().setApplicationData(std::vector<std::uint8_t>{0xBBU}));
    const auto third = packet.serialize();
    if (!third || !coreCrcValid(third.value())
        || second.value() == third.value()) return false;

    pusHeader->setServiceType(5U);
    const auto fourth = packet.serialize();
    if (!fourth || !coreCrcValid(fourth.value())
        || third.value() == fourth.value()) return false;

    TEST_VOID(packet.getPrimaryHeader().setSequenceCount(0x3FFFU));
    const auto fifth = packet.serialize();
    ccsds_packet_view_t view{};
    return fifth && coreCrcValid(fifth.value())
        && ccsds_packet_view_parse(
             fifth.value().data(), fifth.value().size(),
             CCSDS_PACKET_ERROR_CONTROL_CRC16, nullptr, &view) == CCSDS_STATUS_OK
        && view.primary_header.sequence_count == 0x3FFFU
        && view.primary_header.apid == 0x123U;
  });

  tester->unitTest("Manual Packet serialization rejects stale same-length CRC.", [] {
    ccsds::Packet sender;
    TEST_VOID(sender.setApplicationData({0x11U, 0x22U}));
    const auto original = sender.serialize();
    if (!original) return false;

    ccsds::Packet packet;
    TEST_VOID(packet.deserialize(original.value()));
    packet.setUpdatePacketEnable(false);
    const auto unchanged = packet.serialize();
    if (!unchanged || unchanged.value() != original.value()) return false;

    TEST_VOID(packet.getDataField().setApplicationData(
      std::vector<std::uint8_t>{0x11U, 0x23U}));
    const auto stale = packet.serialize();
    if (stale || stale.error().code() != ccsds::INVALID_CHECKSUM
        || packet.getCRC() != sender.getCRC()) return false;

    // An explicit update is a checked finalization even in manual mode.
    TEST_VOID(packet.update());
    const auto corrected = packet.serialize();
    return corrected && corrected.value() != original.value()
        && coreCrcValid(corrected.value());
  });

  tester->unitTest("Const packet snapshot recomputes wire without mutating packet state.", [] {
    ccsds::Packet packet;
    TEST_VOID(packet.setApplicationData({0x11U}));
    const auto original = packet.serialize();
    if (!original) return false;
    packet.setUpdatePacketEnable(false);
    const auto savedCrc = packet.getCRC();
    const auto savedLength = packet.getPrimaryHeader().getDataLength();
    TEST_VOID(packet.getDataField().setApplicationData(
      std::vector<std::uint8_t>{0x22U}));
    const ccsds::Packet &view = packet;
    const auto snapshot1 = view.serializeSnapshot();
    const auto snapshot2 = view.serializeSnapshot();
    return snapshot1 && snapshot2
        && snapshot1.value() == snapshot2.value()
        && snapshot1.value() != original.value()
        && coreCrcValid(snapshot1.value())
        && packet.getCRC() == savedCrc
        && packet.getPrimaryHeader().getDataLength() == savedLength
        && !packet.serialize();
  });

  tester->unitTest("Serialization allocates for secondary-header callback growth.", [] {
    ccsds::Packet packet;
    auto header = std::make_shared<GrowingSecondaryHeader>();
    TEST_VOID(packet.setSecondaryHeader(header));
    TEST_VOID(packet.setApplicationData({0xAAU}));
    const auto originalSize = packet.getSerializedSize();
    const auto wire = packet.serialize();
    if (!wire || wire.value().size() != originalSize + 1U
        || !coreCrcValid(wire.value())
        || header->getSize() != 2U) return false;
    const auto repeated = packet.serialize();
    return repeated && repeated.value() == wire.value()
        && header->getSize() == 2U;
  });

  tester->unitTest("Raw serialization bounds callback size growth before writing.", [] {
    ccsds::Packet packet;
    auto header = std::make_shared<GrowingSecondaryHeader>();
    TEST_VOID(packet.setSecondaryHeader(header));
    TEST_VOID(packet.setApplicationData({0xAAU}));
    const auto originalSize = packet.getSerializedSize();
    std::vector<std::uint8_t> output(originalSize, 0xA5U);
    const auto tooSmall = packet.serialize(output.data(), output.size());
    if (tooSmall || tooSmall.error().code() != ccsds::INVALID_DATA
        || output != std::vector<std::uint8_t>(originalSize, 0xA5U)
        || header->getSize() != 2U) return false;

    // The legacy callback has deliberately mutated its own header and is not
    // failure-transactional. The caller's output buffer is still untouched,
    // and retrying with adequate space must produce a valid C11 packet.
    const auto recovered = packet.serialize();
    return recovered && recovered.value().size() == originalSize + 1U
        && coreCrcValid(recovered.value());
  });

  tester->unitTest("Const Manager retrieval never invokes callbacks on shared headers.", [] {
    ccsds::Packet prototype;
    auto header = std::make_shared<UpdatingSecondaryHeader>();
    TEST_VOID(prototype.setSecondaryHeader(header));
    ccsds::Manager manager(prototype);
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(8U);
    TEST_VOID(manager.setApplicationData({0xAAU, 0xBBU}));
    const ccsds::Manager &view = manager;
    const auto before = header->serialize();
    const auto stream1 = view.getPacketsBuffer();
    const auto stream2 = view.getPacketsBuffer();
    const auto indexed = manager.getPacketBufferAtIndex(0U);
    return stream1 && stream2 && indexed
        && stream1.value() == stream2.value()
        && stream1.value() == indexed.value()
        && coreCrcValid(stream1.value())
        && header->serialize() == before;
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
