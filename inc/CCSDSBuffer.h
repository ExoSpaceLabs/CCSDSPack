// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSBuffer.h
 * @brief Additive raw-buffer adapters for packet and Manager APIs.
 *
 * The existing std::vector APIs remain the convenience interface. These helpers
 * provide pointer-plus-size entry points for transport, DMA, UART, SpaceWire, and
 * other embedded buffers without changing the packet model. The current parsing
 * adapters copy into the existing vector-backed implementation internally. Keeping
 * the raw-buffer API separate allows those internals to evolve toward zero-copy or
 * heap-free parsing later without changing callers.
 */
#ifndef CCSDS_BUFFER_H
#define CCSDS_BUFFER_H

#include "CCSDSManager.h"
#include "CCSDSPacket.h"
#include "CCSDSResult.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ccsds::buffer {

  /**
   * @brief Returns the total Space Packet size declared by a six-byte primary header.
   * @param data Buffer beginning with a CCSDS Space Packet primary header.
   * @param size Number of readable bytes; at least six are required.
   * @return Total packet bytes (`6 + Packet Data Length + 1`) or a header error.
   *
   * Only the primary header is required. The packet body does not need to be present,
   * making this suitable for deciding how many more bytes a transport must receive.
   */
  [[nodiscard]] inline Result<std::size_t> declaredPacketSize(
      const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot inspect packet size: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size < 6U, ErrorCode::INVALID_HEADER_DATA,
                   "Cannot inspect packet size: at least six primary-header bytes are required.");

    const auto version = static_cast<std::uint8_t>((data[0] >> 5U) & 0x07U);
    RET_IF_ERR_MSG(version != 0U, ErrorCode::INVALID_HEADER_DATA,
                   "Cannot inspect packet size: unsupported CCSDS packet version.");

    const auto encodedLength = static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(data[4]) << 8U)
      | static_cast<std::uint16_t>(data[5]));
    return 6U + static_cast<std::size_t>(encodedLength) + 1U;
  }

  /** @brief Vector convenience overload of declaredPacketSize(). */
  [[nodiscard]] inline Result<std::size_t> declaredPacketSize(
      const std::vector<std::uint8_t> &data) {
    return declaredPacketSize(data.data(), data.size());
  }

  /**
   * @brief Parses one generic packet from a raw buffer.
   * @note The current implementation copies the supplied bytes before delegating to Packet.
   */
  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserialize(std::vector<std::uint8_t>(data, data + size));
  }

  /**
   * @brief Parses one generic packet from a raw buffer and reports consumed bytes.
   * @note The current implementation copies the supplied bytes before delegating to Packet.
   */
  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(std::vector<std::uint8_t>(data, data + size));
  }

  /**
   * @brief Parses one typed custom/PUS packet from a raw buffer.
   * @param headerType Registered custom type or canonical PUS selector.
   * @param headerSize Explicit variable custom-header size; PUS sizes come from the profile.
   */
  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::string &headerType, const std::int32_t headerSize = -1) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserialize(std::vector<std::uint8_t>(data, data + size),
                              headerType, headerSize);
  }

  /**
   * @brief Bounded typed custom/PUS parse from a raw buffer.
   * @param headerType Registered custom type or canonical PUS selector.
   * @param headerSize Explicit variable custom-header size; PUS sizes come from the profile.
   */
  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::string &headerType, const std::int32_t headerSize = -1) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(std::vector<std::uint8_t>(data, data + size),
                                     headerType, headerSize);
  }

  /** @brief Parses an opaque-secondary-header packet from a raw buffer. */
  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::uint16_t headerDataSizeBytes) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserialize(std::vector<std::uint8_t>(data, data + size),
                              headerDataSizeBytes);
  }

  /** @brief Bounded opaque-secondary-header parse from a raw buffer. */
  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::uint16_t headerDataSizeBytes) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(std::vector<std::uint8_t>(data, data + size),
                                     headerDataSizeBytes);
  }

  /** @brief Supplies Manager application data from a raw byte buffer. */
  [[nodiscard]] inline ResultBool setApplicationData(
      Manager &manager, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot set Manager application data: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::NO_DATA,
                   "Cannot set Manager application data: raw buffer is empty.");
    return manager.setApplicationData(std::vector<std::uint8_t>(data, data + size));
  }

  /** @brief Parses and adds one packet from a raw byte buffer. */
  [[nodiscard]] inline ResultBool addPacketFromBuffer(
      Manager &manager, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot add packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot add packet: raw buffer is empty.");
    return manager.addPacketFromBuffer(std::vector<std::uint8_t>(data, data + size));
  }

  /** @brief Transactionally loads a concatenated packet stream from a raw byte buffer. */
  [[nodiscard]] inline ResultBool load(
      Manager &manager, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot load packet stream: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot load packet stream: raw buffer is empty.");
    return manager.load(std::vector<std::uint8_t>(data, data + size));
  }

} // namespace ccsds::buffer

#endif // CCSDS_BUFFER_H
