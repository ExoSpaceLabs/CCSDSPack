// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSManager.h"
#include "CCSDSUtils.h"
#include <algorithm>
#include <utility>

void CCSDS::Manager::setSyncPattern(const std::uint32_t syncPattern) {
  m_syncPattern = syncPattern;
}

std::uint32_t CCSDS::Manager::getSyncPattern() const {
  return m_syncPattern;
}

void CCSDS::Manager::setSyncPatternEnable(const bool enable) {
  m_syncPattEnable = enable;
}

bool CCSDS::Manager::getSyncPatternEnable() const {
  return m_syncPattEnable;
}

std::uint16_t CCSDS::Manager::packetIdentifier(const Packet &packet) {
  const auto &header = packet.getPrimaryHeader();
  return static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(header.getVersionNumber()) << 13U)
    | (static_cast<std::uint16_t>(header.getType()) << 12U)
    | (static_cast<std::uint16_t>(header.getDataFieldHeaderFlag()) << 11U)
    | header.getAPID());
}

bool CCSDS::Manager::hasIdentifierBinding() const {
  return m_templateIsSet || !m_packets.empty();
}

std::uint16_t CCSDS::Manager::boundPacketIdentifier() const {
  return m_templateIsSet ? packetIdentifier(m_templatePacket)
                         : packetIdentifier(m_packets.front());
}

CCSDS::ResultBool CCSDS::Manager::validatePacketIdentifier(const Packet &packet) const {
  if (!hasIdentifierBinding()) return true;
  RET_IF_ERR_MSG(packetIdentifier(packet) != boundPacketIdentifier(),
                 ErrorCode::INVALID_HEADER_DATA,
                 "Packet identifier does not match the Manager-bound stream identifier");
  return true;
}

CCSDS::PacketErrorControlMode CCSDS::Manager::boundPacketErrorControlMode() const {
  if (m_templateIsSet) return m_templatePacket.getPacketErrorControlMode();
  if (!m_packets.empty()) return m_packets.front().getPacketErrorControlMode();
  return PacketErrorControlMode::CRC16;
}

void CCSDS::Manager::advanceSequenceCount() {
  if (!getAutoSequenceCountEnable()) return;
  const auto next = static_cast<std::uint16_t>((getSequenceCount() + 1U) & SEQUENCE_COUNT_MASK);
  m_sequenceCount = (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) | next;
}

void CCSDS::Manager::syncSequenceCountFromPacket(const Packet &packet) {
  if (!getAutoSequenceCountEnable()) return;
  const auto next = static_cast<std::uint16_t>(
    (packet.getPrimaryHeader().getSequenceCount() + 1U) & SEQUENCE_COUNT_MASK);
  m_sequenceCount = (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) | next;
}

CCSDS::ResultBool CCSDS::Manager::setPacketTemplate(Packet packet) {
  RET_IF_ERR_MSG(m_templateIsSet, ErrorCode::TEMPLATE_SET_FAILURE,
                 "Cannot set Template as it is already set, please clear Manager first");
  RET_IF_ERR_MSG(packet.getPrimaryHeader().getHeaderStatus() == INVALID,
                 ErrorCode::INVALID_HEADER_DATA, "Cannot set an invalid packet template");

  packet.update();
  RET_IF_ERR_MSG(packet.getPrimaryHeader().getHeaderStatus() == INVALID,
                 ErrorCode::INVALID_HEADER_DATA, "Cannot finalize an invalid packet template");

  m_templatePacket = std::move(packet);
  m_sequenceCount = (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK)
                    | (m_templatePacket.getPrimaryHeader().getSequenceCount()
                       & SEQUENCE_COUNT_MASK);
  m_templatePacket.setUpdatePacketEnable(false);
  m_validator.clear();
  m_validator.setTemplatePacket(m_templatePacket);
  m_validator.configure(true, true, true);
  m_validateEnable = true;
  m_templateIsSet = true;
  return true;
}

CCSDS::ResultBool CCSDS::Manager::loadTemplateConfigFile(const std::string &configPath) {
  Packet templatePacket;
  FORWARD_RESULT(templatePacket.loadFromConfigFile(configPath));
  FORWARD_RESULT(setPacketTemplate(std::move(templatePacket)));
  return true;
}

#ifndef CCSDS_MCU
CCSDS::ResultBool CCSDS::Manager::loadTemplateConfig(const Config &cfg) {
  Packet templatePacket;
  FORWARD_RESULT(templatePacket.loadFromConfig(cfg));
  FORWARD_RESULT(setPacketTemplate(std::move(templatePacket)));
  return true;
}
#endif

