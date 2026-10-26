// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSHeader.h"
#include "ccsdspack/c/primary_header.h"

void ccsds::Header::refreshStatus() {
  m_status = m_APID == IDLE_APID ? IDLE : NORMAL;
}

// Each checked setter asks the C11 core to validate the proposed field set.
// C++ retains the atomic object update and invalid-state recovery contract.
ccsds::ResultBool ccsds::Header::setVersionNumber(const std::uint8_t &value) {
  return setData(PrimaryHeader{value, m_type, m_secondaryHeaderFlag,
                               m_APID, m_sequenceFlags, m_sequenceCount, m_dataLength});
}

ccsds::ResultBool ccsds::Header::setType(const std::uint8_t &value) {
  return setData(PrimaryHeader{m_versionNumber, value, m_secondaryHeaderFlag,
                               m_APID, m_sequenceFlags, m_sequenceCount, m_dataLength});
}

ccsds::ResultBool ccsds::Header::setSecondaryHeaderFlag(const std::uint8_t &value) {
  return setData(PrimaryHeader{m_versionNumber, m_type, value,
                               m_APID, m_sequenceFlags, m_sequenceCount, m_dataLength});
}

ccsds::ResultBool ccsds::Header::setAPID(const std::uint16_t &value) {
  return setData(PrimaryHeader{m_versionNumber, m_type, m_secondaryHeaderFlag,
                               value, m_sequenceFlags, m_sequenceCount, m_dataLength});
}

ccsds::ResultBool ccsds::Header::setSequenceFlags(const std::uint8_t &value) {
  return setData(PrimaryHeader{m_versionNumber, m_type, m_secondaryHeaderFlag,
                               m_APID, value, m_sequenceCount, m_dataLength});
}

ccsds::ResultBool ccsds::Header::setSequenceCount(const std::uint16_t &value) {
  return setData(PrimaryHeader{m_versionNumber, m_type, m_secondaryHeaderFlag,
                               m_APID, m_sequenceFlags, value, m_dataLength});
}

void ccsds::Header::setDataLength(const std::uint16_t &value) {
  m_dataLength = value;
}

ccsds::ResultBool ccsds::Header::deserialize(const std::vector<std::uint8_t> &data) {
  if (data.size() != CCSDS_PRIMARY_HEADER_SIZE) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid Header Data provided: size != 6");
  }

  ccsds_primary_header_t decoded{};
  const auto status = ccsds_primary_header_decode(data.data(), data.size(), &decoded);
  if (status != CCSDS_STATUS_OK) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Invalid Header Data provided.");
  }

  FORWARD_RESULT(setData(PrimaryHeader{
    decoded.version_number,
    decoded.type,
    decoded.secondary_header_flag,
    decoded.apid,
    decoded.sequence_flags,
    decoded.sequence_count,
    decoded.data_length
  }));
  return true;
}

ccsds::ResultBool ccsds::Header::setData(const std::uint64_t &data) {
  ccsds_primary_header_t decoded{};
  const auto status = ccsds_primary_header_unpack(data, &decoded);
  if (status != CCSDS_STATUS_OK) {
    m_status = INVALID;
    return Error(INVALID_HEADER_DATA, "Input data exceeds expected bit size for version or size.");
  }

  FORWARD_RESULT(setData(PrimaryHeader{
    decoded.version_number,
    decoded.type,
    decoded.secondary_header_flag,
    decoded.apid,
    decoded.sequence_flags,
    decoded.sequence_count,
    decoded.data_length
  }));
  return true;
}

std::vector<std::uint8_t> ccsds::Header::serialize() {
  return static_cast<const Header &>(*this).serialize();
}

