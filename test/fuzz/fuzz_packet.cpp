// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSBuffer.h>
#include <CCSDSPacket.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
  if (size == 0U) return 0;

  ccsds::Packet packet;
  packet.setPacketErrorControlMode((data[0] & 0x01U) != 0U
                                     ? ccsds::PacketErrorControlMode::CRC16
                                     : ccsds::PacketErrorControlMode::None);
  const auto parsed = ccsds::buffer::deserializeBounded(packet, data, size);
  if (parsed) {
    if (parsed.value() > size) std::abort();
    const auto declared = ccsds::buffer::declaredPacketSize(data, size);
    if (!declared || declared.value() != parsed.value()) std::abort();
  }
  return 0;
}
