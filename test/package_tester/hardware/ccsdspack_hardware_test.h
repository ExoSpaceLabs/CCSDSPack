// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_HARDWARE_TEST_H
#define CCSDSPACK_HARDWARE_TEST_H

#include "CCSDSPack.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace CCSDSPackHardwareTest {

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
  ValidIdleSerializationFailed = 26,
  StructuredValidatorReportMissing = 27,
  PusHeaderFailed = 28,
  PusDirectionInferenceFailed = 29,
  PusDataFailed = 30,
  PusSerializationFailed = 31,
  PusValidatorFailed = 32,
  PusValidatorReportMissing = 33,
  RawDeclaredSizeFailed = 34,
  RawGenericDecodeFailed = 35,
  RawTruncationAccepted = 36,
  RawPusDecodeFailed = 37,
  RawManagerTemplateFailed = 38,
  RawManagerLoadFailed = 39,
  RawManagerDataMismatch = 40
};

class CustomSecondaryHeader final : public ccsds::SecondaryHeaderAbstract {
public:
  CustomSecondaryHeader() { setVariableLength(true); }
  explicit CustomSecondaryHeader(const std::vector<std::uint8_t> &data) : m_data(data) {
    setVariableLength(true);
  }
  [[nodiscard]] ccsds::ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
    m_data = data;
    return true;
  }
  void update(ccsds::DataField *) override {}
  [[nodiscard]] std::uint16_t getSize() const override {
    return static_cast<std::uint16_t>(m_data.size());
  }
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }
  [[nodiscard]] std::string getType() const override { return "CustomSecondaryHeader"; }
private:
  std::vector<std::uint8_t> m_data{};
};

