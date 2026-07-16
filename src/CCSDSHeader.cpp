// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSHeader.h"

void CCSDS::Header::refreshStatus() {
  m_status = m_APID == IDLE_APID ? IDLE : NORMAL;
}

CCSDS::ResultBool CCSDS::Header::setVersionNumber(const std::uint8_t &value) {
  if (value != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid CCSDS Space Packet Version Number: expected encoded value 0");
  }
  m_versionNumber = value;
  refreshStatus();
  return true;
}

CCSDS::ResultBool CCSDS::Header::setType(const std::uint8_t &value) {
  if (value > 0x01U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid type, value > 1");
  }
  m_type = value;
  refreshStatus();
  return true;
}

CCSDS::ResultBool CCSDS::Header::setDataFieldHeaderFlag(const std::uint8_t &value) {
  if (value > 0x01U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid data field header flag, value > 1");
  }
  if (m_APID == IDLE_APID && value != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid Idle Packet: the Secondary Header Flag must be zero");
  }
  m_dataFieldHeaderFlag = value;
  refreshStatus();
  return true;
}

CCSDS::ResultBool CCSDS::Header::setAPID(const std::uint16_t &value) {
  if (value > IDLE_APID) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid APID, value > 2047");
  }
  if (value == IDLE_APID && m_dataFieldHeaderFlag != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid Idle Packet: APID 2047 requires Secondary Header Flag zero");
  }
  m_APID = value;
  refreshStatus();
  return true;
}

CCSDS::ResultBool CCSDS::Header::setSequenceFlags(const std::uint8_t &value) {
  if (value > 0x03U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid sequence flag, value > 3");
  }
  m_sequenceFlags = value;
  refreshStatus();
  return true;
}

CCSDS::ResultBool CCSDS::Header::setSequenceCount(const std::uint16_t &value) {
  if (value > 0x3FFFU) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid sequence count, value > 16383");
  }
  m_sequenceCount = value;
  refreshStatus();
  return true;
}

void CCSDS::Header::setDataLength(const std::uint16_t &value) {
  m_dataLength = value;
}

CCSDS::ResultBool CCSDS::Header::deserialize(const std::vector<std::uint8_t> &data) {
  if (data.size() != 6U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid Header Data provided: size != 6");
  }

  std::uint64_t headerData = 0;
  for (std::uint8_t i = 0; i < 6; ++i) {
    headerData |= static_cast<std::uint64_t>(data[i]) << (40 - i * 8);
  }

  m_dataLength = static_cast<std::uint16_t>(headerData & 0xFFFFU);
  m_packetSequenceControl = static_cast<std::uint16_t>((headerData >> 16) & 0xFFFFU);
  m_packetIdentificationAndVersion = static_cast<std::uint16_t>((headerData >> 32) & 0xFFFFU);
  m_versionNumber = static_cast<std::uint8_t>(m_packetIdentificationAndVersion >> 13);
  m_type = static_cast<std::uint8_t>((m_packetIdentificationAndVersion >> 12) & 0x01U);
  m_dataFieldHeaderFlag =
    static_cast<std::uint8_t>((m_packetIdentificationAndVersion >> 11) & 0x01U);
  m_APID = static_cast<std::uint16_t>(m_packetIdentificationAndVersion & 0x07FFU);
  m_sequenceFlags = static_cast<std::uint8_t>(m_packetSequenceControl >> 14);
  m_sequenceCount = static_cast<std::uint16_t>(m_packetSequenceControl & 0x3FFFU);
  refreshStatus();
  return true;
}

CCSDS::ResultBool CCSDS::Header::setData(const std::uint64_t &data) {
  if (data > 0xFFFFFFFFFFFFULL) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Input data exceeds expected bit size for version or size.");
  }

  const auto dataLength = static_cast<std::uint16_t>(data & 0xFFFFU);
  const auto packetSequenceControl = static_cast<std::uint16_t>((data >> 16) & 0xFFFFU);
  const auto packetIdentificationAndVersion = static_cast<std::uint16_t>((data >> 32) & 0xFFFFU);
  const auto versionNumber = static_cast<std::uint8_t>(packetIdentificationAndVersion >> 13);
  const auto type = static_cast<std::uint8_t>((packetIdentificationAndVersion >> 12) & 0x01U);
  const auto dataFieldHeaderFlag =
    static_cast<std::uint8_t>((packetIdentificationAndVersion >> 11) & 0x01U);
  const auto APID = static_cast<std::uint16_t>(packetIdentificationAndVersion & 0x07FFU);
  const auto sequenceFlags = static_cast<std::uint8_t>(packetSequenceControl >> 14);
  const auto sequenceCount = static_cast<std::uint16_t>(packetSequenceControl & 0x3FFFU);

  if (versionNumber != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid CCSDS Space Packet Version Number: expected encoded value 0");
  }
  if (APID == IDLE_APID && dataFieldHeaderFlag != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid Idle Packet: APID 2047 requires Secondary Header Flag zero");
  }

  m_dataLength = dataLength;
  m_packetSequenceControl = packetSequenceControl;
  m_packetIdentificationAndVersion = packetIdentificationAndVersion;
  m_versionNumber = versionNumber;
  m_type = type;
  m_dataFieldHeaderFlag = dataFieldHeaderFlag;
  m_APID = APID;
  m_sequenceFlags = sequenceFlags;
  m_sequenceCount = sequenceCount;
  refreshStatus();
  return true;
}

