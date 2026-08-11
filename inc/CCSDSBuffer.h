// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSBuffer.h
 * @brief Raw pointer-plus-size adapters for Packet and Manager APIs.
 *
 * The vector APIs remain the convenience surface. Raw adapters currently bridge to
 * vector-backed storage internally so the signatures can later become zero-copy or
 * heap-free without changing callers.
 */
#ifndef CCSDS_BUFFER_H
#define CCSDS_BUFFER_H

#include "CCSDSManager.h"
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ccsds::buffer {

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
      (static_cast<std::uint16_t>(data[4]) << 8U) | static_cast<std::uint16_t>(data[5]));
    return 6U + static_cast<std::size_t>(encodedLength) + 1U;
  }

  [[nodiscard]] inline Result<std::size_t> declaredPacketSize(
      const std::vector<std::uint8_t> &data) {
    return declaredPacketSize(data.data(), data.size());
  }

  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserialize(std::vector<std::uint8_t>(data, data + size));
  }

  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(std::vector<std::uint8_t>(data, data + size));
  }

  /** @brief Typed raw parse using HeaderT default tailoring or supplied constructor arguments. */
  template <typename HeaderT, typename... Args>
  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size, Args&&... args) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.template deserialize<HeaderT>(
      std::vector<std::uint8_t>(data, data + size), std::forward<Args>(args)...);
  }

  /** @brief Typed bounded raw parse using HeaderT as the secondary-header schema. */
  template <typename HeaderT, typename... Args>
  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size, Args&&... args) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.template deserializeBounded<HeaderT>(
      std::vector<std::uint8_t>(data, data + size), std::forward<Args>(args)...);
  }

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

  [[nodiscard]] inline ResultBool setApplicationData(
      Manager &manager, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot set Manager application data: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::NO_DATA,
                   "Cannot set Manager application data: raw buffer is empty.");
    return manager.setApplicationData(std::vector<std::uint8_t>(data, data + size));
  }

  [[nodiscard]] inline ResultBool addPacketFromBuffer(
      Manager &manager, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot add packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot add packet: raw buffer is empty.");
    return manager.addPacketFromBuffer(std::vector<std::uint8_t>(data, data + size));
  }

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
