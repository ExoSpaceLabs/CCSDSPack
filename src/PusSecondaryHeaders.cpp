// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaders.h"
#include <utility>

namespace {
#ifndef CCSDS_MCU
  ccsds::Result<std::uint64_t> requiredUnsigned(const ccsds::Config &config,
                                                const char *key,
                                                const std::uint64_t maximum) {
    RET_IF_ERR_MSG(!config.isKey(key), ccsds::ErrorCode::CONFIG_FILE_ERROR,
                   std::string{"Config: Missing uint field: "} + key);
    const auto unsignedResult = config.get<std::uint64_t>(key);
    if (unsignedResult) {
      RET_IF_ERR_MSG(unsignedResult.value() > maximum,
                     ccsds::ErrorCode::CONFIG_FILE_ERROR,
                     std::string{"Config: Out-of-range uint field: "} + key);
      return unsignedResult.value();
    }
    const auto intResult = config.get<int>(key);
    RET_IF_ERR_MSG(!intResult || intResult.value() < 0
                   || static_cast<std::uint64_t>(intResult.value()) > maximum,
                   ccsds::ErrorCode::CONFIG_FILE_ERROR,
                   std::string{"Config: Invalid unsigned field: "} + key);
    return static_cast<std::uint64_t>(intResult.value());
  }