std::vector<std::uint8_t> CCSDS::Header::serialize() {
  return static_cast<const Header &>(*this).serialize();
}

std::vector<std::uint8_t> CCSDS::Header::serialize() const {
  if (m_status == INVALID || m_versionNumber != 0U
      || (m_APID == IDLE_APID && m_dataFieldHeaderFlag != 0U)) {
    return {};
  }

  const auto packetSequenceControl = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_sequenceFlags) << 14U) | m_sequenceCount);
  const auto packetIdentificationAndVersion = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_versionNumber) << 13U)
    | (static_cast<std::uint16_t>(m_type) << 12U)
    | (static_cast<std::uint16_t>(m_dataFieldHeaderFlag) << 11U)
    | m_APID);

  return {
    static_cast<std::uint8_t>(packetIdentificationAndVersion >> 8U),
    static_cast<std::uint8_t>(packetIdentificationAndVersion & 0xFFU),
    static_cast<std::uint8_t>(packetSequenceControl >> 8U),
    static_cast<std::uint8_t>(packetSequenceControl & 0xFFU),
    static_cast<std::uint8_t>(m_dataLength >> 8U),
    static_cast<std::uint8_t>(m_dataLength & 0xFFU),
  };
}

std::uint64_t CCSDS::Header::getFullHeader() {
  return static_cast<const Header &>(*this).getFullHeader();
}

std::uint64_t CCSDS::Header::getFullHeader() const {
  if (m_status == INVALID || m_versionNumber != 0U
      || (m_APID == IDLE_APID && m_dataFieldHeaderFlag != 0U)) {
    return 0U;
  }
  const auto packetSequenceControl = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_sequenceFlags) << 14U) | m_sequenceCount);
  const auto packetIdentificationAndVersion = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_versionNumber) << 13U)
    | (static_cast<std::uint16_t>(m_type) << 12U)
    | (static_cast<std::uint16_t>(m_dataFieldHeaderFlag) << 11U)
    | m_APID);
  return (static_cast<std::uint64_t>(packetIdentificationAndVersion) << 32U)
         | (static_cast<std::uint32_t>(packetSequenceControl) << 16U)
         | m_dataLength;
}

CCSDS::ResultBool CCSDS::Header::setData(const PrimaryHeader &data) {
  if (data.versionNumber != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid CCSDS Space Packet Version Number: expected encoded value 0");
  }
  if (data.type > 0x01U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid type, value > 1");
  }
  if (data.dataFieldHeaderFlag > 0x01U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid data field header flag, value > 1");
  }
  if (data.APID > IDLE_APID) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid APID, value > 2047");
  }
  if (data.APID == IDLE_APID && data.dataFieldHeaderFlag != 0U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA,
                 "Invalid Idle Packet: APID 2047 requires Secondary Header Flag zero");
  }
  if (data.sequenceFlags > 0x03U) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid sequence flag, value > 3");
  }
  if (data.sequenceCount > 0x3FFFU) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid sequence count, value > 16383");
  }

  m_versionNumber = data.versionNumber;
  m_type = data.type;
  m_dataFieldHeaderFlag = data.dataFieldHeaderFlag;
  m_APID = data.APID;
  m_sequenceFlags = data.sequenceFlags;
  m_sequenceCount = data.sequenceCount;
  m_dataLength = data.dataLength;
  m_packetSequenceControl = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_sequenceFlags) << 14U) | m_sequenceCount);
  m_packetIdentificationAndVersion = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_versionNumber) << 13U)
    | (static_cast<std::uint16_t>(m_type) << 12U)
    | (static_cast<std::uint16_t>(m_dataFieldHeaderFlag) << 11U)
    | m_APID);
  refreshStatus();
  return true;
}
