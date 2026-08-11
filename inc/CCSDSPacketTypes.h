// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_PACKET_TYPES_H
#define CCSDS_PACKET_TYPES_H

#include <cstdint>

namespace ccsds {

  /** @brief Generic CCSDS packet direction derived from the primary-header Packet Type. */
  enum class PacketDirection : std::uint8_t {
    Unspecified = 0,
    Telemetry = 1,
    Telecommand = 2
  };

  /** @brief Packet-level CCSDSPack error-control trailer selection. */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0,
    CRC16 = 1
  };

  /** @brief Converts a concrete direction to the CCSDS primary-header Packet Type bit. */
  [[nodiscard]] constexpr std::uint8_t packetTypeForDirection(
      const PacketDirection direction) noexcept {
    return direction == PacketDirection::Telecommand ? 1U : 0U;
  }

  /** @brief Converts the CCSDS primary-header Packet Type bit to a direction. */
  [[nodiscard]] constexpr PacketDirection directionForPacketType(
      const std::uint8_t packetType) noexcept {
    return packetType == 0U ? PacketDirection::Telemetry
                            : PacketDirection::Telecommand;
  }

} // namespace ccsds

#endif // CCSDS_PACKET_TYPES_H
