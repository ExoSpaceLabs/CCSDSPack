// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_MISSION_PROFILE_H
#define CCSDS_MISSION_PROFILE_H

#include <cstdint>
#include "CCSDSPacket.h"

namespace CCSDS {

  /**
   * @brief PUS revisions accepted by the CCSDSPack v2 standards-facing profile.
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

  /** @brief CCSDS time-code families expressible by a mission profile. */
  enum class TimeCodeFormat : std::uint8_t {
    None = 0,
    Cuc = 1,
    Cds = 2,
    Ccs = 3
  };

  /**
   * @brief Initial mission-tailoring contract for standards-facing v2 APIs.
   *
   * A default-constructed profile represents a generic CCSDS Space Packet and
   * does not silently enable PUS. Set pusEnabled and direction explicitly when
   * selecting an ECSS-E-ST-70-41C TC or TM profile.
   *
   * Width values are expressed in octets. Zero disables an optional field only
   * where the selected packet definition permits absence. Full profile validation
   * and codec integration are delivered by issues #66 and #67.
   */
  struct MissionProfile {
    bool pusEnabled{false};
    PusRevision pusRevision{PusRevision::C};
    PacketDirection direction{PacketDirection::Telemetry};
    std::uint8_t sourceIdOctets{0};
    std::uint8_t destinationIdOctets{0};
    PacketErrorControlMode packetErrorControl{PacketErrorControlMode::CRC16};
    bool telemetryTimestampPresent{false};
    TimeCodeFormat telemetryTimeCode{TimeCodeFormat::None};
    std::uint8_t telemetryTimeCodeOctets{0};
  };

} // namespace CCSDS

#endif // CCSDS_MISSION_PROFILE_H
