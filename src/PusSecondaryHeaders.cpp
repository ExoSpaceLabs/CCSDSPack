// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaders.h"
#include <utility>

namespace {
  CCSDS::ResultBool validateHeaderProfile(const CCSDS::MissionProfile &profile,
                                          const CCSDS::PusRevision revision,
                                          const CCSDS::PacketDirection direction) {
    FORWARD_RESULT(CCSDS::validateMissionProfile(profile));
    RET_IF_ERR_MSG(profile.pusRevision != revision || profile.direction != direction,
                   CCSDS::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "PUS header concrete type does not match its mission profile.");
    return true;
  }
}

#ifndef CCSDS_MCU
CCSDS::ResultBool CCSDS::PusSecondaryHeader::loadFromConfig(const Config &config) {
  (void)config;
  return true;
}
#endif

bool CCSDS::PusSecondaryHeader::identifierFits(const std::uint32_t value,
                                               const std::uint8_t octets) const {
  if (octets == 0U) return value == 0U;
  if (octets >= 4U) return true;
  return value < (1UL << (octets * 8U));
}

void CCSDS::PusSecondaryHeader::appendIdentifier(std::vector<std::uint8_t> &bytes,
                                                 const std::uint32_t value,
                                                 const std::uint8_t octets) {
  for (std::uint8_t i = 0; i < octets; ++i) {
    const auto shift = static_cast<std::uint8_t>((octets - i - 1U) * 8U);
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
  }
}

std::uint32_t CCSDS::PusSecondaryHeader::readIdentifier(
    const std::vector<std::uint8_t> &bytes, const std::size_t offset,
    const std::uint8_t octets) {
  std::uint32_t result = 0U;
  for (std::uint8_t i = 0; i < octets; ++i) {
    result = static_cast<std::uint32_t>((result << 8U) | bytes[offset + i]);
  }
  return result;
}

bool CCSDS::PusSecondaryHeader::trailingSpareIsZero(
    const std::vector<std::uint8_t> &bytes) const {
  const auto spare = static_cast<std::size_t>(m_profile.secondaryHeaderSpareOctets);
  if (spare > bytes.size()) return false;
  for (std::size_t i = bytes.size() - spare; i < bytes.size(); ++i) {
    if (bytes[i] != 0U) return false;
  }
  return true;
}

CCSDS::PusTcSecondaryHeader::PusTcSecondaryHeader(
    MissionProfile profile, const std::uint8_t serviceType,
    const std::uint8_t serviceSubtype, const std::uint32_t sourceId,
    const std::uint8_t acknowledgementFlags)
  : PusSecondaryHeader(std::move(profile)),
    m_acknowledgementFlags(acknowledgementFlags), m_serviceType(serviceType),
    m_serviceSubtype(serviceSubtype), m_sourceId(sourceId) {}