void CCSDS::Manager::setDataFieldSize(const std::uint16_t size) {
  m_templatePacket.setDataFieldSize(size);
}

std::uint16_t CCSDS::Manager::getDataFieldSize() const {
  return m_templatePacket.getDataFieldMaximumSize();
}

void CCSDS::Manager::setAutoSequenceCountEnable(const bool enable) {
  if (enable) {
    m_sequenceCount &= static_cast<std::uint16_t>(~AUTO_SEQUENCE_DISABLED_MASK);
  } else {
    m_sequenceCount |= AUTO_SEQUENCE_DISABLED_MASK;
  }
}

CCSDS::ResultBool CCSDS::Manager::setSequenceCount(const std::uint16_t count) {
  RET_IF_ERR_MSG(count > SEQUENCE_COUNT_MASK, ErrorCode::INVALID_HEADER_DATA,
                 "Unable to set Manager sequence count above 16383");
  if (m_templateIsSet) {
    FORWARD_RESULT(m_templatePacket.setSequenceCount(count));
  }
  m_sequenceCount = (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) | count;
  return true;
}

CCSDS::ResultBool CCSDS::Manager::setApplicationData(
    const std::vector<std::uint8_t> &data) {
  RET_IF_ERR_MSG(data.empty(), ErrorCode::NO_DATA,
                 "Cannot set Application data, Provided data is empty");
  RET_IF_ERR_MSG(!m_templateIsSet, ErrorCode::INVALID_HEADER_DATA,
                 "Cannot set Application data, No template has been set");
  RET_IF_ERR_MSG(m_templatePacket.getPrimaryHeader().getHeaderStatus() == INVALID,
                 ErrorCode::INVALID_HEADER_DATA,
                 "Cannot set Application data using an invalid packet template");

  const auto maxBytesPerPacket = m_templatePacket.getDataFieldMaximumSize();
  RET_IF_ERR_MSG(maxBytesPerPacket == 0U, ErrorCode::INVALID_APPLICATION_DATA,
                 "Cannot segment application data into a zero-sized packet data field");

  const auto packetCount =
    (data.size() + static_cast<std::size_t>(maxBytesPerPacket) - 1U)
    / static_cast<std::size_t>(maxBytesPerPacket);

  std::vector<Packet> generated;
  generated.reserve(packetCount);
  auto nextCount = getSequenceCount();

  std::size_t offset = 0U;
  for (std::size_t index = 0U; index < packetCount; ++index) {
    Packet packet = m_templatePacket;
    const auto bytes = std::min<std::size_t>(maxBytesPerPacket, data.size() - offset);
    const std::vector<std::uint8_t> chunk(
      data.begin() + static_cast<std::ptrdiff_t>(offset),
      data.begin() + static_cast<std::ptrdiff_t>(offset + bytes));

    ESequenceFlag flags = UNSEGMENTED;
    if (packetCount > 1U) {
      if (index == 0U) flags = FIRST_SEGMENT;
      else if (index + 1U == packetCount) flags = LAST_SEGMENT;
      else flags = CONTINUING_SEGMENT;
    }

    packet.setSequenceFlags(flags);
    FORWARD_RESULT(packet.setSequenceCount(nextCount));
    FORWARD_RESULT(packet.setApplicationData(chunk));
    packet.setUpdatePacketEnable(m_updateEnable);
    generated.push_back(std::move(packet));

    if (getAutoSequenceCountEnable()) {
      nextCount = static_cast<std::uint16_t>((nextCount + 1U) & SEQUENCE_COUNT_MASK);
    }
    offset += bytes;
  }

  m_packets = std::move(generated);
  m_sequenceCount = (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) | nextCount;
  return true;
}

void CCSDS::Manager::setAutoUpdateEnable(const bool enable) {
  m_updateEnable = enable;
  for (auto &packet : m_packets) packet.setUpdatePacketEnable(enable);
}

void CCSDS::Manager::setAutoValidateEnable(const bool enable) {
  m_validateEnable = enable;
}

CCSDS::ResultBuffer CCSDS::Manager::getPacketTemplate() {
  auto packet = m_templatePacket;
  const auto data = packet.serialize();
  RET_IF_ERR_MSG(data.empty(), ErrorCode::NO_DATA,
                 "Cannot get Packet template data, data is empty");
  return data;
}

