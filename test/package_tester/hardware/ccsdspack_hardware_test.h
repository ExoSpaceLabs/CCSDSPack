// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_HARDWARE_TEST_H
#define CCSDSPACK_HARDWARE_TEST_H

#include "CCSDSPack.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace CCSDSPackHardwareTest {

enum ResultCode : int {
  Pass = 0,
  SetPrimaryHeaderFailed = 1,
  SetManagerTemplateFailed = 2,
  ManagerSequenceConfigurationFailed = 3,
  SetApplicationDataFailed = 4,
  WireVectorMismatch = 5,
  ManagerSequenceAdvanceFailed = 6,
  RawDeclaredSizeFailed = 7,
  RawGenericDecodeFailed = 8,
  BoundedDecodeSizeMismatch = 9,
  DecodedFieldsMismatch = 10,
  RawTruncationAccepted = 11,
  ValidatorRejectedPacket = 12,
  StructuredValidatorReportMissing = 13,
  RawManagerTemplateFailed = 14,
  RawManagerLoadFailed = 15,
  RawManagerDataMismatch = 16,
  PusHeaderFailed = 17,
  PusDirectionInferenceFailed = 18,
  PusDataFailed = 19,
  PusSerializationFailed = 20,
  RawPusDecodeFailed = 21,
  PusValidatorFailed = 22,
  PusValidatorReportMissing = 23,
  CrcFreeHeaderFailed = 24,
  CrcFreeDataFailed = 25,
  CrcFreeVectorMismatch = 26,
  CrcFreeDecodeFailed = 27,
  InvalidVersionHeaderFailed = 28,
  InvalidVersionDataFailed = 29,
  InvalidVersionSerialized = 30,
  InvalidIdleHeaderFailed = 31,
  InvalidIdleSecondaryHeaderFailed = 32,
  InvalidIdleDataFailed = 33,
  InvalidIdleSerialized = 34,
  ValidIdleHeaderFailed = 35,
  ValidIdleDataFailed = 36,
  ValidIdleSerializationFailed = 37
};

inline int run() {
  // Independent generic CRC16 vector also used by the hosted reference suite:
  // 00 00 C0 00 00 03 AA 55 2E BB
  ccsds::Packet templatePacket;
  if (const auto result = templatePacket.setPrimaryHeader(ccsds::PrimaryHeader{
        0, 0, 0, 0x000, ccsds::UNSEGMENTED, 0, 0
      }); !result) return SetPrimaryHeaderFailed;

  ccsds::Manager manager;
  if (const auto result = manager.setPacketTemplate(templatePacket); !result)
    return SetManagerTemplateFailed;
  manager.setAutoValidateEnable(false);
  manager.setDataFieldSize(1000U);

  if (manager.getSequenceCount() != 0U || !manager.getAutoSequenceCountEnable())
    return ManagerSequenceConfigurationFailed;

  const std::uint8_t applicationData[]{0xAA, 0x55};
  if (const auto result = manager.setApplicationData(applicationData, sizeof(applicationData)); !result)
    return SetApplicationDataFailed;

  const std::vector<std::uint8_t> expectedPacket{
    0x00, 0x00, 0xC0, 0x00, 0x00, 0x03, 0xAA, 0x55, 0x2E, 0xBB
  };

  const auto packetsResult = manager.getPacketsBuffer();
  if (!packetsResult || packetsResult.value() != expectedPacket) return WireVectorMismatch;
  const auto &packetsData = packetsResult.value();
  if (manager.getSequenceCount() != 1U || manager.getTotalPackets() != 1U)
    return ManagerSequenceAdvanceFailed;

  // Transport-facing raw-buffer framing: only the six-octet primary header is required.
  const auto declared = ccsds::buffer::declaredPacketSize(packetsData.data(), 6U);
  if (!declared || declared.value() != packetsData.size()) return RawDeclaredSizeFailed;

  // Raw generic pointer-plus-size parsing and exact consumed-byte reporting.
  ccsds::Packet decoded;
  const auto consumed = ccsds::buffer::deserializeBounded(
    decoded, packetsData.data(), packetsData.size());
  if (!consumed) return RawGenericDecodeFailed;
  if (consumed.value() != expectedPacket.size()) return BoundedDecodeSizeMismatch;

  const auto &header = decoded.getPrimaryHeader();
  if (header.getVersionNumber() != 0U
      || header.getType() != 0U
      || header.getSecondaryHeaderFlag() != 0U
      || header.getAPID() != 0U
      || header.getSequenceFlags() != ccsds::UNSEGMENTED
      || header.getSequenceCount() != 0U
      || decoded.getApplicationDataBytes() != std::vector<std::uint8_t>({0xAA, 0x55})
      || decoded.getCRC() != 0x2EBBU
      || decoded.getSerializedSize() != expectedPacket.size()) return DecodedFieldsMismatch;

  // The raw parser must reject an incomplete transport buffer rather than over-read it.
  ccsds::Packet truncated;
  const auto truncatedResult = ccsds::buffer::deserializeBounded(
    truncated, packetsData.data(), packetsData.size() - 1U);
  if (truncatedResult) return RawTruncationAccepted;

  ccsds::Validator validator(templatePacket);
  validator.configure(true, false, true);
  const auto validation = validator.validate(decoded);
  if (!validation.valid()) return ValidatorRejectedPacket;
  if (!validation.passed(ccsds::ValidationCode::PacketDataLength)
      || !validation.passed(ccsds::ValidationCode::Crc16)
      || !validation.passed(ccsds::ValidationCode::PacketIdentifier))
    return StructuredValidatorReportMissing;

  // Raw Manager stream ingestion must reconstruct the original application payload.
  ccsds::Manager rawReceiver;
  if (const auto result = rawReceiver.setPacketTemplate(templatePacket); !result)
    return RawManagerTemplateFailed;
  rawReceiver.setAutoValidateEnable(false);
  rawReceiver.setDataFieldSize(1000U);
  const auto rawLoad = rawReceiver.load(packetsData.data(), packetsData.size());
  if (!rawLoad) return RawManagerLoadFailed;
  const auto reconstructed = rawReceiver.getApplicationDataBuffer();
  if (!reconstructed || reconstructed.value() != std::vector<std::uint8_t>({0xAA, 0x55}))
    return RawManagerDataMismatch;

  // Concrete PUS identity, Packet Type inference, typed raw PUS decoding, and validation.
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

  // Packet-level PEC=None remains independent of secondary-header/PUS policy.
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