CCSDS::ResultBool CCSDS::PusTcSecondaryHeader::setAcknowledgementFlags(
    const std::uint8_t flags) {
  RET_IF_ERR_MSG(flags > 0x0FU, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS acknowledgement flags exceed four bits.");
  m_acknowledgementFlags = flags;
  return true;
}

CCSDS::ResultBool CCSDS::PusTcSecondaryHeader::setSourceId(const std::uint32_t value) {
  RET_IF_ERR_MSG(!identifierFits(value, m_profile.sourceIdOctets),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS source ID does not fit the configured width.");
  m_sourceId = value;
  return true;
}

std::uint16_t CCSDS::PusTcSecondaryHeader::tcSize() const {
  return static_cast<std::uint16_t>(3U + m_profile.sourceIdOctets
                                    + m_profile.secondaryHeaderSpareOctets);
}

void CCSDS::PusTcSecondaryHeader::appendTcBody(std::vector<std::uint8_t> &bytes) const {
  bytes.push_back(m_serviceType);
  bytes.push_back(m_serviceSubtype);
  appendIdentifier(bytes, m_sourceId, m_profile.sourceIdOctets);
  bytes.insert(bytes.end(), m_profile.secondaryHeaderSpareOctets, 0U);
}

CCSDS::ResultBool CCSDS::PusTcSecondaryHeader::parseTcBody(
    const std::vector<std::uint8_t> &data, const std::size_t offset) {
  RET_IF_ERR_MSG(data.size() != tcSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TC secondary-header size does not match the mission profile.");
  RET_IF_ERR_MSG(!trailingSpareIsZero(data), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TC secondary-header spare octets must be zero.");
  const auto source = readIdentifier(data, offset + 2U, m_profile.sourceIdOctets);
  m_serviceType = data[offset];
  m_serviceSubtype = data[offset + 1U];
  m_sourceId = source;
  return true;
}

CCSDS::PusTmSecondaryHeader::PusTmSecondaryHeader(
    MissionProfile profile, const std::uint8_t serviceType,
    const std::uint8_t serviceSubtype, const std::uint32_t destinationId,
    std::vector<std::uint8_t> timestamp)
  : PusSecondaryHeader(std::move(profile)), m_serviceType(serviceType),
    m_serviceSubtype(serviceSubtype), m_destinationId(destinationId),
    m_timestamp(std::move(timestamp)) {}

CCSDS::ResultBool CCSDS::PusTmSecondaryHeader::setDestinationId(const std::uint32_t value) {
  RET_IF_ERR_MSG(!identifierFits(value, m_profile.destinationIdOctets),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS destination ID does not fit the configured width.");
  m_destinationId = value;
  return true;
}

CCSDS::ResultBool CCSDS::PusTmSecondaryHeader::setTimestamp(
    const std::vector<std::uint8_t> &timestamp) {
  const auto expected = m_profile.telemetryTimestampPresent
                          ? m_profile.telemetryTimeCodeOctets : 0U;
  RET_IF_ERR_MSG(timestamp.size() != expected, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TM timestamp size does not match the mission profile.");
  m_timestamp = timestamp;
  return true;
}

void CCSDS::PusTmSecondaryHeader::appendTmTail(std::vector<std::uint8_t> &bytes) const {
  appendIdentifier(bytes, m_destinationId, m_profile.destinationIdOctets);
  bytes.insert(bytes.end(), m_timestamp.begin(), m_timestamp.end());
  bytes.insert(bytes.end(), m_profile.secondaryHeaderSpareOctets, 0U);
}

CCSDS::ResultBool CCSDS::PusTmSecondaryHeader::parseTmTail(
    const std::vector<std::uint8_t> &data, const std::size_t offset) {
  const auto tail = tmTailSize();
  RET_IF_ERR_MSG(offset + tail != data.size(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TM secondary-header size does not match the mission profile.");
  RET_IF_ERR_MSG(!trailingSpareIsZero(data), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TM secondary-header spare octets must be zero.");
  const auto destination = readIdentifier(data, offset, m_profile.destinationIdOctets);
  const auto timestampOffset = offset + m_profile.destinationIdOctets;
  const auto timestampEnd = timestampOffset + m_profile.telemetryTimeCodeOctets;
  std::vector<std::uint8_t> timestamp(data.begin() + static_cast<std::ptrdiff_t>(timestampOffset),
                                      data.begin() + static_cast<std::ptrdiff_t>(timestampEnd));
  m_destinationId = destination;
  m_timestamp = std::move(timestamp);
  return true;
}

std::uint16_t CCSDS::PusTmSecondaryHeader::tmTailSize() const {
  return static_cast<std::uint16_t>(m_profile.destinationIdOctets
                                    + m_profile.telemetryTimeCodeOctets
                                    + m_profile.secondaryHeaderSpareOctets);
}

CCSDS::PusATcHeader::PusATcHeader(MissionProfile profile, const std::uint8_t serviceType,
                                  const std::uint8_t serviceSubtype,
                                  const std::uint32_t sourceId,
                                  const std::uint8_t acknowledgementFlags)
  : PusTcSecondaryHeader(std::move(profile), serviceType, serviceSubtype,
                         sourceId, acknowledgementFlags) {}

CCSDS::ResultBool CCSDS::PusATcHeader::deserialize(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateHeaderProfile(m_profile, PusRevision::A, PacketDirection::Telecommand));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TC secondary-header size mismatch.");
  RET_IF_ERR_MSG((data[0] & 0x80U) != 0U || ((data[0] >> 4U) & 0x07U) != 1U,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TC reserved bit or version is invalid.");
  const auto result = parseTcBody(data, 1U);
  if (!result) return result.error();
  m_acknowledgementFlags = data[0] & 0x0FU;
  return true;
}

std::vector<std::uint8_t> CCSDS::PusATcHeader::serialize() const {
  if (!profileIsValid() || m_profile.pusRevision != PusRevision::A
      || m_profile.direction != PacketDirection::Telecommand
      || m_acknowledgementFlags > 0x0FU
      || !identifierFits(m_sourceId, m_profile.sourceIdOctets)) return {};
  std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(0x10U | m_acknowledgementFlags)};
  appendTcBody(bytes);
  return bytes;
}

CCSDS::PusCTcHeader::PusCTcHeader(MissionProfile profile, const std::uint8_t serviceType,
                                  const std::uint8_t serviceSubtype,
                                  const std::uint32_t sourceId,
                                  const std::uint8_t acknowledgementFlags)
  : PusTcSecondaryHeader(std::move(profile), serviceType, serviceSubtype,
                         sourceId, acknowledgementFlags) {}

CCSDS::ResultBool CCSDS::PusCTcHeader::deserialize(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateHeaderProfile(m_profile, PusRevision::C, PacketDirection::Telecommand));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TC secondary-header size mismatch.");
  RET_IF_ERR_MSG((data[0] >> 4U) != 2U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TC version is invalid.");
  const auto result = parseTcBody(data, 1U);
  if (!result) return result.error();
  m_acknowledgementFlags = data[0] & 0x0FU;
  return true;
}

std::vector<std::uint8_t> CCSDS::PusCTcHeader::serialize() const {
  if (!profileIsValid() || m_profile.pusRevision != PusRevision::C
      || m_profile.direction != PacketDirection::Telecommand
      || m_acknowledgementFlags > 0x0FU
      || !identifierFits(m_sourceId, m_profile.sourceIdOctets)) return {};
  std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(0x20U | m_acknowledgementFlags)};
  appendTcBody(bytes);
  return bytes;
}

CCSDS::PusATmHeader::PusATmHeader(MissionProfile profile, const std::uint8_t serviceType,
                                  const std::uint8_t serviceSubtype,
                                  const std::uint8_t packetSubcounter,
                                  const std::uint32_t destinationId,
                                  std::vector<std::uint8_t> timestamp)
  : PusTmSecondaryHeader(std::move(profile), serviceType, serviceSubtype,
                         destinationId, std::move(timestamp)),
    m_packetSubcounter(packetSubcounter) {}

std::uint16_t CCSDS::PusATmHeader::getSize() const {
  return static_cast<std::uint16_t>(3U
    + (m_profile.pusATmPacketSubcounterPresent ? 1U : 0U) + tmTailSize());
}

CCSDS::ResultBool CCSDS::PusATmHeader::deserialize(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateHeaderProfile(m_profile, PusRevision::A, PacketDirection::Telemetry));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TM secondary-header size mismatch.");
  RET_IF_ERR_MSG(data[0] != 0x10U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TM reserved bits or version are invalid.");
  const auto subcounterOffset = 3U;
  const auto tailOffset = subcounterOffset + (m_profile.pusATmPacketSubcounterPresent ? 1U : 0U);
  const auto tailResult = parseTmTail(data, tailOffset);
  if (!tailResult) return tailResult.error();
  m_serviceType = data[1];
  m_serviceSubtype = data[2];
  m_packetSubcounter = m_profile.pusATmPacketSubcounterPresent ? data[subcounterOffset] : 0U;
  return true;
}

std::vector<std::uint8_t> CCSDS::PusATmHeader::serialize() const {
  if (!profileIsValid() || m_profile.pusRevision != PusRevision::A
      || m_profile.direction != PacketDirection::Telemetry
      || !identifierFits(m_destinationId, m_profile.destinationIdOctets)
      || m_timestamp.size() != m_profile.telemetryTimeCodeOctets) return {};
  std::vector<std::uint8_t> bytes{0x10U, m_serviceType, m_serviceSubtype};
  if (m_profile.pusATmPacketSubcounterPresent) bytes.push_back(m_packetSubcounter);
  appendTmTail(bytes);
  return bytes;
}

CCSDS::PusCTmHeader::PusCTmHeader(MissionProfile profile, const std::uint8_t serviceType,
                                  const std::uint8_t serviceSubtype,
                                  const std::uint16_t messageTypeCounter,
                                  const std::uint32_t destinationId,
                                  const std::uint8_t timeReferenceStatus,
                                  std::vector<std::uint8_t> timestamp)
  : PusTmSecondaryHeader(std::move(profile), serviceType, serviceSubtype,
                         destinationId, std::move(timestamp)),
    m_messageTypeCounter(messageTypeCounter), m_timeReferenceStatus(timeReferenceStatus) {}

CCSDS::ResultBool CCSDS::PusCTmHeader::setTimeReferenceStatus(const std::uint8_t value) {
  RET_IF_ERR_MSG(value > 0x0FU, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C time-reference status exceeds four bits.");
  m_timeReferenceStatus = value;
  return true;
}

std::uint16_t CCSDS::PusCTmHeader::getSize() const {
  return static_cast<std::uint16_t>(5U + tmTailSize());
}

CCSDS::ResultBool CCSDS::PusCTmHeader::deserialize(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateHeaderProfile(m_profile, PusRevision::C, PacketDirection::Telemetry));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TM secondary-header size mismatch.");
  RET_IF_ERR_MSG((data[0] >> 4U) != 2U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TM version is invalid.");
  const auto tailResult = parseTmTail(data, 5U);
  if (!tailResult) return tailResult.error();
  m_timeReferenceStatus = data[0] & 0x0FU;
  m_serviceType = data[1];
  m_serviceSubtype = data[2];
  m_messageTypeCounter = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(data[3]) << 8U) | data[4]);
  return true;
}

std::vector<std::uint8_t> CCSDS::PusCTmHeader::serialize() const {
  if (!profileIsValid() || m_profile.pusRevision != PusRevision::C
      || m_profile.direction != PacketDirection::Telemetry
      || m_timeReferenceStatus > 0x0FU
      || !identifierFits(m_destinationId, m_profile.destinationIdOctets)
      || m_timestamp.size() != m_profile.telemetryTimeCodeOctets) return {};
  std::vector<std::uint8_t> bytes{
    static_cast<std::uint8_t>(0x20U | m_timeReferenceStatus),
    m_serviceType, m_serviceSubtype,
    static_cast<std::uint8_t>(m_messageTypeCounter >> 8U),
    static_cast<std::uint8_t>(m_messageTypeCounter & 0xFFU)
  };
  appendTmTail(bytes);
  return bytes;
}
