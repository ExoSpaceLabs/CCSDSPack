// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_MISSION_PROFILE_H
#define CCSDS_MISSION_PROFILE_H

#include <CCSDSResult.h>
#include <cstdint>
#include <string>

namespace CCSDS {

  /** @brief Packet error-control mode selected by a mission profile. */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0,  ///< No packet error-control field.
    CRC16 = 1  ///< CRC-16 packet error-control field.
  };

  /** @brief Supported ECSS PUS revisions. */
  enum class PusRevision : std::uint8_t {
    Unspecified = 0,  ///< No PUS revision selected.
    A = 1,            ///< ECSS-E-70-41A.
    C = 2             ///< ECSS-E-ST-70-41C.
  };

  /** @brief Packet direction used by a PUS secondary header. */
  enum class PacketDirection : std::uint8_t {
    Unspecified = 0,  ///< No packet direction selected.
    Telemetry = 1,    ///< Telemetry packet type.
    Telecommand = 2   ///< Telecommand packet type.
  };

  /** @brief Supported CCSDS time-code families for PUS telemetry. */
  enum class TimeCodeFormat : std::uint8_t {
    None = 0,  ///< No telemetry time code.
    Cuc = 1,   ///< CCSDS Unsegmented Time Code.
    Cds = 2    ///< CCSDS Day Segmented Time Code.
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

  /** @brief Builds a default PUS profile for one revision and direction. */
  [[nodiscard]] MissionProfile makePusProfile(PusRevision revision,
                                              PacketDirection direction);
  /** @brief Validates all generic CCSDS and PUS mission-profile fields. */
  [[nodiscard]] ResultBool validateMissionProfile(const MissionProfile &profile);
  /** @brief Returns whether two mission profiles select identical wire tailoring. */
  [[nodiscard]] bool missionProfilesEqual(const MissionProfile &lhs,
                                          const MissionProfile &rhs);
  /** @brief Returns the canonical selector for a supported PUS revision and direction. */
  [[nodiscard]] std::string pusSelector(PusRevision revision,
                                        PacketDirection direction);

} // namespace CCSDS

#endif // CCSDS_MISSION_PROFILE_H
