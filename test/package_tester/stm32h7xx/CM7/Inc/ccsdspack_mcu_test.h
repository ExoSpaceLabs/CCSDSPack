// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_MCU_TEST_H
#define CCSDSPACK_MCU_TEST_H

// The prebuilt MCU archive is compiled with CCSDS_MCU. Consumers of the static
// archive must see the same public-header interface in every translation unit.
#ifndef CCSDS_MCU
#error "Define CCSDS_MCU when compiling the STM32 validation application"
#endif

#include "CCSDSPack.h"

#include <cstdint>
#include <string>
#include <vector>

namespace CCSDSPackMcuTest {
  enum ResultCode : int {
    Pass = 0,
    SetPrimaryHeaderFailed = 1,
    RegisterSecondaryHeaderFailed = 2,
    SetSecondaryHeaderFailed = 3,
    SetManagerTemplateFailed = 4,
    ManagerSequenceConfigurationFailed = 5,
    SetApplicationDataFailed = 6,
    WireVectorMismatch = 7,
    ManagerSequenceAdvanceFailed = 8,
    BoundedDecodeFailed = 9,
    BoundedDecodeSizeMismatch = 10,
    DecodedFieldsMismatch = 11,
    ValidatorRejectedPacket = 12,
    CrcFreeHeaderFailed = 13,
    CrcFreeDataFailed = 14,
    CrcFreeVectorMismatch = 15,
    CrcFreeDecodeFailed = 16,
    InvalidVersionHeaderFailed = 17,
    InvalidVersionDataFailed = 18,
    InvalidVersionSerialized = 19,
    InvalidIdleHeaderFailed = 20,
    InvalidIdleSecondaryHeaderFailed = 21,
    InvalidIdleDataFailed = 22,
    InvalidIdleSerialized = 23,
    ValidIdleHeaderFailed = 24,
    ValidIdleDataFailed = 25,
    ValidIdleSerializationFailed = 26
  };

  class CustomSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
  public:
    CustomSecondaryHeader() { setVariableLength(true); }

    explicit CustomSecondaryHeader(const std::vector<std::uint8_t> &data) : m_data(data) {
      setVariableLength(true);
    }

    [[nodiscard]] CCSDS::ResultBool deserialize(
      const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }

    void update(CCSDS::DataField *) override {}

    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }

    [[nodiscard]] std::vector<std::uint8_t> serialize() const override {
      return m_data;
    }

    [[nodiscard]] std::string getType() const override {
      return "CustomSecondaryHeader";
    }

  private:
    std::vector<std::uint8_t> m_data{};
  };

  /**
   * @brief Runs the deterministic bare-metal consumer validation suite.
   * @return Pass, or the first non-zero ResultCode identifying the failed stage.
   *
   * The suite deliberately exercises std::vector, std::string, shared ownership,
   * factory registration, packet generation, CRC16, bounded parsing, Validator,
   * CRC-free packets, PVN enforcement, and Idle Packet rules. It therefore checks
   * the C++ runtime and heap configuration as well as the packet logic when run on
   * the target MCU.
   */
  inline int run() {
    CCSDS::Packet templatePacket;
    if (const auto result = templatePacket.setPrimaryHeader(CCSDS::PrimaryHeader{
          0, 1, 0, 0x123, CCSDS::UNSEGMENTED, 5, 0
        }); !result) {
      return SetPrimaryHeaderFailed;
    }

    if (const auto result = templatePacket.RegisterSecondaryHeader<CustomSecondaryHeader>();
        !result) {
      return RegisterSecondaryHeaderFailed;
    }

    const std::vector<std::uint8_t> secondaryHeader{
      0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05
    };
    if (const auto result = templatePacket.setDataFieldHeader(
          secondaryHeader, "CustomSecondaryHeader"); !result) {
      return SetSecondaryHeaderFailed;
    }

    CCSDS::Manager manager;
    if (const auto result = manager.setPacketTemplate(templatePacket); !result) {
      return SetManagerTemplateFailed;
    }
    manager.setAutoValidateEnable(false);
    manager.setDataFieldSize(1000U);

    if (manager.getSequenceCount() != 5U || !manager.getAutoSequenceCountEnable()) {
      return ManagerSequenceConfigurationFailed;
    }

    const std::vector<std::uint8_t> applicationData{0x01, 0x02, 0x03};
    if (const auto result = manager.setApplicationData(applicationData); !result) {
      return SetApplicationDataFailed;
    }

    const std::vector<std::uint8_t> expectedPacket{
      0x19, 0x23, 0xC0, 0x05, 0x00, 0x0B,
      0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05,
      0x01, 0x02, 0x03,
      0xB7, 0x45
    };

    const auto packetsData = manager.getPacketsBuffer();
    if (packetsData != expectedPacket) {
      return WireVectorMismatch;
    }
    if (manager.getSequenceCount() != 6U || manager.getTotalPackets() != 1U) {
      return ManagerSequenceAdvanceFailed;
    }

    CCSDS::Packet decoded;
    const auto consumed = decoded.deserializeBounded(
      packetsData, static_cast<std::uint16_t>(secondaryHeader.size()));
    if (!consumed) {
      return BoundedDecodeFailed;
    }
    if (consumed.value() != expectedPacket.size()) {
      return BoundedDecodeSizeMismatch;
    }

    const auto &header = decoded.getPrimaryHeader();
    if (header.getVersionNumber() != 0U
        || header.getType() != 1U
        || header.getDataFieldHeaderFlag() != 1U
        || header.getAPID() != 0x123U
        || header.getSequenceFlags() != CCSDS::UNSEGMENTED
        || header.getSequenceCount() != 5U
        || decoded.getDataFieldHeaderBytes() != secondaryHeader
        || decoded.getApplicationDataBytes() != applicationData
        || decoded.getCRC() != 0xB745U
        || decoded.getSerializedSize() != expectedPacket.size()) {
      return DecodedFieldsMismatch;
    }

    CCSDS::Validator validator(templatePacket);
    validator.configure(true, false, true);
    if (!validator.validate(decoded)) {
      return ValidatorRejectedPacket;
    }

    CCSDS::Packet crcDisabled;
    crcDisabled.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    if (const auto result = crcDisabled.setPrimaryHeader(CCSDS::PrimaryHeader{
          0, 0, 0, 0x123, CCSDS::UNSEGMENTED, 7, 0
        }); !result) {
      return CrcFreeHeaderFailed;
    }
    if (const auto result = crcDisabled.setApplicationData({0xAA, 0x55}); !result) {
      return CrcFreeDataFailed;
    }

    const std::vector<std::uint8_t> expectedCrcDisabled{
      0x01, 0x23, 0xC0, 0x07, 0x00, 0x01, 0xAA, 0x55
    };
    const auto crcDisabledBytes = crcDisabled.serialize();
    if (crcDisabledBytes != expectedCrcDisabled) {
      return CrcFreeVectorMismatch;
    }

    CCSDS::Packet decodedCrcDisabled;
    decodedCrcDisabled.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    const auto crcDisabledConsumed = decodedCrcDisabled.deserializeBounded(crcDisabledBytes);
    if (!crcDisabledConsumed
        || crcDisabledConsumed.value() != crcDisabledBytes.size()
        || decodedCrcDisabled.getPrimaryHeader().getSequenceCount() != 7U
        || decodedCrcDisabled.getApplicationDataBytes()
           != std::vector<std::uint8_t>({0xAA, 0x55})
        || decodedCrcDisabled.getCRC() != 0U) {
      return CrcFreeDecodeFailed;
    }

    CCSDS::Packet invalidVersion;
    if (const auto result = invalidVersion.setPrimaryHeader(CCSDS::PrimaryHeader{
          1, 0, 0, 1, CCSDS::UNSEGMENTED, 0, 0
        }); !result) {
      return InvalidVersionHeaderFailed;
    }
    if (const auto result = invalidVersion.setApplicationData({0x01}); !result) {
      return InvalidVersionDataFailed;
    }
    if (!invalidVersion.serialize().empty()) {
      return InvalidVersionSerialized;
    }

    CCSDS::Packet invalidIdle;
    if (const auto result = invalidIdle.setPrimaryHeader(CCSDS::PrimaryHeader{
          0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
        }); !result) {
      return InvalidIdleHeaderFailed;
    }
    if (const auto result = invalidIdle.setDataFieldHeader({0x01}); !result) {
      return InvalidIdleSecondaryHeaderFailed;
    }
    if (const auto result = invalidIdle.setApplicationData({0x00}); !result) {
      return InvalidIdleDataFailed;
    }
    if (!invalidIdle.serialize().empty()) {
      return InvalidIdleSerialized;
    }

    CCSDS::Packet validIdle;
    if (const auto result = validIdle.setPrimaryHeader(CCSDS::PrimaryHeader{
          0, 0, 0, CCSDS::IDLE_APID, CCSDS::UNSEGMENTED, 0, 0
        }); !result) {
      return ValidIdleHeaderFailed;
    }
    if (const auto result = validIdle.setApplicationData({0x00}); !result) {
      return ValidIdleDataFailed;
    }
    const auto validIdleBytes = validIdle.serialize();
    if (validIdleBytes.empty() || validIdle.getSerializedSize() != validIdleBytes.size()) {
      return ValidIdleSerializationFailed;
    }

    return Pass;
  }
} // namespace CCSDSPackMcuTest

#endif // CCSDSPACK_MCU_TEST_H
