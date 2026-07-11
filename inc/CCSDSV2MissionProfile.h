// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_V2_MISSION_PROFILE_H
#define CCSDS_V2_MISSION_PROFILE_H

#include <cstdint>

namespace CCSDS::v2 {

  /**
   * @brief PUS revisions accepted by the CCSDSPack v2 compliance profile.
   *
   * ECSS-E-ST-70-41C encodes its PUS version number as 2. PUS-A is
   * intentionally not represented because it is deferred from v2.0.0.
   */
  enum class PusRevision : std::uint8_t {
    C = 2
  };

  /** @brief Packet error-control modes exposed by the v2 mission profile. */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0