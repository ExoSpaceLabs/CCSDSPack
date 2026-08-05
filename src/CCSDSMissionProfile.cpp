// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSMissionProfile.h"
#ifndef CCSDS_MCU
#include "CCSDSConfig.h"
#include <initializer_list>
#endif

namespace {
  bool validIdentifierWidth(const std::uint8_t width) {
    return width == 0U || width == 1U || width == 2U || width == 4U;
  }

#ifndef CCSDS_MCU
  ccsds::Result<int> requiredInt(const ccsds::Config &config,
                                 const char *key,
                                 const int minimum,
                                 const int maximum) {
    RET_IF_ERR_MSG(!config.isKey(key), ccsds::ErrorCode::CONFIG_FILE_ERROR,
                   std::string{"Config: Missing int field: "} + key);
    const auto result = config.get<int>(key);
    if (!result) {
      return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR,
                          std::string{"Config: Invalid int field: "} + key};
    }
    RET_IF_ERR_MSG(result.value() < minimum || result.value() > maximum,
                   ccsds::ErrorCode::CONFIG_FILE_ERROR,
                   std::string{"Config: Out-of-range int field: "} + key);
    return result.value();
  }

  ccsds::Result<std::string> requiredString(const ccsds::Config &config,
                                            const char *key) {
    RET_IF_ERR_MSG(!config.isKey(key), ccsds::ErrorCode::CONFIG_FILE_ERROR,
                   std::string{"Config: Missing string field: "} + key);
    const auto result = config.get<std::string>(key);
    if (!result) {
      return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR,
                          std::string{"Config: Invalid string field: "} + key};
    }
    return result.value();
  }

  bool hasAnyKey(const ccsds::Config &config,
                 const std::initializer_list<const char *> keys) {
    for (const auto key : keys) {
      if (config.isKey(key)) return true;
    }
    return false;
  }

  ccsds::Result<ccsds::PacketErrorControlMode> readPacketErrorControl(
      const ccsds::Config &config) {
    std::string mode;
    ASSIGN_CP(mode, requiredString(config, "ccsds_packet_error_control"));
    if (mode == "none") return ccsds::PacketErrorControlMode::None;
    if (mode == "crc16") return ccsds::PacketErrorControlMode::CRC16;
    return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR,
                        "Config: ccsds_packet_error_control must be 'none' or 'crc16'"};
  }
#endif
}

ccsds::MissionProfile ccsds::pus::makeProfile(const Revision revision,
                                              const Direction direction) {
  MissionProfile profile;
  profile.pusEnabled = true;
  profile.pusRevision = revision;
  profile.direction = direction;
  if (revision == Revision::C) {
    if (direction == Direction::Telecommand) profile.sourceIdOctets = 2U;
    if (direction == Direction::Telemetry) profile.destinationIdOctets = 2U;
  }
  return profile;
}