  ccsds::Result<std::uint64_t> optionalUnsigned(const ccsds::Config &config,
                                                const char *key,
                                                const std::uint64_t maximum,
                                                const std::uint64_t fallback) {
    if (!config.isKey(key)) return fallback;
    return requiredUnsigned(config, key, maximum);
  }
#endif
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::pus::SecondaryHeader::loadFromConfig(const ccsds::Config &config) {
  RET_IF_ERR_MSG(!config.isKey("secondary_header_type"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing string field: secondary_header_type");
  const auto type = config.get<std::string>("secondary_header_type");
  RET_IF_ERR_MSG(!type || type.value() != getType(), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: secondary_header_type does not match the concrete PUS header.");

  std::uint64_t spare{};
  ASSIGN_CP(spare, optionalUnsigned(config, "secondary_header_spare_octets", UINT8_MAX,
                                   m_secondaryHeaderSpareOctets));
  m_secondaryHeaderSpareOctets = static_cast<std::uint8_t>(spare);
  return true;
}
#endif

bool ccsds::pus::SecondaryHeader::identifierFits(const std::uint32_t value,
                                                  const std::uint8_t octets) const {
  if (octets == 0U) return value == 0U;
  if (octets >= 4U) return true;
  return value < (1UL << (octets * 8U));
}

void ccsds::pus::SecondaryHeader::appendIdentifier(std::vector<std::uint8_t> &bytes,
                                                    const std::uint32_t value,
                                                    const std::uint8_t octets) {
  for (std::uint8_t i = 0; i < octets; ++i) {
    const auto shift = static_cast<std::uint8_t>((octets - i - 1U) * 8U);
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
  }
}

std::uint32_t ccsds::pus::SecondaryHeader::readIdentifier(
    const std::vector<std::uint8_t> &bytes, const std::size_t offset,
    const std::uint8_t octets) {
  std::uint32_t result = 0U;
  for (std::uint8_t i = 0; i < octets; ++i) {
    result = static_cast<std::uint32_t>((result << 8U) | bytes[offset + i]);
  }
  return result;
}

bool ccsds::pus::SecondaryHeader::trailingSpareIsZero(
    const std::vector<std::uint8_t> &bytes) const {
  const auto spare = static_cast<std::size_t>(m_secondaryHeaderSpareOctets);
  if (spare > bytes.size()) return false;
  for (std::size_t i = bytes.size() - spare; i < bytes.size(); ++i) {
    if (bytes[i] != 0U) return false;
  }
  return true;
}

ccsds::pus::TcSecondaryHeader::TcSecondaryHeader(
    const std::uint8_t sourceIdOctets, const std::uint8_t spareOctets,
    const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
    const std::uint32_t sourceId, const std::uint8_t acknowledgementFlags)
  : SecondaryHeader(spareOctets), m_sourceIdOctets(sourceIdOctets),
    m_acknowledgementFlags(acknowledgementFlags), m_serviceType(serviceType),
    m_serviceSubtype(serviceSubtype), m_sourceId(sourceId) {}

ccsds::ResultBool ccsds::pus::TcSecondaryHeader::setAcknowledgementFlags(
    const std::uint8_t flags) {
  RET_IF_ERR_MSG(flags > 0x0FU, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS acknowledgement flags exceed four bits.");
  m_acknowledgementFlags = flags;
  return true;
}

ccsds::ResultBool ccsds::pus::TcSecondaryHeader::setSourceId(const std::uint32_t value) {
  RET_IF_ERR_MSG(!identifierFits(value, m_sourceIdOctets),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS source ID does not fit the configured width.");
  m_sourceId = value;
  return true;
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::pus::TcSecondaryHeader::loadFromConfig(
    const ccsds::Config &config) {
  FORWARD_RESULT(SecondaryHeader::loadFromConfig(config));

  if (getRevision() == Revision::A) {
    std::uint64_t width{};
    ASSIGN_CP(width, optionalUnsigned(config, "pus_source_id_octets", 4U, m_sourceIdOctets));
    RET_IF_ERR_MSG(!validIdentifierWidth(static_cast<std::uint8_t>(width)),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: pus_source_id_octets must be 0, 1, 2, or 4.");
    m_sourceIdOctets = static_cast<std::uint8_t>(width);
  } else if (config.isKey("pus_source_id_octets")) {
    std::uint64_t width{};
    ASSIGN_CP(width, requiredUnsigned(config, "pus_source_id_octets", 4U));
    RET_IF_ERR_MSG(width != 2U, ErrorCode::CONFIG_FILE_ERROR,
                   "Config: PUS-C TC source ID is fixed at two octets.");
  }

  std::uint64_t serviceType{};
  std::uint64_t serviceSubtype{};
  std::uint64_t acknowledgementFlags{};
  std::uint64_t sourceId{};
  ASSIGN_CP(serviceType, requiredUnsigned(config, "pus_service_type", UINT8_MAX));
  ASSIGN_CP(serviceSubtype, requiredUnsigned(config, "pus_service_subtype", UINT8_MAX));
  ASSIGN_CP(acknowledgementFlags,
            requiredUnsigned(config, "pus_acknowledgement_flags", 0x0FU));
  ASSIGN_CP(sourceId, requiredUnsigned(config, "pus_source_id", UINT32_MAX));
  m_serviceType = static_cast<std::uint8_t>(serviceType);
  m_serviceSubtype = static_cast<std::uint8_t>(serviceSubtype);
  FORWARD_RESULT(setAcknowledgementFlags(static_cast<std::uint8_t>(acknowledgementFlags)));
  FORWARD_RESULT(setSourceId(static_cast<std::uint32_t>(sourceId)));
  return true;
}
#endif

std::uint16_t ccsds::pus::TcSecondaryHeader::tcSize() const {
  return static_cast<std::uint16_t>(3U + m_sourceIdOctets
                                    + m_secondaryHeaderSpareOctets);
}

void ccsds::pus::TcSecondaryHeader::appendTcBody(std::vector<std::uint8_t> &bytes) const {
  bytes.push_back(m_serviceType);
  bytes.push_back(m_serviceSubtype);
  appendIdentifier(bytes, m_sourceId, m_sourceIdOctets);
  bytes.insert(bytes.end(), m_secondaryHeaderSpareOctets, 0U);
}

ccsds::ResultBool ccsds::pus::TcSecondaryHeader::parseTcBody(
    const std::vector<std::uint8_t> &data, const std::size_t offset) {
  RET_IF_ERR_MSG(data.size() != tcSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TC secondary-header size does not match its tailoring.");
  RET_IF_ERR_MSG(!trailingSpareIsZero(data), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TC secondary-header spare octets must be zero.");
  const auto source = readIdentifier(data, offset + 2U, m_sourceIdOctets);
  m_serviceType = data[offset];
  m_serviceSubtype = data[offset + 1U];
  m_sourceId = source;
  return true;
}

ccsds::pus::TmSecondaryHeader::TmSecondaryHeader(
    const std::uint8_t destinationIdOctets, const bool timestampPresent,
    time::CucConfiguration cuc, const std::uint8_t spareOctets,
    const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
    const std::uint32_t destinationId, time::CucTime timestamp)
  : SecondaryHeader(spareOctets), m_destinationIdOctets(destinationIdOctets),
    m_timestampPresent(timestampPresent), m_cuc(std::move(cuc)),
    m_serviceType(serviceType), m_serviceSubtype(serviceSubtype),
    m_destinationId(destinationId), m_timestamp(std::move(timestamp)) {}

ccsds::ResultBool ccsds::pus::TmSecondaryHeader::setDestinationId(const std::uint32_t value) {
  RET_IF_ERR_MSG(!identifierFits(value, m_destinationIdOctets),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS destination ID does not fit the configured width.");
  m_destinationId = value;
  return true;
}

ccsds::ResultBool ccsds::pus::TmSecondaryHeader::setTimestamp(
    const time::CucTime &timestamp) {
  RET_IF_ERR_MSG(!m_timestampPresent,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TM timestamp is disabled by the active tailoring.");
  const auto encoded = time::serialize(timestamp, m_cuc);
  if (!encoded) {
    return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Invalid PUS TM CUC timestamp: " + encoded.error().message()};
  }
  m_timestamp = timestamp;
  return true;
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::pus::TmSecondaryHeader::loadFromConfig(
    const ccsds::Config &config) {
  FORWARD_RESULT(SecondaryHeader::loadFromConfig(config));

  if (getRevision() == Revision::A) {
    std::uint64_t width{};
    ASSIGN_CP(width, optionalUnsigned(config, "pus_destination_id_octets", 4U,
                                     m_destinationIdOctets));
    RET_IF_ERR_MSG(!validIdentifierWidth(static_cast<std::uint8_t>(width)),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: pus_destination_id_octets must be 0, 1, 2, or 4.");
    m_destinationIdOctets = static_cast<std::uint8_t>(width);
  } else if (config.isKey("pus_destination_id_octets")) {
    std::uint64_t width{};
    ASSIGN_CP(width, requiredUnsigned(config, "pus_destination_id_octets", 4U));
    RET_IF_ERR_MSG(width != 2U, ErrorCode::CONFIG_FILE_ERROR,
                   "Config: PUS-C TM destination ID is fixed at two octets.");
  }

  if (config.isKey("pus_time_format")) {
    const auto format = config.get<std::string>("pus_time_format");
    RET_IF_ERR_MSG(!format, ErrorCode::CONFIG_FILE_ERROR,
                   "Config: invalid pus_time_format.");
    if (format.value() == "none") {
      m_timestampPresent = false;
      m_cuc = {};
      RET_IF_ERR_MSG(config.isKey("pus_time_epoch") || config.isKey("pus_time_p_field")
                     || config.isKey("pus_time_coarse_octets")
                     || config.isKey("pus_time_fine_octets")
                     || config.isKey("pus_time_coarse") || config.isKey("pus_time_fine"),
                     ErrorCode::CONFIG_FILE_ERROR,
                     "Config: disabled PUS time cannot contain CUC fields.");
    } else if (format.value() == "cuc") {
      m_timestampPresent = true;
      RET_IF_ERR_MSG(!config.isKey("pus_time_epoch") || !config.isKey("pus_time_p_field")
                     || !config.isKey("pus_time_coarse_octets")
                     || !config.isKey("pus_time_fine_octets"),
                     ErrorCode::CONFIG_FILE_ERROR,
                     "Config: CUC tailoring requires epoch, P-field, coarse, and fine width fields.");
      const auto epoch = config.get<std::string>("pus_time_epoch");
      const auto pField = config.get<std::string>("pus_time_p_field");
      RET_IF_ERR_MSG(!epoch || !pField, ErrorCode::CONFIG_FILE_ERROR,
                     "Config: invalid PUS CUC string field.");
      m_cuc.epoch = epoch.value() == "ccsds-1958-tai" ? time::Epoch::Ccsds1958Tai
                    : epoch.value() == "agency-defined" ? time::Epoch::AgencyDefined
                    : time::Epoch::Unspecified;
      m_cuc.pField = pField.value() == "implicit" ? time::PFieldMode::Implicit
                     : pField.value() == "explicit" ? time::PFieldMode::Explicit
                     : static_cast<time::PFieldMode>(0xFFU);
      std::uint64_t coarse{};
      std::uint64_t fine{};
      ASSIGN_CP(coarse, requiredUnsigned(config, "pus_time_coarse_octets", 4U));
      ASSIGN_CP(fine, requiredUnsigned(config, "pus_time_fine_octets", 3U));
      RET_IF_ERR_MSG(coarse < 1U, ErrorCode::CONFIG_FILE_ERROR,
                     "Config: pus_time_coarse_octets must be 1..4.");
      m_cuc.coarseOctets = static_cast<std::uint8_t>(coarse);
      m_cuc.fineOctets = static_cast<std::uint8_t>(fine);
      FORWARD_RESULT(time::validate(m_cuc));
    } else {
      return Error{ErrorCode::CONFIG_FILE_ERROR,
                   "Config: pus_time_format must be 'none' or 'cuc'."};
    }
  }

  std::uint64_t serviceType{};
  std::uint64_t serviceSubtype{};
  std::uint64_t destinationId{};
  ASSIGN_CP(serviceType, requiredUnsigned(config, "pus_service_type", UINT8_MAX));
  ASSIGN_CP(serviceSubtype, requiredUnsigned(config, "pus_service_subtype", UINT8_MAX));
  ASSIGN_CP(destinationId, requiredUnsigned(config, "pus_destination_id", UINT32_MAX));
  m_serviceType = static_cast<std::uint8_t>(serviceType);
  m_serviceSubtype = static_cast<std::uint8_t>(serviceSubtype);
  FORWARD_RESULT(setDestinationId(static_cast<std::uint32_t>(destinationId)));

  if (m_timestampPresent) {
    time::CucTime timestamp;
    ASSIGN_CP(timestamp.coarse, requiredUnsigned(config, "pus_time_coarse", UINT32_MAX));
    ASSIGN_CP(timestamp.fine, requiredUnsigned(config, "pus_time_fine", 0xFFFFFFU));
    FORWARD_RESULT(setTimestamp(timestamp));
  } else {
    m_timestamp = {};
  }
  return true;
}
#endif

ccsds::ResultBool ccsds::pus::TmSecondaryHeader::appendTmTail(
    std::vector<std::uint8_t> &bytes) const {
  appendIdentifier(bytes, m_destinationId, m_destinationIdOctets);
  if (m_timestampPresent) {
    std::vector<std::uint8_t> encoded;
    ASSIGN_MV(encoded, time::serialize(m_timestamp, m_cuc));
    bytes.insert(bytes.end(), encoded.begin(), encoded.end());
  }
  bytes.insert(bytes.end(), m_secondaryHeaderSpareOctets, 0U);
  return true;
}

ccsds::ResultBool ccsds::pus::TmSecondaryHeader::parseTmTail(
    const std::vector<std::uint8_t> &data, const std::size_t offset) {
  const auto tail = tmTailSize();
  RET_IF_ERR_MSG(offset + tail != data.size(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TM secondary-header size does not match its tailoring.");
  RET_IF_ERR_MSG(!trailingSpareIsZero(data), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS TM secondary-header spare octets must be zero.");
  const auto destination = readIdentifier(data, offset, m_destinationIdOctets);
  const auto timestampOffset = offset + m_destinationIdOctets;
  const auto timestampSize = m_timestampPresent ? time::encodedSize(m_cuc) : 0U;
  const auto timestampEnd = timestampOffset + timestampSize;
  m_destinationId = destination;
  if (m_timestampPresent) {
    const std::vector<std::uint8_t> encoded(
      data.begin() + static_cast<std::ptrdiff_t>(timestampOffset),
      data.begin() + static_cast<std::ptrdiff_t>(timestampEnd));
    ASSIGN_CP(m_timestamp, time::deserialize(encoded, m_cuc));
  } else {
    m_timestamp = {};
  }
  return true;
}

std::uint16_t ccsds::pus::TmSecondaryHeader::tmTailSize() const {
  return static_cast<std::uint16_t>(m_destinationIdOctets
                                    + (m_timestampPresent ? time::encodedSize(m_cuc) : 0U)
                                    + m_secondaryHeaderSpareOctets);
}

ccsds::pus::rev_a::TcHeader::TcHeader()
  : TcHeader(TcTailoring{}) {}

ccsds::pus::rev_a::TcHeader::TcHeader(
    const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
    const std::uint32_t sourceId, const std::uint8_t acknowledgementFlags)
  : TcHeader(TcTailoring{}, serviceType, serviceSubtype, sourceId,
             acknowledgementFlags) {}

ccsds::pus::rev_a::TcHeader::TcHeader(
    TcTailoring tailoring, const std::uint8_t serviceType,
    const std::uint8_t serviceSubtype, const std::uint32_t sourceId,
    const std::uint8_t acknowledgementFlags)
  : TcSecondaryHeader(tailoring.sourceIdOctets, tailoring.secondaryHeaderSpareOctets,
                      serviceType, serviceSubtype, sourceId, acknowledgementFlags) {}

ccsds::ResultBool ccsds::pus::rev_a::TcHeader::deserialize(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateTailoring(getTailoring()));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TC secondary-header size mismatch.");
  RET_IF_ERR_MSG((data[0] & 0x80U) != 0U || ((data[0] >> 4U) & 0x07U) != 1U,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TC reserved bit or version is invalid.");
  FORWARD_RESULT(parseTcBody(data, 1U));
  m_acknowledgementFlags = data[0] & 0x0FU;
  return true;
}

std::vector<std::uint8_t> ccsds::pus::rev_a::TcHeader::serialize() const {
  if (!validateTailoring(getTailoring()) || m_acknowledgementFlags > 0x0FU
      || !identifierFits(m_sourceId, m_sourceIdOctets)) return {};
  std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(0x10U | m_acknowledgementFlags)};
  appendTcBody(bytes);
  return bytes;
}

ccsds::pus::rev_a::TmHeader::TmHeader()
  : TmHeader(TmTailoring{}) {}

ccsds::pus::rev_a::TmHeader::TmHeader(
    const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
    const std::uint8_t packetSubcounter, const std::uint32_t destinationId,
    time::CucTime timestamp)
  : TmHeader(TmTailoring{}, serviceType, serviceSubtype, packetSubcounter,
             destinationId, std::move(timestamp)) {}

ccsds::pus::rev_a::TmHeader::TmHeader(
    TmTailoring tailoring, const std::uint8_t serviceType,
    const std::uint8_t serviceSubtype, const std::uint8_t packetSubcounter,
    const std::uint32_t destinationId, time::CucTime timestamp)
  : TmSecondaryHeader(tailoring.destinationIdOctets, tailoring.timestampPresent,
                      tailoring.cuc, tailoring.secondaryHeaderSpareOctets,
                      serviceType, serviceSubtype, destinationId, std::move(timestamp)),
    m_packetSubcounterPresent(tailoring.packetSubcounterPresent),
    m_packetSubcounter(packetSubcounter) {}

std::uint16_t ccsds::pus::rev_a::TmHeader::getSize() const {
  return static_cast<std::uint16_t>(3U + (m_packetSubcounterPresent ? 1U : 0U)
                                    + tmTailSize());
}

ccsds::ResultBool ccsds::pus::rev_a::TmHeader::deserialize(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateTailoring(getTailoring()));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TM secondary-header size mismatch.");
  RET_IF_ERR_MSG(data[0] != 0x10U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-A TM reserved bits or version are invalid.");
  const auto subcounterOffset = 3U;
  const auto tailOffset = subcounterOffset + (m_packetSubcounterPresent ? 1U : 0U);
  FORWARD_RESULT(parseTmTail(data, tailOffset));
  m_serviceType = data[1];
  m_serviceSubtype = data[2];
  m_packetSubcounter = m_packetSubcounterPresent ? data[subcounterOffset] : 0U;
  return true;
}

std::vector<std::uint8_t> ccsds::pus::rev_a::TmHeader::serialize() const {
  if (!validateTailoring(getTailoring())
      || !identifierFits(m_destinationId, m_destinationIdOctets)) return {};
  std::vector<std::uint8_t> bytes{0x10U, m_serviceType, m_serviceSubtype};
  if (m_packetSubcounterPresent) bytes.push_back(m_packetSubcounter);
  if (!appendTmTail(bytes)) return {};
  return bytes;
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::pus::rev_a::TmHeader::loadFromConfig(
    const ccsds::Config &config) {
  FORWARD_RESULT(TmSecondaryHeader::loadFromConfig(config));
  if (config.isKey("pus_a_tm_packet_subcounter_present")) {
    const auto enabled = config.get<bool>("pus_a_tm_packet_subcounter_present");
    RET_IF_ERR_MSG(!enabled, ErrorCode::CONFIG_FILE_ERROR,
                   "Config: pus_a_tm_packet_subcounter_present must be bool.");
    m_packetSubcounterPresent = enabled.value();
  }
  if (m_packetSubcounterPresent) {
    std::uint64_t subcounter{};
    ASSIGN_CP(subcounter, requiredUnsigned(config, "pus_packet_subcounter", UINT8_MAX));
    m_packetSubcounter = static_cast<std::uint8_t>(subcounter);
  } else {
    RET_IF_ERR_MSG(config.isKey("pus_packet_subcounter"), ErrorCode::CONFIG_FILE_ERROR,
                   "Config: disabled PUS-A packet subcounter cannot have a value.");
    m_packetSubcounter = 0U;
  }
  FORWARD_RESULT(validateTailoring(getTailoring()));
  return true;
}
#endif

ccsds::pus::rev_c::TcHeader::TcHeader()
  : TcHeader(TcTailoring{}) {}

ccsds::pus::rev_c::TcHeader::TcHeader(
    const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
    const std::uint32_t sourceId, const std::uint8_t acknowledgementFlags)
  : TcHeader(TcTailoring{}, serviceType, serviceSubtype, sourceId,
             acknowledgementFlags) {}

ccsds::pus::rev_c::TcHeader::TcHeader(
    TcTailoring tailoring, const std::uint8_t serviceType,
    const std::uint8_t serviceSubtype, const std::uint32_t sourceId,
    const std::uint8_t acknowledgementFlags)
  : TcSecondaryHeader(2U, tailoring.secondaryHeaderSpareOctets, serviceType,
                      serviceSubtype, sourceId, acknowledgementFlags) {}

ccsds::ResultBool ccsds::pus::rev_c::TcHeader::deserialize(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateTailoring(getTailoring()));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TC secondary-header size mismatch.");
  RET_IF_ERR_MSG((data[0] >> 4U) != 2U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TC version is invalid.");
  FORWARD_RESULT(parseTcBody(data, 1U));
  m_acknowledgementFlags = data[0] & 0x0FU;
  return true;
}

std::vector<std::uint8_t> ccsds::pus::rev_c::TcHeader::serialize() const {
  if (!validateTailoring(getTailoring()) || m_acknowledgementFlags > 0x0FU
      || !identifierFits(m_sourceId, 2U)) return {};
  std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(0x20U | m_acknowledgementFlags)};
  appendTcBody(bytes);
  return bytes;
}

ccsds::pus::rev_c::TmHeader::TmHeader()
  : TmHeader(TmTailoring{}) {}

ccsds::pus::rev_c::TmHeader::TmHeader(
    const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
    const std::uint16_t messageTypeCounter, const std::uint32_t destinationId,
    const std::uint8_t timeReferenceStatus, time::CucTime timestamp)
  : TmHeader(TmTailoring{}, serviceType, serviceSubtype, messageTypeCounter,
             destinationId, timeReferenceStatus, std::move(timestamp)) {}

ccsds::pus::rev_c::TmHeader::TmHeader(
    TmTailoring tailoring, const std::uint8_t serviceType,
    const std::uint8_t serviceSubtype, const std::uint16_t messageTypeCounter,
    const std::uint32_t destinationId, const std::uint8_t timeReferenceStatus,
    time::CucTime timestamp)
  : TmSecondaryHeader(2U, tailoring.timestampPresent, tailoring.cuc,
                      tailoring.secondaryHeaderSpareOctets, serviceType,
                      serviceSubtype, destinationId, std::move(timestamp)),
    m_messageTypeCounter(messageTypeCounter), m_timeReferenceStatus(timeReferenceStatus) {}

ccsds::ResultBool ccsds::pus::rev_c::TmHeader::setTimeReferenceStatus(
    const std::uint8_t value) {
  RET_IF_ERR_MSG(value > 0x0FU, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C time-reference status exceeds four bits.");
  m_timeReferenceStatus = value;
  return true;
}

std::uint16_t ccsds::pus::rev_c::TmHeader::getSize() const {
  return static_cast<std::uint16_t>(5U + tmTailSize());
}

ccsds::ResultBool ccsds::pus::rev_c::TmHeader::deserialize(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(validateTailoring(getTailoring()));
  RET_IF_ERR_MSG(data.size() != getSize(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TM secondary-header size mismatch.");
  RET_IF_ERR_MSG((data[0] >> 4U) != 2U, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS-C TM version is invalid.");
  FORWARD_RESULT(parseTmTail(data, 5U));
  m_timeReferenceStatus = data[0] & 0x0FU;
  m_serviceType = data[1];
  m_serviceSubtype = data[2];
  m_messageTypeCounter = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(data[3]) << 8U) | data[4]);
  return true;
}

std::vector<std::uint8_t> ccsds::pus::rev_c::TmHeader::serialize() const {
  if (!validateTailoring(getTailoring()) || m_timeReferenceStatus > 0x0FU
      || !identifierFits(m_destinationId, 2U)) return {};
  std::vector<std::uint8_t> bytes{
    static_cast<std::uint8_t>(0x20U | m_timeReferenceStatus),
    m_serviceType, m_serviceSubtype,
    static_cast<std::uint8_t>(m_messageTypeCounter >> 8U),
    static_cast<std::uint8_t>(m_messageTypeCounter & 0xFFU)
  };
  if (!appendTmTail(bytes)) return {};
  return bytes;
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::pus::rev_c::TmHeader::loadFromConfig(
    const ccsds::Config &config) {
  FORWARD_RESULT(TmSecondaryHeader::loadFromConfig(config));
  std::uint64_t messageTypeCounter{};
  std::uint64_t timeReferenceStatus{};
  ASSIGN_CP(messageTypeCounter,
            requiredUnsigned(config, "pus_message_type_counter", UINT16_MAX));
  ASSIGN_CP(timeReferenceStatus,
            requiredUnsigned(config, "pus_time_reference_status", 0x0FU));
  m_messageTypeCounter = static_cast<std::uint16_t>(messageTypeCounter);
  FORWARD_RESULT(setTimeReferenceStatus(static_cast<std::uint8_t>(timeReferenceStatus)));
  FORWARD_RESULT(validateTailoring(getTailoring()));
  return true;
}
#endif
