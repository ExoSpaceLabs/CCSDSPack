// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSBuffer.h
 * @brief Raw pointer-plus-size adapters for Packet and Manager APIs.
 *
 * The vector APIs remain the convenience surface. Packet parsing and Manager
 * stream loading delegate to pointer-native parsing paths, avoiding a copy of the
 * complete receive buffer. Owning C++ objects still copy state/data they retain.
 * Generation helpers such as setApplicationData() and addPacketFromBuffer() may
 * materialize owned vectors because the Manager/Packet ownership model requires it.
 */
#ifndef CCSDS_BUFFER_H
#define CCSDS_BUFFER_H

#include "CCSDSManager.h"
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include "ccsdspack/c/primary_header.h"
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
    RET_IF_ERR_MSG(size < CCSDS_PRIMARY_HEADER_SIZE, ErrorCode::INVALID_HEADER_DATA,
                   "Cannot inspect packet size: at least six primary-header bytes are required.");

    std::size_t declaredSize = 0U;
    const auto status = ccsds_packet_declared_size(data, size, &declaredSize);
    RET_IF_ERR_MSG(status != CCSDS_STATUS_OK, ErrorCode::INVALID_HEADER_DATA,
                   "Cannot inspect packet size: unsupported CCSDS packet version.");
    return declaredSize;
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
    return packet.deserialize(data, size);
  }

  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(data, size);
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
      data, size, std::forward<Args>(args)...);
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
      data, size, std::forward<Args>(args)...);
  }

  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::string &headerType, const std::int32_t headerSize = -1) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserialize(data, size, headerType, headerSize);
  }

  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::string &headerType, const std::int32_t headerSize = -1) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(data, size, headerType, headerSize);
  }

  [[nodiscard]] inline ResultBool deserialize(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::uint16_t headerDataSizeBytes) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserialize(data, size, headerDataSizeBytes);
  }

  [[nodiscard]] inline Result<std::size_t> deserializeBounded(
      Packet &packet, const std::uint8_t *data, const std::size_t size,
      const std::uint16_t headerDataSizeBytes) {
    RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                   "Cannot deserialize packet: raw buffer pointer is null.");
    RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                   "Cannot deserialize packet: raw buffer is empty.");
    return packet.deserializeBounded(data, size, headerDataSizeBytes);
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
    return manager.load(data, size);
  }

} // namespace ccsds::buffer

#endif // CCSDS_BUFFER_H