ccsds::ResultBool ccsds::validateMissionProfile(const MissionProfile &profile) {
  RET_IF_ERR_MSG(profile.packetErrorControl != PacketErrorControlMode::None
                 && profile.packetErrorControl != PacketErrorControlMode::CRC16,
                 ErrorCode::INVALID_DATA, "Mission profile selects an unsupported packet error control mode.");

  if (!profile.pusEnabled) {
    RET_IF_ERR_MSG(profile.pusRevision != pus::Revision::Unspecified
                   || profile.direction != pus::Direction::Unspecified
                   || profile.sourceIdOctets != 0U
                   || profile.destinationIdOctets != 0U
                   || profile.telemetryTimestampPresent
                   || profile.telemetryTimeCode != time::Format::None
                   || profile.telemetryCuc.epoch != time::Epoch::Unspecified
                   || profile.telemetryCuc.pField != time::PFieldMode::Implicit
                   || profile.telemetryCuc.coarseOctets != 0U
                   || profile.telemetryCuc.fineOctets != 0U
                   || profile.pusATmPacketSubcounterPresent
                   || profile.secondaryHeaderSpareOctets != 0U,
                   ErrorCode::INVALID_DATA,
                   "Generic CCSDS mission profiles cannot contain PUS tailoring.");
    return true;
  }

  RET_IF_ERR_MSG(profile.pusRevision != pus::Revision::A
                 && profile.pusRevision != pus::Revision::C,
                 ErrorCode::INVALID_DATA, "PUS is enabled without a supported explicit revision.");
  RET_IF_ERR_MSG(profile.direction != pus::Direction::Telecommand
                 && profile.direction != pus::Direction::Telemetry,
                 ErrorCode::INVALID_DATA, "PUS is enabled without an explicit packet direction.");
  RET_IF_ERR_MSG(!validIdentifierWidth(profile.sourceIdOctets)
                 || !validIdentifierWidth(profile.destinationIdOctets),
                 ErrorCode::INVALID_DATA, "PUS identifier widths must be 0, 1, 2, or 4 octets.");

  if (profile.direction == pus::Direction::Telecommand) {
    RET_IF_ERR_MSG(profile.destinationIdOctets != 0U || profile.telemetryTimestampPresent
                   || profile.telemetryTimeCode != time::Format::None
                   || profile.telemetryCuc.epoch != time::Epoch::Unspecified
                   || profile.telemetryCuc.pField != time::PFieldMode::Implicit
                   || profile.telemetryCuc.coarseOctets != 0U
                   || profile.telemetryCuc.fineOctets != 0U
                   || profile.pusATmPacketSubcounterPresent,
                   ErrorCode::INVALID_DATA, "A PUS TC profile contains TM-only tailoring.");
    RET_IF_ERR_MSG(profile.pusRevision == pus::Revision::C && profile.sourceIdOctets != 2U,
                   ErrorCode::INVALID_DATA, "PUS-C TC requires a two-octet source ID.");
  } else {
    RET_IF_ERR_MSG(profile.sourceIdOctets != 0U, ErrorCode::INVALID_DATA,
                   "A PUS TM profile cannot contain a source-ID width.");
    RET_IF_ERR_MSG(profile.pusRevision == pus::Revision::C && profile.destinationIdOctets != 2U,
                   ErrorCode::INVALID_DATA, "PUS-C TM requires a two-octet destination ID.");
    RET_IF_ERR_MSG(profile.pusRevision == pus::Revision::C
                   && profile.pusATmPacketSubcounterPresent,
                   ErrorCode::INVALID_DATA, "The PUS-A packet subcounter is not part of PUS-C TM.");

    if (profile.telemetryTimestampPresent) {
      RET_IF_ERR_MSG(profile.telemetryTimeCode != time::Format::Cuc,
                     ErrorCode::INVALID_DATA,
                     "A present TM timestamp requires the supported CUC format.");
      FORWARD_RESULT(time::validate(profile.telemetryCuc));
    } else {
      RET_IF_ERR_MSG(profile.telemetryTimeCode != time::Format::None
                     || profile.telemetryCuc.epoch != time::Epoch::Unspecified
                     || profile.telemetryCuc.pField != time::PFieldMode::Implicit
                     || profile.telemetryCuc.coarseOctets != 0U
                     || profile.telemetryCuc.fineOctets != 0U,
                     ErrorCode::INVALID_DATA, "An absent TM timestamp requires format None and size zero.");
    }
  }
  return true;
}

bool ccsds::missionProfilesEqual(const MissionProfile &lhs, const MissionProfile &rhs) {
  return lhs.pusEnabled == rhs.pusEnabled
         && lhs.pusRevision == rhs.pusRevision
         && lhs.direction == rhs.direction
         && lhs.sourceIdOctets == rhs.sourceIdOctets
         && lhs.destinationIdOctets == rhs.destinationIdOctets
         && lhs.packetErrorControl == rhs.packetErrorControl
         && lhs.telemetryTimestampPresent == rhs.telemetryTimestampPresent
         && lhs.telemetryTimeCode == rhs.telemetryTimeCode
         && lhs.telemetryCuc.epoch == rhs.telemetryCuc.epoch
         && lhs.telemetryCuc.pField == rhs.telemetryCuc.pField
         && lhs.telemetryCuc.coarseOctets == rhs.telemetryCuc.coarseOctets
         && lhs.telemetryCuc.fineOctets == rhs.telemetryCuc.fineOctets
         && lhs.pusATmPacketSubcounterPresent == rhs.pusATmPacketSubcounterPresent
         && lhs.secondaryHeaderSpareOctets == rhs.secondaryHeaderSpareOctets;
}