CCSDS::ResultBuffer CCSDS::Manager::getPacketBufferAtIndex(const std::uint16_t index) {
  RET_IF_ERR_MSG(index >= m_packets.size(), ErrorCode::INVALID_DATA,
                 "Cannot get packet, index is out of bounds");

  auto packet = m_packets[index];
  if (m_validateEnable) {
    packet.update();
    const std::string errorMessage =
      "Validation failure for packet at index " + std::to_string(index);
    RET_IF_ERR_MSG(!m_validator.validate(packet), ErrorCode::VALIDATION_FAILURE,
                   errorMessage);
  }

  const auto packetBuffer = packet.serialize();
  RET_IF_ERR_MSG(packetBuffer.empty(), ErrorCode::INVALID_HEADER_DATA,
                 "Cannot serialize packet with an invalid header");
  return packetBuffer;
}

std::vector<std::uint8_t> CCSDS::Manager::getPacketsBuffer() const {
  std::vector<std::uint8_t> buffer;
  for (auto packet : m_packets) {
    if (m_syncPattEnable) {
      buffer.push_back(static_cast<std::uint8_t>((m_syncPattern >> 24U) & 0xFFU));
      buffer.push_back(static_cast<std::uint8_t>((m_syncPattern >> 16U) & 0xFFU));
      buffer.push_back(static_cast<std::uint8_t>((m_syncPattern >> 8U) & 0xFFU));
      buffer.push_back(static_cast<std::uint8_t>(m_syncPattern & 0xFFU));
    }

    const auto packetBuffer = packet.serialize();
    if (packetBuffer.empty()) return {};
    buffer.insert(buffer.end(), packetBuffer.begin(), packetBuffer.end());
  }
  return buffer;
}

CCSDS::ResultBuffer CCSDS::Manager::getApplicationDataBuffer() {
  RET_IF_ERR_MSG(m_packets.empty(), ErrorCode::NO_DATA,
                 "Cannot get Application data, no packets have been set.");
  std::vector<std::uint8_t> data;

  for (std::size_t index = 0U; index < m_packets.size(); ++index) {
    if (m_validateEnable) {
      const std::string errorMessage =
        "Validation failure for packet at index " + std::to_string(index);
      RET_IF_ERR_MSG(!m_validator.validate(m_packets[index]),
                     ErrorCode::VALIDATION_FAILURE, errorMessage);
    }
    const auto applicationData = m_packets[index].getApplicationDataBytes();
    data.insert(data.end(), applicationData.begin(), applicationData.end());
  }
  return data;
}

CCSDS::ResultBuffer CCSDS::Manager::getApplicationDataBufferAtIndex(
    const std::uint16_t index) {
  RET_IF_ERR_MSG(index >= m_packets.size(), ErrorCode::INVALID_DATA,
                 "Cannot get Application data, index is out of bounds");
  return m_packets[index].getApplicationDataBytes();
}

std::uint16_t CCSDS::Manager::getTotalPackets() const {
  return static_cast<std::uint16_t>(m_packets.size());
}

std::vector<CCSDS::Packet> CCSDS::Manager::getPackets() {
  return m_packets;
}

std::vector<CCSDS::Packet> CCSDS::Manager::getPackets() const {
  return m_packets;
}

CCSDS::ResultBool CCSDS::Manager::addPacket(Packet packet) {
  RET_IF_ERR_MSG(packet.getPrimaryHeader().getHeaderStatus() == INVALID,
                 ErrorCode::INVALID_HEADER_DATA,
                 "Cannot add packet with an invalid primary header");
  FORWARD_RESULT(validatePacketIdentifier(packet));

  if (m_validateEnable && !m_updateEnable) {
    auto validator = m_validator;
    if (!m_templateIsSet && m_packets.empty()) {
      validator.configure(true, true, false);
    }
    RET_IF_ERR_MSG(!validator.validate(packet), ErrorCode::VALIDATION_FAILURE,
                   "packet is not valid");
    m_validator = std::move(validator);
  }

  packet.setUpdatePacketEnable(m_updateEnable);
  m_packets.push_back(std::move(packet));
  syncSequenceCountFromPacket(m_packets.back());
  return true;
}

CCSDS::ResultBool CCSDS::Manager::addPacketFromBuffer(
    const std::vector<std::uint8_t> &packetBuffer) {
  Packet packet;
  packet.setPacketErrorControlMode(boundPacketErrorControlMode());
  FORWARD_RESULT(packet.deserialize(packetBuffer));
  FORWARD_RESULT(addPacket(std::move(packet)));
  return true;
}

