// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_MISSION_PROFILE_H
#define CCSDS_MISSION_PROFILE_H

#include <cstdint>

namespace CCSDS {

  /**
   * @brief PUS revisions accepted by the CCSDSPack v2+ compliance profile.
   *
   * ECSS-E-ST-70-41C encodes its PUS version number as 2. PUS-A is
   * intentionally not represented because it is deferred from v2.0.0.
   */
  enum class PusRevision : std::uint8_t {
    C = 2
  };

  /** @brief Packet direction, independent from the selected PUS revision. */
  enum class PacketDirection : std::uint8_t {
    Telemetry = 0,
    Telecommand = 1
  };

  /** @brief Packet error-control modes exposed by the profile. */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0,
    Crc16Ccitt = 1
  };

  /** @brief CCSDS time-code families supported by a mission profile. */
  enum class TimeCodeFormat : std::uint8_t {
    None = 0,
    Cuc = 1,
    Cds = 2,
    Ccs = 3
  };

  /**
   * @brief Initial mission-tailoring contract for standards-facing v2+ APIs.
   *
   * Width values are expressed in octets. Zero disables an optional field.
   * Profile validation and codec integration are implemented in Phase 3.
   */
  struct MissionProfile {
    PusRevision pusRevision{PusRevision::C};
    std::uint8_t sourceIdOctets{1};
    std::uint8_t destinationIdOctets{1};
    PacketErrorControlMode packetErrorControl{PacketErrorControlMode::Crc16Ccitt};
    bool telemetryTimestampPresent{false};
    TimeCodeFormat telemetryTimeCode{TimeCodeFormat::None};
    std::uint8_t telemetryTimeCodeOctets{0};
  };

} // namespace CCSDS

#endif // CCSDS_MISSION_PROFILE_H