#ifndef CCSDS_MCU
ccsds::Result<ccsds::MissionProfile> ccsds::missionProfileFromConfig(
    const Config &config) {
  RET_IF_ERR_MSG(hasAnyKey(config, {"pus_version", "pus_event_id", "pus_time_code"}),
                 ErrorCode::CONFIG_FILE_ERROR,
                 "Config: legacy pus_version, pus_event_id, and pus_time_code fields were removed in v2.");

  std::string profileType;
  ASSIGN_CP(profileType, requiredString(config, "mission_profile"));
  PacketErrorControlMode errorControl{};
  ASSIGN_CP(errorControl, readPacketErrorControl(config));

  MissionProfile profile;
  profile.packetErrorControl = errorControl;
  if (profileType == "generic") {
    RET_IF_ERR_MSG(hasAnyKey(config, {
                     "pus_revision", "pus_direction", "pus_source_id_octets",
                     "pus_destination_id_octets", "pus_a_tm_packet_subcounter_present",
                     "secondary_header_spare_octets",
                     "pus_time_format", "pus_time_epoch", "pus_time_p_field",
                     "pus_time_coarse_octets", "pus_time_fine_octets",
                     "pus_service_type", "pus_service_subtype", "pus_acknowledgement_flags",
                     "pus_source_id", "pus_destination_id", "pus_packet_subcounter",
                     "pus_message_type_counter", "pus_time_reference_status",
                     "pus_time_coarse", "pus_time_fine"}),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: a generic mission profile cannot contain PUS fields.");
    const auto validation = validateMissionProfile(profile);
    if (!validation) return validation.error();
    return profile;
  }

  RET_IF_ERR_MSG(profileType != "pus", ErrorCode::CONFIG_FILE_ERROR,
                 "Config: mission_profile must be 'generic' or 'pus'.");

  std::string revisionValue;
  std::string directionValue;
  ASSIGN_CP(revisionValue, requiredString(config, "pus_revision"));
  ASSIGN_CP(directionValue, requiredString(config, "pus_direction"));
  const auto revision = revisionValue == "A" ? pus::Revision::A
                        : revisionValue == "C" ? pus::Revision::C
                        : pus::Revision::Unspecified;
  const auto direction = directionValue == "TC" ? pus::Direction::Telecommand
                         : directionValue == "TM" ? pus::Direction::Telemetry
                         : pus::Direction::Unspecified;
  RET_IF_ERR_MSG(revision == pus::Revision::Unspecified, ErrorCode::CONFIG_FILE_ERROR,
                 "Config: pus_revision must be 'A' or 'C'.");
  RET_IF_ERR_MSG(direction == pus::Direction::Unspecified, ErrorCode::CONFIG_FILE_ERROR,
                 "Config: pus_direction must be 'TC' or 'TM'.");

  profile = pus::makeProfile(revision, direction);
  profile.packetErrorControl = errorControl;
  if (config.isKey("secondary_header_spare_octets")) {
    int spare{};
    ASSIGN_CP(spare, requiredInt(config, "secondary_header_spare_octets", 0, 255));
    profile.secondaryHeaderSpareOctets = static_cast<std::uint8_t>(spare);
  }

  if (direction == pus::Direction::Telecommand) {
    int sourceWidth{};
    ASSIGN_CP(sourceWidth, requiredInt(config, "pus_source_id_octets", 0, 4));
    profile.sourceIdOctets = static_cast<std::uint8_t>(sourceWidth);
    RET_IF_ERR_MSG(hasAnyKey(config, {
                     "pus_destination_id_octets", "pus_a_tm_packet_subcounter_present",
                     "pus_time_format", "pus_time_epoch", "pus_time_p_field",
                     "pus_time_coarse_octets", "pus_time_fine_octets",
                     "pus_destination_id", "pus_packet_subcounter",
                     "pus_message_type_counter", "pus_time_reference_status",
                     "pus_time_coarse", "pus_time_fine"}),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: a PUS TC profile contains TM-only fields.");
  } else {
    int destinationWidth{};
    ASSIGN_CP(destinationWidth,
              requiredInt(config, "pus_destination_id_octets", 0, 4));
    profile.destinationIdOctets = static_cast<std::uint8_t>(destinationWidth);

    if (revision == pus::Revision::A
        && config.isKey("pus_a_tm_packet_subcounter_present")) {
      const auto enabled = config.get<bool>("pus_a_tm_packet_subcounter_present");
      if (!enabled) {
        return Error{ErrorCode::CONFIG_FILE_ERROR,
                     "Config: pus_a_tm_packet_subcounter_present must be bool."};
      }
      profile.pusATmPacketSubcounterPresent = enabled.value();
    }
    RET_IF_ERR_MSG(revision == pus::Revision::C
                   && config.isKey("pus_a_tm_packet_subcounter_present"),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: PUS-C TM cannot select the PUS-A packet subcounter.");

    std::string timeFormat;
    ASSIGN_CP(timeFormat, requiredString(config, "pus_time_format"));
    if (timeFormat == "none") {
      RET_IF_ERR_MSG(hasAnyKey(config, {
                       "pus_time_epoch", "pus_time_p_field", "pus_time_coarse_octets",
                       "pus_time_fine_octets", "pus_time_coarse", "pus_time_fine"}),
                     ErrorCode::CONFIG_FILE_ERROR,
                     "Config: disabled PUS time cannot contain CUC fields.");
    } else if (timeFormat == "cuc") {
      profile.telemetryTimestampPresent = true;
      profile.telemetryTimeCode = time::Format::Cuc;
      std::string epoch;
      std::string pField;
      ASSIGN_CP(epoch, requiredString(config, "pus_time_epoch"));
      ASSIGN_CP(pField, requiredString(config, "pus_time_p_field"));
      profile.telemetryCuc.epoch = epoch == "ccsds-1958-tai"
                                     ? time::Epoch::Ccsds1958Tai
                                   : epoch == "agency-defined"
                                     ? time::Epoch::AgencyDefined
                                     : time::Epoch::Unspecified;
      profile.telemetryCuc.pField = pField == "implicit" ? time::PFieldMode::Implicit
                                    : pField == "explicit" ? time::PFieldMode::Explicit
                                    : static_cast<time::PFieldMode>(0xFFU);
      int coarseOctets{};
      int fineOctets{};
      ASSIGN_CP(coarseOctets, requiredInt(config, "pus_time_coarse_octets", 1, 4));
      ASSIGN_CP(fineOctets, requiredInt(config, "pus_time_fine_octets", 0, 3));
      profile.telemetryCuc.coarseOctets = static_cast<std::uint8_t>(coarseOctets);
      profile.telemetryCuc.fineOctets = static_cast<std::uint8_t>(fineOctets);
    } else {
      return Error{ErrorCode::CONFIG_FILE_ERROR,
                   "Config: pus_time_format must be 'none' or 'cuc'."};
    }
  }

  const auto validation = validateMissionProfile(profile);
  if (!validation) {
    return Error{ErrorCode::CONFIG_FILE_ERROR,
                 "Config: invalid mission profile: " + validation.error().message()};
  }
  return profile;
}
#endif

std::string ccsds::pus::selector(const Revision revision, const Direction direction) {
  const char *rev = revision == Revision::A ? "revA"
                    : revision == Revision::C ? "revC" : "unspecified";
  const char *dir = direction == Direction::Telecommand ? "TC"
                    : direction == Direction::Telemetry ? "TM" : "unspecified";
  return std::string{"PUS:"} + rev + ":" + dir;
}
