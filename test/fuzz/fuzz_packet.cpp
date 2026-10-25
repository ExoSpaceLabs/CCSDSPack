// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSBuffer.h>
#include <CCSDSPacket.h>
#include <ccsdspack/c/ccsdspack.h>
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
  // Exercise C11 contiguous/split wire policy on identical complete PDU bytes.
  // Incomplete or concatenated data deliberately has different framing policy.
  if (size >= CCSDS_PRIMARY_HEADER_SIZE) {
    size_t declared = 0U;
    if (ccsds_packet_declared_size(data, size, &declared) == CCSDS_STATUS_OK
        && size == declared) {
      const auto mode = (data[0] & 0x01U) != 0U
        ? CCSDS_PACKET_ERROR_CONTROL_CRC16
        : CCSDS_PACKET_ERROR_CONTROL_NONE;
      ccsds_packet_parts_view_t splitView{};
      ccsds_packet_view_t contiguousView{};
      ccsds_packet_parse_reason_t splitReason = CCSDS_PACKET_PARSE_OK;
      ccsds_packet_parse_reason_t contiguousReason = CCSDS_PACKET_PARSE_OK;
      const auto splitStatus = ccsds_packet_parts_parse(
        data, CCSDS_PRIMARY_HEADER_SIZE,
        data + CCSDS_PRIMARY_HEADER_SIZE, size - CCSDS_PRIMARY_HEADER_SIZE,
        mode, nullptr, &splitView, &splitReason);
      const auto contiguousStatus = ccsds_packet_view_parse_ex(
        data, size, mode, nullptr, &contiguousView, &contiguousReason);
      if (splitStatus != contiguousStatus || splitReason != contiguousReason) std::abort();
      if (splitStatus == CCSDS_STATUS_OK) {
        if (splitView.consumed != contiguousView.consumed
            || splitView.data_field.size != contiguousView.data_field.size
            || splitView.received_crc16 != contiguousView.received_crc16)
          std::abort();
      }
    }
  }
  return 0;
}
