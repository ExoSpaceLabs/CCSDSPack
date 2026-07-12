// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSHeader.h"
#include "CCSDSUtils.h"

CCSDS::ResultBool CCSDS::Header::setVersionNumber(const std::uint8_t &value) {
  if (value > 0x0007) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid version number, value > 7");
  }
  m_versionNumber = value;
  return true;
} ///< 3 bits

CCSDS::ResultBool CCSDS::Header::setType( const  std::uint8_t &value ) {
  if (value > 0x0001) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid type, value > 1");
  }
  m_type = value;
  return true;
} ///< 1 bits

CCSDS::ResultBool CCSDS::Header::setDataFieldHeaderFlag( const  std::uint8_t &value ) {
  if (value > 0x0001) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid data field header flag, value > 1");
  }
  m_dataFieldHeaderFlag = value;
  return true;
} ///< 1 bits

CCSDS::ResultBool CCSDS::Header::setAPID( const std::uint16_t &value ) {
  if (value > 0x07FF) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid APID, value > 2047");
  }
  if (value == 0x07FF) {
    m_status = IDLE;
  }
  m_APID = value;
  return true;
} ///< 11 bits

CCSDS::ResultBool CCSDS::Header::setSequenceFlags( const  std::uint8_t &value ) {
  if (value > 0x0003) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid sequence flag, value > 3");
  }
  m_sequenceFlags = value;
  return true;
} ///< 2 bits

CCSDS::ResultBool CCSDS::Header::setSequenceCount( const std::uint16_t &value ) {
  if (value > 0x3FFF) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid  sequence count, value > 16383" );
  }
  m_sequenceCount = value;
  return true;
} ///< 14 bits

void CCSDS::Header::setDataLength(const std::uint16_t &value) {
  m_dataLength = value;
} ///< 16 bits



CCSDS::ResultBool CCSDS::Header::deserialize(const std::vector<std::uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() != 6, ErrorCode::INVALID_HEADER_DATA, "Invalid Header Data provided: size != 6");

  std::uint64_t headerData = 0;
  for (std::uint8_t i = 0; i < 6; ++i) {
    headerData |= static_cast<std::uint64_t>(data[i]) << (40 - i * 8); // Combine MSB to LSB
  }
  FORWARD_RESULT(setData(headerData));
  return true;
}

// packet so it cannot fail explicit validation
CCSDS::ResultBool CCSDS::Header::setData(const std::uint64_t &data) {
  RET_IF_ERR_MSG(data > 0xFFFFFFFFFFFF, ErrorCode::INVALID_HEADER_DATA,
                 "Input data exceeds expected bit size for version or size.");
  // Decompose data using mask and shifts
  m_dataLength = (data & 0xFFFF); // last 16 bits
  m_packetSequenceControl = (data >> 16) & 0xFFFF; // middle 16 bits
  m_packetIdentificationAndVersion = (data >> 32); // first 16 bits

  // decompose packet identifier
  m_versionNumber = (m_packetIdentificationAndVersion >> 13); // First 3 bits for version
  m_type = (m_packetIdentificationAndVersion >> 12) & 0x1; // Next 1 bit
  m_dataFieldHeaderFlag = (m_packetIdentificationAndVersion >> 11) & 0x1; // Next 1 bit
  m_APID = (m_packetIdentificationAndVersion & 0x07FF); // Last 11 bits

  // decompose sequence control
  m_sequenceFlags = (m_packetSequenceControl >> 14); // first 2 bits
  m_sequenceCount = (m_packetSequenceControl & 0x3FFF); // Last 14 bits.
  return true;
}

std::vector<std::uint8_t> CCSDS::Header::serialize() {
  m_packetSequenceControl = (static_cast<std::uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
  m_packetIdentificationAndVersion = (static_cast<std::uint16_t>(m_versionNumber) << 13) | (m_type << 12) | static_cast<
                                       std::uint16_t>((m_dataFieldHeaderFlag) << 11) | m_APID;

  std::vector data{
    static_cast<unsigned char>(m_packetIdentificationAndVersion >> 8),
    static_cast<unsigned char>(m_packetIdentificationAndVersion & 0xFF),
    static_cast<unsigned char>(m_packetSequenceControl >> 8),
    static_cast<unsigned char>(m_packetSequenceControl & 0xFF),
    static_cast<unsigned char>(m_dataLength >> 8),
    static_cast<unsigned char>(m_dataLength & 0xFF),
  };
  return data;
}

CCSDS::ResultBool CCSDS::Header::setData(const PrimaryHeader &data) {
  FORWARD_RESULT(setVersionNumber(data.versionNumber));
  FORWARD_RESULT(setType(data.type));
  FORWARD_RESULT(setDataFieldHeaderFlag(data.dataFieldHeaderFlag));
  FORWARD_RESULT(setAPID(data.APID));
  FORWARD_RESULT(setSequenceFlags(data.sequenceFlags));
  FORWARD_RESULT(setSequenceCount(data.sequenceCount));
  setDataLength(data.dataLength);

  m_packetSequenceControl = (static_cast<std::uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
  m_packetIdentificationAndVersion = (static_cast<std::uint16_t>(m_versionNumber) << 13) | (m_type << 12) | static_cast<
                                       std::uint16_t>((m_dataFieldHeaderFlag) << 11) | m_APID;
  return true;
}
