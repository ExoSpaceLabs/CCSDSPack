// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSMissionProfile.h"

namespace {
  bool validIdentifierWidth(const std::uint8_t width) {
    return width == 0U || width == 1U || width == 2U || width == 4U;
  }
}

CCSDS::MissionProfile CCSDS::makePusProfile(const PusRevision revision,
                                            const PacketDirection direction) {
  MissionProfile profile;
  profile.pusEnabled = true;
  profile.pusRevision = revision;
  profile.direction = direction;
  if (revision == PusRevision::C) {
    if (direction == PacketDirection::Telecommand) profile.sourceIdOctets = 2U;
    if (direction == PacketDirection::Telemetry) profile.destinationIdOctets = 2U;
  }
  return profile;
}

CCSDS::ResultBool CCSDS::validateMissionProfile(const MissionProfile &profile) {
  RET_IF_ERR_MSG(profile.packetErrorControl != PacketErrorControlMode::None
                 && profile.packetErrorControl != PacketErrorControlMode::CRC16,
                 ErrorCode::INVALID_DATA, "Mission profile selects an unsupported packet error control mode.");

  if (!profile.pusEnabled) {
    RET_IF_ERR_MSG(profile.pusRevision != PusRevision::Unspecified
                   || profile.direction != PacketDirection::Unspecified
                   || profile.sourceIdOctets != 0U
                   || profile.destinationIdOctets != 0U
                   || profile.telemetryTimestampPresent
                   || profile.telemetryTimeCode != TimeCodeFormat::None
                   || profile.telemetryTimeCodeOctets != 0U
                   || profile.pusATmPacketSubcounterPresent
                   || profile.secondaryHeaderSpareOctets != 0U,
                   ErrorCode::INVALID_DATA,
                   "Generic CCSDS mission profiles cannot contain PUS tailoring.");
    return true;
  }

  RET_IF_ERR_MSG(profile.pusRevision != PusRevision::A
                 && profile.pusRevision != PusRevision::C,
                 ErrorCode::INVALID_DATA, "PUS is enabled without a supported explicit revision.");
  RET_IF_ERR_MSG(profile.direction != PacketDirection::Telecommand
                 && profile.direction != PacketDirection::Telemetry,
                 ErrorCode::INVALID_DATA, "PUS is enabled without an explicit packet direction.");
  RET_IF_ERR_MSG(!validIdentifierWidth(profile.sourceIdOctets)
                 || !validIdentifierWidth(profile.destinationIdOctets),
                 ErrorCode::INVALID_DATA, "PUS identifier widths must be 0, 1, 2, or 4 octets.");

  if (profile.direction == PacketDirection::Telecommand) {
    RET_IF_ERR_MSG(profile.destinationIdOctets != 0U || profile.telemetryTimestampPresent
                   || profile.telemetryTimeCode != TimeCodeFormat::None
                   || profile.telemetryTimeCodeOctets != 0U
                   || profile.pusATmPacketSubcounterPresent,
                   ErrorCode::INVALID_DATA, "A PUS TC profile contains TM-only tailoring.");
    RET_IF_ERR_MSG(profile.pusRevision == PusRevision::C && profile.sourceIdOctets != 2U,
                   ErrorCode::INVALID_DATA, "PUS-C TC requires a two-octet source ID.");
  } else {
    RET_IF_ERR_MSG(profile.sourceIdOctets != 0U, ErrorCode::INVALID_DATA,
                   "A PUS TM profile cannot contain a source-ID width.");
    RET_IF_ERR_MSG(profile.pusRevision == PusRevision::C && profile.destinationIdOctets != 2U,
                   ErrorCode::INVALID_DATA, "PUS-C TM requires a two-octet destination ID.");
    RET_IF_ERR_MSG(profile.pusRevision == PusRevision::C
                   && profile.pusATmPacketSubcounterPresent,
                   ErrorCode::INVALID_DATA, "The PUS-A packet subcounter is not part of PUS-C TM.");

    if (profile.telemetryTimestampPresent) {
      RET_IF_ERR_MSG(profile.telemetryTimeCode != TimeCodeFormat::Cuc
                     && profile.telemetryTimeCode != TimeCodeFormat::Cds,
                     ErrorCode::INVALID_DATA, "A present TM timestamp requires CUC or CDS selection.");
      RET_IF_ERR_MSG(profile.telemetryTimeCodeOctets == 0U,
                     ErrorCode::INVALID_DATA, "A present TM timestamp requires a non-zero encoded size.");
    } else {
      RET_IF_ERR_MSG(profile.telemetryTimeCode != TimeCodeFormat::None
                     || profile.telemetryTimeCodeOctets != 0U,
                     ErrorCode::INVALID_DATA, "An absent TM timestamp requires format None and size zero.");
    }
  }
  return true;
}

bool CCSDS::missionProfilesEqual(const MissionProfile &lhs, const MissionProfile &rhs) {
  return lhs.pusEnabled == rhs.pusEnabled
         && lhs.pusRevision == rhs.pusRevision
         && lhs.direction == rhs.direction
         && lhs.sourceIdOctets == rhs.sourceIdOctets
         && lhs.destinationIdOctets == rhs.destinationIdOctets
         && lhs.packetErrorControl == rhs.packetErrorControl
         && lhs.telemetryTimestampPresent == rhs.telemetryTimestampPresent
         && lhs.telemetryTimeCode == rhs.telemetryTimeCode
         && lhs.telemetryTimeCodeOctets == rhs.telemetryTimeCodeOctets
         && lhs.pusATmPacketSubcounterPresent == rhs.pusATmPacketSubcounterPresent
         && lhs.secondaryHeaderSpareOctets == rhs.secondaryHeaderSpareOctets;
}

std::string CCSDS::pusSelector(const PusRevision revision, const PacketDirection direction) {
  const char *rev = revision == PusRevision::A ? "revA"
                    : revision == PusRevision::C ? "revC" : "unspecified";
  const char *dir = direction == PacketDirection::Telecommand ? "TC"
                    : direction == PacketDirection::Telemetry ? "TM" : "unspecified";
  return std::string{"PUS:"} + rev + ":" + dir;
}
