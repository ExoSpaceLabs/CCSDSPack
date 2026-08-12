// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSBuffer.h>
#include <PusSecondaryHeaders.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace {
  std::uint8_t identifierWidth(const std::uint8_t selector) {
    constexpr std::array<std::uint8_t, 4U> widths{{0U, 1U, 2U, 4U}};
    return widths[selector & 0x03U];
  }
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
  if (size == 0U) return 0;

  ccsds::Packet packet;
  packet.setPacketErrorControlMode((data[0] & 0x04U) != 0U
                                     ? ccsds::PacketErrorControlMode::CRC16
                                     : ccsds::PacketErrorControlMode::None);

  const auto mode = static_cast<std::uint8_t>(data[0] & 0x03U);
  ccsds::Result<std::size_t> parsed = ccsds::Error{
    ccsds::ErrorCode::INVALID_DATA, "Fuzz parser not selected."};

  switch (mode) {
    case 0U: {
      ccsds::pus::rev_a::TcTailoring tailoring;
      tailoring.sourceIdOctets = identifierWidth(size > 1U ? data[1] : 0U);
      tailoring.secondaryHeaderSpareOctets = static_cast<std::uint8_t>(
        size > 2U ? data[2] % 3U : 0U);
      parsed = ccsds::buffer::deserializeBounded<ccsds::pus::rev_a::TcHeader>(
        packet, data, size, tailoring);
      break;
    }
    case 1U: {
      ccsds::pus::rev_a::TmTailoring tailoring;
      tailoring.destinationIdOctets = identifierWidth(size > 1U ? data[1] : 0U);
      tailoring.packetSubcounterPresent = size > 2U && (data[2] & 0x01U) != 0U;
      tailoring.timestampPresent = size > 2U && (data[2] & 0x02U) != 0U;
      tailoring.secondaryHeaderSpareOctets = static_cast<std::uint8_t>(
        size > 3U ? data[3] % 3U : 0U);
      if (tailoring.timestampPresent) {
        tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                         (size > 4U && (data[4] & 0x01U) != 0U)
                           ? ccsds::time::PFieldMode::Explicit
                           : ccsds::time::PFieldMode::Implicit,
                         static_cast<std::uint8_t>(1U + (size > 5U ? data[5] % 4U : 0U)),
                         static_cast<std::uint8_t>(size > 6U ? data[6] % 4U : 0U)};
      }
      parsed = ccsds::buffer::deserializeBounded<ccsds::pus::rev_a::TmHeader>(
        packet, data, size, tailoring);
      break;
    }
    case 2U: {
      ccsds::pus::rev_c::TcTailoring tailoring;
      tailoring.secondaryHeaderSpareOctets = static_cast<std::uint8_t>(
        size > 1U ? data[1] % 3U : 0U);
      parsed = ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TcHeader>(
        packet, data, size, tailoring);
      break;
    }
    default: {
      ccsds::pus::rev_c::TmTailoring tailoring;
      tailoring.timestampPresent = size > 1U && (data[1] & 0x01U) != 0U;
      tailoring.secondaryHeaderSpareOctets = static_cast<std::uint8_t>(
        size > 2U ? data[2] % 3U : 0U);
      if (tailoring.timestampPresent) {
        tailoring.cuc = {ccsds::time::Epoch::Ccsds1958Tai,
                         (size > 3U && (data[3] & 0x01U) != 0U)
                           ? ccsds::time::PFieldMode::Explicit
                           : ccsds::time::PFieldMode::Implicit,
                         static_cast<std::uint8_t>(1U + (size > 4U ? data[4] % 4U : 0U)),
                         static_cast<std::uint8_t>(size > 5U ? data[5] % 4U : 0U)};
      }
      parsed = ccsds::buffer::deserializeBounded<ccsds::pus::rev_c::TmHeader>(
        packet, data, size, tailoring);
      break;
    }
  }

  if (parsed && parsed.value() > size) std::abort();
  return 0;
}
