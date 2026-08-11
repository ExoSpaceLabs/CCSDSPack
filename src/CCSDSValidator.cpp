// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSValidator.h"
#include "CCSDSUtils.h"
#include "PusSecondaryHeaders.h"

namespace {
  bool identifierFits(const std::uint32_t value, const std::uint8_t octets) noexcept {
    if (octets == 0U) return value == 0U;
    if (octets >= 4U) return true;
    return value < (1UL << (octets * 8U));
  }

  bool validRevision(const ccsds::pus::Revision revision) noexcept {
    return revision == ccsds::pus::Revision::A
           || revision == ccsds::pus::Revision::C;
  }

  bool validDirection(const ccsds::pus::Direction direction) noexcept {
    return direction == ccsds::pus::Direction::Telecommand
           || direction == ccsds::pus::Direction::Telemetry;
  }

  bool reservedBitsValid(const std::vector<std::uint8_t> &bytes,
                         const ccsds::pus::Revision revision,
                         const ccsds::pus::Direction direction) noexcept {
    if (bytes.empty()) return false;
    if (revision == ccsds::pus::Revision::A
        && direction == ccsds::pus::Direction::Telecommand) {
      return (bytes[0] & 0x80U) == 0U && ((bytes[0] >> 4U) & 0x07U) == 1U;
    }
    if (revision == ccsds::pus::Revision::A
        && direction == ccsds::pus::Direction::Telemetry) {
      return bytes[0] == 0x10U;
    }
    if (revision == ccsds::pus::Revision::C) {
      return (bytes[0] >> 4U) == 2U;
    }
    return false;
  }

  bool spareFieldsValid(const std::vector<std::uint8_t> &bytes,
                        const std::uint8_t spareOctets) noexcept {
    const auto spare = static_cast<std::size_t>(spareOctets);
    if (spare > bytes.size()) return false;
    for (std::size_t i = bytes.size() - spare; i < bytes.size(); ++i) {
      if (bytes[i] != 0U) return false;
    }
    return true;
  }
}

const char *ccsds::validationCodeName(const ValidationCode code) noexcept {
  switch (code) {
    case ValidationCode::PrimaryHeader: return "CCSDS primary header";
    case ValidationCode::PacketVersion: return "CCSDS packet version";
    case ValidationCode::PacketDataLength: return "Packet Data Length";
    case ValidationCode::PacketErrorControlProfile: return "Packet error-control profile";
    case ValidationCode::Crc16: return "CRC16";
    case ValidationCode::SecondaryHeaderPresence: return "Secondary-header presence";
    case ValidationCode::SequenceFlags: return "Sequence flags";
    case ValidationCode::SequenceCount: return "Sequence count";
    case ValidationCode::PacketIdentifier: return "Packet Identification";
    case ValidationCode::SegmentationClass: return "Segmentation class";
    case ValidationCode::MissionProfile: return "Mission profile";
    case ValidationCode::TemplateMissionProfile: return "Template mission profile";
    case ValidationCode::PusHeader: return "PUS secondary header";
    case ValidationCode::PusRevision: return "PUS revision";
    case ValidationCode::PusDirection: return "PUS direction";
    case ValidationCode::PusPacketType: return "PUS direction / Packet Type";
    case ValidationCode::PusProfile: return "PUS header profile";
    case ValidationCode::PusSecondaryHeaderSize: return "PUS secondary-header size";
    case ValidationCode::PusReservedBits: return "PUS reserved/version bits";
    case ValidationCode::PusSpareFields: return "PUS spare fields";
    case ValidationCode::PusAcknowledgement: return "PUS acknowledgement flags";
    case ValidationCode::PusSourceId: return "PUS source ID";
    case ValidationCode::PusDestinationId: return "PUS destination ID";
    case ValidationCode::PusPacketSubcounter: return "PUS-A packet subcounter";
    case ValidationCode::PusTimeReferenceStatus: return "PUS-C time-reference status";
    case ValidationCode::PusTimestamp: return "PUS CUC timestamp";
  }
  return "Unknown validation check";
}

void ccsds::Validator::configure(const bool validatePacketCoherence,
                                 const bool validateSequenceCount,
                                 const bool validateAgainstTemplate) {
  m_validatePacketCoherence = validatePacketCoherence;
  m_validateSequenceCount = validateSequenceCount;
  m_validateAgainstTemplate = validateAgainstTemplate;
}