inline int run() {
  ccsds::Packet templatePacket;
  if (const auto result = templatePacket.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 1, 0, 0x123, ccsds::UNSEGMENTED, 5, 0
      }); !result) return SetPrimaryHeaderFailed;

  if (const auto result = templatePacket.RegisterSecondaryHeader<CustomSecondaryHeader>();
      !result) return RegisterSecondaryHeaderFailed;

  const std::vector<std::uint8_t> secondaryHeader{
    0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05
  };
  if (const auto result = templatePacket.setSecondaryHeader(
        secondaryHeader, "CustomSecondaryHeader"); !result) return SetSecondaryHeaderFailed;

  ccsds::Manager manager;
  if (const auto result = manager.setPacketTemplate(templatePacket); !result)
    return SetManagerTemplateFailed;
  manager.setAutoValidateEnable(false);
  manager.setDataFieldSize(1000U);

  if (manager.getSequenceCount() != 5U || !manager.getAutoSequenceCountEnable())
    return ManagerSequenceConfigurationFailed;

  const std::uint8_t applicationData[]{0x01, 0x02, 0x03};
  if (const auto result = manager.setApplicationData(applicationData, sizeof(applicationData)); !result)
    return SetApplicationDataFailed;

  const std::vector<std::uint8_t> expectedPacket{
    0x19, 0x23, 0xC0, 0x05, 0x00, 0x0B,
    0x77, 0xFA, 0x0B, 0x00, 0x00, 0x0B, 0x05,
    0x01, 0x02, 0x03,
    0xB7, 0x45
  };

  const auto packetsResult = manager.getPacketsBuffer();
  if (!packetsResult) return WireVectorMismatch;
  const auto &packetsData = packetsResult.value();
  if (packetsData != expectedPacket) return WireVectorMismatch;
  if (manager.getSequenceCount() != 6U || manager.getTotalPackets() != 1U)
    return ManagerSequenceAdvanceFailed;

  const auto declared = ccsds::buffer::declaredPacketSize(packetsData.data(), 6U);
  if (!declared || declared.value() != packetsData.size()) return RawDeclaredSizeFailed;

  ccsds::Packet decoded;
  const auto consumed = ccsds::buffer::deserializeBounded(
    decoded, packetsData.data(), packetsData.size(), static_cast<std::uint16_t>(secondaryHeader.size()));
  if (!consumed) return RawGenericDecodeFailed;
  if (consumed.value() != expectedPacket.size()) return BoundedDecodeSizeMismatch;

  const auto &header = decoded.getPrimaryHeader();
  if (header.getVersionNumber() != 0U
      || header.getType() != 1U
      || header.getSecondaryHeaderFlag() != 1U
      || header.getAPID() != 0x123U
      || header.getSequenceFlags() != ccsds::UNSEGMENTED
      || header.getSequenceCount() != 5U
      || decoded.getSecondaryHeaderBytes() != secondaryHeader
      || decoded.getApplicationDataBytes() != std::vector<std::uint8_t>({0x01, 0x02, 0x03})
      || decoded.getCRC() != 0xB745U
      || decoded.getSerializedSize() != expectedPacket.size()) return DecodedFieldsMismatch;

  ccsds::Packet truncated;
  const auto truncatedResult = ccsds::buffer::deserializeBounded(
    truncated, packetsData.data(), packetsData.size() - 1U, static_cast<std::uint16_t>(secondaryHeader.size()));
  if (truncatedResult) return RawTruncationAccepted;

  ccsds::Validator validator(templatePacket);
  validator.configure(true, false, true);
  const auto validation = validator.validate(decoded);
  if (!validation.valid()) return ValidatorRejectedPacket;
  if (!validation.contains(ccsds::ValidationCode::PacketDataLength)
      || !validation.contains(ccsds::ValidationCode::Crc16)
      || !validation.contains(ccsds::ValidationCode::PacketIdentifier)
      || !validation.passed(ccsds::ValidationCode::TemplateSecondaryHeader))
    return StructuredValidatorReportMissing;

  ccsds::Manager rawReceiver;
  if (const auto result = rawReceiver.setPacketTemplate(templatePacket); !result)
    return RawManagerTemplateFailed;
  rawReceiver.setAutoValidateEnable(false);
  rawReceiver.setDataFieldSize(1000U);
  const auto rawLoad = rawReceiver.load(packetsData.data(), packetsData.size());
  if (!rawLoad) return RawManagerLoadFailed;
  const auto reconstructed = rawReceiver.getApplicationDataBuffer();
  if (!reconstructed
      || reconstructed.value() != std::vector<std::uint8_t>({0x01, 0x02, 0x03}))
    return RawManagerDataMismatch;

  ccsds::Packet pusPacket;
  if (const auto result = pusPacket.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, 0x42, ccsds::UNSEGMENTED, 0, 0
      }); !result) return PusHeaderFailed;
  if (const auto result = pusPacket.setSecondaryHeader(
        std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x09U)); !result)
    return PusHeaderFailed;
  if (pusPacket.getDirection() != ccsds::PacketDirection::Telecommand
      || pusPacket.getPrimaryHeader().getType() != 1U)
    return PusDirectionInferenceFailed;
  if (const auto result = pusPacket.setApplicationData({0xAAU, 0x55U}); !result)
    return PusDataFailed;
  const auto pusWire = pusPacket.serialize();
  if (!pusWire) return PusSerializationFailed;

  ccsds::Packet pusDecoded;
  const auto pusConsumed = ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
    pusDecoded, pusWire.value().data(), pusWire.value().size());
  if (!pusConsumed || pusConsumed.value() != pusWire.value().size()
      || pusDecoded.getDirection() != ccsds::PacketDirection::Telecommand)
    return RawPusDecodeFailed;

  ccsds::Validator pusValidator;
  const auto pusValidation = pusValidator.validate(pusDecoded);
  if (!pusValidation.valid()) return PusValidatorFailed;
  if (!pusValidation.passed(ccsds::ValidationCode::PusRevision)
      || !pusValidation.passed(ccsds::ValidationCode::PusDirection)
      || !pusValidation.passed(ccsds::ValidationCode::PusPacketType)
      || !pusValidation.passed(ccsds::ValidationCode::PusTailoring)
      || !pusValidation.passed(ccsds::ValidationCode::PusAcknowledgement)
      || !pusValidation.passed(ccsds::ValidationCode::PusSourceId))
    return PusValidatorReportMissing;

  ccsds::Packet crcDisabled;
  crcDisabled.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  if (const auto result = crcDisabled.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, 0x123, ccsds::UNSEGMENTED, 7, 0
      }); !result) return CrcFreeHeaderFailed;
  if (const auto result = crcDisabled.setApplicationData({0xAA, 0x55}); !result)
    return CrcFreeDataFailed;

  const std::vector<std::uint8_t> expectedCrcDisabled{
    0x01, 0x23, 0xC0, 0x07, 0x00, 0x01, 0xAA, 0x55
  };
  const auto crcDisabledResult = crcDisabled.serialize();
  if (!crcDisabledResult || crcDisabledResult.value() != expectedCrcDisabled)
    return CrcFreeVectorMismatch;

  ccsds::Packet decodedCrcDisabled;
  decodedCrcDisabled.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  const auto crcDisabledConsumed = ccsds::buffer::deserializeBounded(
    decodedCrcDisabled, expectedCrcDisabled.data(), expectedCrcDisabled.size());
  if (!crcDisabledConsumed
      || crcDisabledConsumed.value() != expectedCrcDisabled.size()
      || decodedCrcDisabled.getPrimaryHeader().getSequenceCount() != 7U
      || decodedCrcDisabled.getApplicationDataBytes() != std::vector<std::uint8_t>({0xAA, 0x55})
      || decodedCrcDisabled.getCRC() != 0U) return CrcFreeDecodeFailed;

  ccsds::Packet invalidVersion;
  if (const auto result = invalidVersion.setPrimaryHeader(ccsds::PrimaryHeader{
        1, 0, 0, 1, ccsds::UNSEGMENTED, 0, 0
      }); !result) return InvalidVersionHeaderFailed;
  if (const auto result = invalidVersion.setApplicationData({0x01}); !result)
    return InvalidVersionDataFailed;
  if (invalidVersion.serialize()) return InvalidVersionSerialized;

  ccsds::Packet invalidIdle;
  if (const auto result = invalidIdle.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
      }); !result) return InvalidIdleHeaderFailed;
  if (const auto result = invalidIdle.setSecondaryHeader({0x01}); !result)
    return InvalidIdleSecondaryHeaderFailed;
  if (const auto result = invalidIdle.setApplicationData({0x00}); !result)
    return InvalidIdleDataFailed;
  if (invalidIdle.serialize()) return InvalidIdleSerialized;

  ccsds::Packet validIdle;
  if (const auto result = validIdle.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, ccsds::IDLE_APID, ccsds::UNSEGMENTED, 0, 0
      }); !result) return ValidIdleHeaderFailed;
  if (const auto result = validIdle.setApplicationData({0x00}); !result)
    return ValidIdleDataFailed;
  const auto validIdleResult = validIdle.serialize();
  if (!validIdleResult || validIdle.getSerializedSize() != validIdleResult.value().size())
    return ValidIdleSerializationFailed;

  return Pass;
}

} // namespace CCSDSPackHardwareTest

#endif // CCSDSPACK_HARDWARE_TEST_H