CCSDS::ResultBool CCSDS::Manager::load(const std::vector<Packet> &packets) {
  Manager staged = *this;
  for (const auto &packet : packets) {
    const auto result = staged.addPacket(packet);
    if (!result) return result.error();
  }
  *this = std::move(staged);
  return true;
}

CCSDS::ResultBool CCSDS::Manager::load(
    const std::vector<std::uint8_t> &packetsBuffer) {
  RET_IF_ERR_MSG(packetsBuffer.size() < 7U, ErrorCode::INVALID_DATA,
                 "invalid packet buffer size");

  Manager staged = *this;
  std::size_t offset{0U};
  while (offset < packetsBuffer.size()) {
    if (staged.m_syncPattEnable) {
      RET_IF_ERR_MSG(packetsBuffer.size() - offset < 4U, ErrorCode::INVALID_DATA,
                     "Truncated sync pattern.");
      const std::uint32_t value =
        (static_cast<std::uint32_t>(packetsBuffer[offset]) << 24U)
        | (static_cast<std::uint32_t>(packetsBuffer[offset + 1U]) << 16U)
        | (static_cast<std::uint32_t>(packetsBuffer[offset + 2U]) << 8U)
        | static_cast<std::uint32_t>(packetsBuffer[offset + 3U]);
      RET_IF_ERR_MSG(value != staged.m_syncPattern, ErrorCode::INVALID_DATA,
                     "Sync Pattern mismatch.");
      offset += 4U;
    }

    RET_IF_ERR_MSG(packetsBuffer.size() - offset < 6U, ErrorCode::INVALID_DATA,
                   "Truncated CCSDS primary header.");

    const std::vector<std::uint8_t> remaining(
      packetsBuffer.begin() + static_cast<std::ptrdiff_t>(offset),
      packetsBuffer.end());
    Packet packet;
    packet.setPacketErrorControlMode(staged.boundPacketErrorControlMode());
    std::size_t consumed{};
    ASSIGN_CP(consumed, packet.deserializeBounded(remaining));
    const auto addResult = staged.addPacket(std::move(packet));
    if (!addResult) return addResult.error();
    offset += consumed;
  }

  *this = std::move(staged);
  return true;
}

CCSDS::ResultBool CCSDS::Manager::read(const std::string &binaryFile) {
  std::vector<std::uint8_t> buffer;
  ASSIGN_CP(buffer, readBinaryFile(binaryFile));
  FORWARD_RESULT(load(buffer));
  return true;
}

CCSDS::ResultBool CCSDS::Manager::write(const std::string &binaryFile) const {
  const auto buffer = getPacketsBuffer();
  RET_IF_ERR_MSG(!m_packets.empty() && buffer.empty(),
                 ErrorCode::INVALID_HEADER_DATA,
                 "Cannot write packet stream containing an invalid header");
  FORWARD_RESULT(writeBinaryFile(buffer, binaryFile));
  return true;
}

CCSDS::ResultBool CCSDS::Manager::readTemplate(const std::string &filename) {
  Packet templatePacket;
  templatePacket.setUpdatePacketEnable(false);
  if (stringEndsWith(filename, ".bin")) {
    std::vector<std::uint8_t> buffer;
    ASSIGN_CP(buffer, readBinaryFile(filename));
    FORWARD_RESULT(templatePacket.deserialize(buffer));
  } else if (stringEndsWith(filename, ".cfg")) {
    FORWARD_RESULT(templatePacket.loadFromConfigFile(filename));
  } else {
    return Error{INVALID_DATA,
                 "Cannot load template, invalid file provided [supported extensions [.bin, .cfg]]"};
  }
  FORWARD_RESULT(setPacketTemplate(std::move(templatePacket)));
  return true;
}

void CCSDS::Manager::clear() {
  m_packets.clear();
  m_templateIsSet = false;
  m_templatePacket = {};
  m_sequenceCount &= AUTO_SEQUENCE_DISABLED_MASK;
  m_validator.clear();
}

void CCSDS::Manager::clearPackets() {
  m_packets.clear();
  m_sequenceCount &= AUTO_SEQUENCE_DISABLED_MASK;
  m_validator.clear();
  if (m_templateIsSet) {
    m_validator.setTemplatePacket(m_templatePacket);
    m_validator.configure(true, true, true);
  }
}
