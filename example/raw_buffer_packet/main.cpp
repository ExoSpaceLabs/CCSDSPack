// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  ccsds::Packet outgoing;
  outgoing.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);

  const auto headerResult = outgoing.setPrimaryHeader(ccsds::PrimaryHeader{
    0, 0, 0, 0x321, ccsds::UNSEGMENTED, 7, 0
  });
  if (!headerResult) return headerResult.error().code();

  const std::vector<std::uint8_t> applicationData{0x10, 0x20, 0x30, 0x40};
  const auto dataResult = outgoing.setApplicationData(applicationData);
  if (!dataResult) return dataResult.error().code();

  const auto serialized = outgoing.serialize();
  if (!serialized) return serialized.error().code();
  const auto &wire = serialized.value();

  // A transport can inspect only the six primary-header bytes to determine how
  // many bytes belong to the complete Space Packet.
  const auto declared = ccsds::buffer::declaredPacketSize(wire.data(), 6U);
  if (!declared || declared.value() != wire.size()) return 1;

  // The raw-buffer API accepts memory owned by a UART/DMA/network receive buffer.
  // v2.0 currently bridges this call to the existing vector-backed parser; callers
  // do not need to change when that implementation becomes zero-copy later.
  ccsds::Packet incoming;
  incoming.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  const auto consumed = ccsds::buffer::deserializeBounded(
    incoming, wire.data(), wire.size());
  if (!consumed) {
    std::cerr << ccsds::errorCodeName(consumed.error().code())
              << ": " << consumed.error().message() << '\n';
    return consumed.error().code();
  }

  if (consumed.value() != wire.size()) return 2;
  if (incoming.getPrimaryHeader().getAPID() != 0x321U) return 3;
  if (incoming.getApplicationDataBytes() != applicationData) return 4;

  return 0;
}