void ccsds::Validator::acceptSequence(const Header &header) noexcept {
  const auto next = static_cast<std::uint16_t>(
    (header.getSequenceCount() + 1U) & SEQUENCE_COUNT_MASK);
  auto state = static_cast<std::uint16_t>(SEQUENCE_INITIALIZED_MASK | next);
  if (header.getSequenceFlags() == FIRST_SEGMENT
      || header.getSequenceFlags() == CONTINUING_SEGMENT) {
    state |= SEGMENT_OPEN_MASK;
  }
  m_sequenceCounter = state;
}

ccsds::ValidationReport ccsds::Validator::validate(const Packet &packet) {
  m_report = {};

  const auto &header = packet.getPrimaryHeader();
  const auto headerData = header.serialize();
  const bool primaryHeaderValid = header.getHeaderStatus() != INVALID
                                  && headerData.size() == 6U;
  setCheck(ValidationCode::PrimaryHeader, primaryHeaderValid);
  if (!primaryHeaderValid) return m_report;

  bool sequenceFlagsValid{true};
  bool sequenceCountValid{true};

  if (m_validatePacketCoherence) {
    setCheck(ValidationCode::PacketVersion, header.getVersionNumber() == 0U);

    const auto serializedSize = packet.getSerializedSize();
    const auto packetDataFieldSize = serializedSize >= 6U ? serializedSize - 6U : 0U;
    setCheck(ValidationCode::PacketDataLength,
             packetDataFieldSize > 0U
             && header.getDataLength() == packetDataFieldSize - 1U);

    if (packet.getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
      auto crcInput = headerData;
      const auto dataFieldBytes = packet.getFullDataFieldBytes();
      crcInput.insert(crcInput.end(), dataFieldBytes.begin(), dataFieldBytes.end());
      const CRC16Config crcConfig{};
      const auto calculatedCRC = ccsds::crc16(
        crcInput, crcConfig.polynomial, crcConfig.initialValue,
        crcConfig.finalXorValue);
      setCheck(ValidationCode::Crc16, calculatedCRC == packet.getCRC());
    }

    const auto secondary = packet.getSecondaryHeader();
    const bool secondaryPresence =
      (header.getSecondaryHeaderFlag() != 0U) == static_cast<bool>(secondary);
    setCheck(ValidationCode::SecondaryHeaderPresence, secondaryPresence);

    const auto &profile = packet.getMissionProfile();
    const auto profileResult = validateMissionProfile(profile);
    setCheck(ValidationCode::MissionProfile, static_cast<bool>(profileResult));

    if (!profile.pusEnabled) {
      if (secondary && secondary->isPusHeader()) {
        setCheck(ValidationCode::PusHeader, false);
      }
    } else {
      const bool revisionValid = validRevision(profile.pusRevision);
      const bool directionValid = validDirection(profile.direction);
      setCheck(ValidationCode::PusRevision, revisionValid);
      setCheck(ValidationCode::PusDirection, directionValid);
      setCheck(ValidationCode::PacketErrorControlProfile,
               packet.getPacketErrorControlMode() == profile.packetErrorControl);

      if (directionValid) {
        const auto expectedType = profile.direction == pus::Direction::Telecommand ? 1U : 0U;
        setCheck(ValidationCode::PusPacketType, header.getType() == expectedType);
      } else {
        setCheck(ValidationCode::PusPacketType, false);
      }

      const bool pusHeaderPresent = secondary && secondary->isPusHeader();
      setCheck(ValidationCode::PusHeader, pusHeaderPresent);

      if (pusHeaderPresent) {
        const auto &pusHeader = static_cast<const pus::SecondaryHeader &>(*secondary);
        setCheck(ValidationCode::PusRevision,
                 revisionValid && pusHeader.getRevision() == profile.pusRevision);
        setCheck(ValidationCode::PusDirection,
                 directionValid && pusHeader.getDirection() == profile.direction);
        setCheck(ValidationCode::PusProfile,
                 pusHeader.matchesMissionProfile(profile));

        const auto bytes = secondary->serialize();
        const bool sizeValid = !bytes.empty()
                               && bytes.size() == secondary->getSize();
        setCheck(ValidationCode::PusSecondaryHeaderSize, sizeValid);
        setCheck(ValidationCode::PusReservedBits,
                 sizeValid && reservedBitsValid(bytes, profile.pusRevision,
                                                profile.direction));
        setCheck(ValidationCode::PusSpareFields,
                 sizeValid && spareFieldsValid(bytes,
                                               profile.secondaryHeaderSpareOctets));

        if (pusHeader.getDirection() == pus::Direction::Telecommand) {
          const auto &tc = static_cast<const pus::TcSecondaryHeader &>(pusHeader);
          setCheck(ValidationCode::PusAcknowledgement,
                   tc.getAcknowledgementFlags() <= 0x0FU);
          setCheck(ValidationCode::PusSourceId,
                   identifierFits(tc.getSourceId(), profile.sourceIdOctets));
        } else if (pusHeader.getDirection() == pus::Direction::Telemetry) {
          const auto &tm = static_cast<const pus::TmSecondaryHeader &>(pusHeader);
          setCheck(ValidationCode::PusDestinationId,
                   identifierFits(tm.getDestinationId(), profile.destinationIdOctets));

          if (profile.telemetryTimestampPresent) {
            const auto timestamp = time::serialize(tm.getTimestamp(), profile.telemetryCuc);
            setCheck(ValidationCode::PusTimestamp, static_cast<bool>(timestamp));
          } else {
            setCheck(ValidationCode::PusTimestamp,
                     tm.getTimestamp() == time::CucTime{});
          }

          if (pusHeader.getRevision() == pus::Revision::A) {
            const auto &aTm = static_cast<const pus::rev_a::TmHeader &>(pusHeader);
            setCheck(ValidationCode::PusPacketSubcounter,
                     profile.pusATmPacketSubcounterPresent
                     || aTm.getPacketSubcounter() == 0U);
          } else if (pusHeader.getRevision() == pus::Revision::C) {
            const auto &cTm = static_cast<const pus::rev_c::TmHeader &>(pusHeader);
            setCheck(ValidationCode::PusTimeReferenceStatus,
                     cTm.getTimeReferenceStatus() <= 0x0FU);
          }
        }
      }
    }

    const auto flags = static_cast<ESequenceFlag>(header.getSequenceFlags());
    const bool open = segmentOpen();
    switch (flags) {
      case UNSEGMENTED:
      case FIRST_SEGMENT:
        sequenceFlagsValid = !open;
        break;
      case CONTINUING_SEGMENT:
      case LAST_SEGMENT:
        sequenceFlagsValid = open;
        break;
      default:
        sequenceFlagsValid = false;
        break;
    }
    setCheck(ValidationCode::SequenceFlags, sequenceFlagsValid);

    if (m_validateSequenceCount) {
      sequenceCountValid = !sequenceInitialized()
                           || header.getSequenceCount() == expectedSequenceCount();
      setCheck(ValidationCode::SequenceCount, sequenceCountValid);
    }
  }

  if (m_validateAgainstTemplate) {
    const auto &templateHeader = m_templatePacket.getPrimaryHeader();
    const auto templateHeaderData = templateHeader.serialize();
    const bool templateHeaderValid = templateHeader.getHeaderStatus() != INVALID
                                     && templateHeaderData.size() == 6U;
    setCheck(ValidationCode::PacketIdentifier,
             templateHeaderValid
             && templateHeaderData[0] == headerData[0]
             && templateHeaderData[1] == headerData[1]);

    bool segmentationClassValid{false};
    if (templateHeaderValid) {
      segmentationClassValid = templateHeader.getSequenceFlags() == UNSEGMENTED
        ? header.getSequenceFlags() == UNSEGMENTED
        : header.getSequenceFlags() != UNSEGMENTED;
    }
    setCheck(ValidationCode::SegmentationClass, segmentationClassValid);
    setCheck(ValidationCode::TemplateMissionProfile,
             missionProfilesEqual(m_templatePacket.getMissionProfile(),
                                  packet.getMissionProfile()));
  }

  if (m_validatePacketCoherence && sequenceFlagsValid
      && (!m_validateSequenceCount || sequenceCountValid)
      && m_report.valid()) {
    acceptSequence(header);
  }

  return m_report;
}

void ccsds::Validator::clear() {
  m_sequenceCounter = 0U;
  m_report = {};
  m_templatePacket = {};
  m_templatePacket.setUpdatePacketEnable(false);
}