std::vector<std::uint8_t> ccsds::Header::serialize() const {
  if (m_status == INVALID) {
    return {};
  }

  const ccsds_primary_header_t header{
    m_versionNumber,
    m_type,
    m_secondaryHeaderFlag,
    m_APID,
    m_sequenceFlags,
    m_sequenceCount,
    m_dataLength
  };
  std::uint8_t bytes[CCSDS_PRIMARY_HEADER_SIZE]{};
  if (ccsds_primary_header_encode(&header, bytes, sizeof(bytes)) != CCSDS_STATUS_OK) {
    return {};
  }
  return std::vector<std::uint8_t>(bytes, bytes + CCSDS_PRIMARY_HEADER_SIZE);
}

std::uint64_t ccsds::Header::getFullHeader() {
  return static_cast<const Header &>(*this).getFullHeader();
}

std::uint64_t ccsds::Header::getFullHeader() const {
  if (m_status == INVALID) {
    return 0U;
  }

  const ccsds_primary_header_t header{
    m_versionNumber,
    m_type,
    m_secondaryHeaderFlag,
    m_APID,
    m_sequenceFlags,
    m_sequenceCount,
    m_dataLength
  };
  std::uint64_t packed = 0U;
  return ccsds_primary_header_pack(&header, &packed) == CCSDS_STATUS_OK ? packed : 0U;
}

ccsds::ResultBool ccsds::Header::setData(const PrimaryHeader &data) {
  // Wire field widths have exactly one authority: the C11 primary-header core.
  // The C++ layer maps field identity to established diagnostic text.
  const ccsds_primary_header_t core{
    data.versionNumber, data.type, data.secondaryHeaderFlag, data.APID,
    data.sequenceFlags, data.sequenceCount, data.dataLength
  };
  ccsds_primary_header_field_t invalid = CCSDS_PRIMARY_HEADER_FIELD_NONE;
  const auto status = ccsds_primary_header_validate_ex(&core, &invalid);
  if (status != CCSDS_STATUS_OK) {
    m_status = INVALID;
    switch (invalid) {
      case CCSDS_PRIMARY_HEADER_FIELD_VERSION:
        return Error(INVALID_HEADER_DATA, "Invalid version number, value > 7");
      case CCSDS_PRIMARY_HEADER_FIELD_TYPE:
        return Error(INVALID_HEADER_DATA, "Invalid type, value > 1");
      case CCSDS_PRIMARY_HEADER_FIELD_SECONDARY_HEADER_FLAG:
        return Error(INVALID_HEADER_DATA, "Invalid secondary header flag, value > 1");
      case CCSDS_PRIMARY_HEADER_FIELD_APID:
        return Error(INVALID_HEADER_DATA, "Invalid APID, value > 2047");
      case CCSDS_PRIMARY_HEADER_FIELD_SEQUENCE_FLAGS:
        return Error(INVALID_HEADER_DATA, "Invalid sequence flag, value > 3");
      case CCSDS_PRIMARY_HEADER_FIELD_SEQUENCE_COUNT:
        return Error(INVALID_HEADER_DATA, "Invalid sequence count, value > 16383");
      case CCSDS_PRIMARY_HEADER_FIELD_NONE:
        return Error(INVALID_HEADER_DATA, "Invalid primary header.");
    }
    return Error(INVALID_HEADER_DATA, "Invalid primary header.");
  }

  m_versionNumber = data.versionNumber;
  m_type = data.type;
  m_secondaryHeaderFlag = data.secondaryHeaderFlag;
  m_APID = data.APID;
  m_sequenceFlags = data.sequenceFlags;
  m_sequenceCount = data.sequenceCount;
  m_dataLength = data.dataLength;
  m_packetSequenceControl = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_sequenceFlags) << 14U) | m_sequenceCount);
  m_packetIdentificationAndVersion = static_cast<std::uint16_t>(
    (static_cast<std::uint16_t>(m_versionNumber) << 13U)
    | (static_cast<std::uint16_t>(m_type) << 12U)
    | (static_cast<std::uint16_t>(m_secondaryHeaderFlag) << 11U)
    | m_APID);
  refreshStatus();
  return true;
}
