// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_MISSION_PROFILE_H
#define CCSDS_MISSION_PROFILE_H

#include <CCSDSResult.h>
#include <cstdint>
#include <string>

namespace CCSDS {

  enum class PacketErrorControlMode : std::uint8_t {
    None = 0,
    CRC16 = 1
  };

  enum class PusRevision : std::uint8_t {
    Unspecified = 0,
    A = 1,
    C = 2
  };

  enum class PacketDirection : std::uint8_t {
    Unspecified = 0,
    Telemetry = 1,
    Telecommand = 2
  };

  enum class TimeCodeFormat : std::uint8_t {
    None = 0,
    Cuc = 1,
    Cds = 2
  };

  /**
   * @brief Explicit mission tailoring used by the standards-facing PUS codecs.
   *
   * A default profile is generic CCSDS. A PUS profile must explicitly select
   * both revision and direction. Identifier and time widths are octet counts;
   * secondaryHeaderSpareOctets models only octet-aligned spare fields.
   */
  struct MissionProfile {
    bool pusEnabled{false};
    PusRevision pusRevision{PusRevision::Unspecified};
    PacketDirection direction{PacketDirection::Unspecified};
    std::uint8_t sourceIdOctets{0};
    std::uint8_t destinationIdOctets{0};
    PacketErrorControlMode packetErrorControl{PacketErrorControlMode::CRC16};
    bool telemetryTimestampPresent{false};
    TimeCodeFormat telemetryTimeCode{TimeCodeFormat::None};
    std::uint8_t telemetryTimeCodeOctets{0};
    bool pusATmPacketSubcounterPresent{false};
    std::uint8_t secondaryHeaderSpareOctets{0};
  };

  [[nodiscard]] MissionProfile makePusProfile(PusRevision revision,
                                              PacketDirection direction);
  [[nodiscard]] ResultBool validateMissionProfile(const MissionProfile &profile);
  [[nodiscard]] bool missionProfilesEqual(const MissionProfile &lhs,
                                          const MissionProfile &rhs);
  [[nodiscard]] std::string pusSelector(PusRevision revision,
                                        PacketDirection direction);

} // namespace CCSDS

#endif // CCSDS_MISSION_PROFILE_H
