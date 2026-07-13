// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSPacket.h"
#include "CCSDSDataField.h"
#include "CCSDSUtils.h"
#include <algorithm>
#include <limits>
#include <utility>

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

namespace {
  struct ParsedPacket {
    CCSDS::Header header{};
    std::vector<std::uint8_t> dataField{};
    std::uint16_t receivedCRC{};
  };

  CCSDS::Result<std::size_t> declaredPacketSize(const std::vector<std::uint8_t> &data) {
    if (data.size() < 6U) {
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: truncated CCSDS primary header."};
    }

    const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
    CCSDS::Header header;
    const auto headerResult = header.deserialize(headerData);
    if (!headerResult) return headerResult.error();
    if (header.getVersionNumber() != 0U) {
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: unsupported CCSDS packet version."};
    }

    const auto packetSize = 6U + static_cast<std::size_t>(header.getDataLength()) + 1U;
    if (data.size() < packetSize) {
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                          "Cannot deserialize packet: truncated packet body."};
    }
    return packetSize;
  }

  CCSDS::Result<ParsedPacket> validatePacketBytes(
      const std::vector<std::uint8_t> &headerData,
      const std::vector<std::uint8_t> &packetData,
      const CCSDS::PacketErrorControlMode mode,
      const CCSDS::CRC16Config &crcConfig) {
    if (headerData.size() != 6U) {
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: primary header must contain exactly six bytes."};
    }

    ParsedPacket parsed;
    const auto headerResult = parsed.header.deserialize(headerData);
    if (!headerResult) return headerResult.error();
    if (parsed.header.getVersionNumber() != 0U) {
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: unsupported CCSDS packet version."};
    }

    const auto expectedDataSize = static_cast<std::size_t>(parsed.header.getDataLength()) + 1U;
    if (packetData.size() != expectedDataSize) {
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                          "Cannot deserialize packet: packet body size does not match Packet Data Length."};
    }

    if (mode == CCSDS::PacketErrorControlMode::CRC16) {
      if (packetData.size() < 2U) {
        return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                            "Cannot deserialize packet: CRC16 mode requires two packet error-control bytes."};
      }

      parsed.receivedCRC = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(packetData[packetData.size() - 2U]) << 8U)
        | packetData.back());

      std::vector<std::uint8_t> crcInput = headerData;
      crcInput.insert(crcInput.end(), packetData.begin(), packetData.end() - 2);
      const auto expectedCRC = ::crc16(crcInput, crcConfig.polynomial,
                                       crcConfig.initialValue, crcConfig.finalXorValue);
      if (expectedCRC != parsed.receivedCRC) {
        return CCSDS::Error{CCSDS::ErrorCode::INVALID_CHECKSUM,
                            "Cannot deserialize packet: CRC16 packet error-control mismatch."};
      }
      parsed.dataField.assign(packetData.begin(), packetData.end() - 2);
    } else {
      parsed.receivedCRC = 0U;
      parsed.dataField = packetData;
    }

    return parsed;
  }
}

void CCSDS::Packet::update() {
  if (m_updateStatus || !m_enableUpdatePacket) return;
  if (m_primaryHeader.getHeaderStatus() == INVALID) return;

  const auto dataField = m_dataField.serialize();
  const auto packetDataFieldSize = dataField.size() + getPacketErrorControlSize();
  constexpr auto maximumPacketDataFieldSize =
    static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;

  if (packetDataFieldSize == 0U || packetDataFieldSize > maximumPacketDataFieldSize) return;

  m_primaryHeader.setDataLength(static_cast<std::uint16_t>(packetDataFieldSize - 1U));
  if (!m_primaryHeader.setSequenceCount(m_sequenceCounter & SEQUENCE_COUNT_MASK)) return;

  if (getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
    auto crcInput = static_cast<const Header &>(m_primaryHeader).serialize();
    if (crcInput.size() != 6U) return;
    crcInput.insert(crcInput.end(), dataField.begin(), dataField.end());
    m_CRC16 = ::crc16(crcInput, m_CRC16Config.polynomial, m_CRC16Config.initialValue,
                      m_CRC16Config.finalXorValue);
  } else {
    m_CRC16 = 0U;
  }
  m_updateStatus = true;
}

#ifndef CCSDS_MCU
CCSDS::ResultBool CCSDS::Packet::loadFromConfigFile(const std::string &configPath) {
  Config cfg;
  FORWARD_RESULT(cfg.load(configPath));
  FORWARD_RESULT(loadFromConfig(cfg));
  return true;
}

CCSDS::ResultBool CCSDS::Packet::loadFromConfig(const Config &cfg) {
  int versionNumber{};
  bool type{};
  int APID{};
  bool dataFieldHeaderFlag{};
  std::uint16_t sequenceCount{};
  ESequenceFlag sequenceFlag{};
  bool segmented{};

  RET_IF_ERR_MSG(!cfg.isKey("ccsds_version_number"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing int field: ccsds_version_number");
  RET_IF_ERR_MSG(!cfg.isKey("ccsds_type"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing bool field: ccsds_type");
  RET_IF_ERR_MSG(!cfg.isKey("ccsds_data_field_header_flag"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing bool field: ccsds_data_field_header_flag");
  RET_IF_ERR_MSG(!cfg.isKey("ccsds_APID"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing int field: ccsds_APID");
  RET_IF_ERR_MSG(!cfg.isKey("ccsds_segmented"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing bool field: ccsds_segmented");

  ASSIGN_CP(versionNumber, cfg.get<int>("ccsds_version_number"));
  ASSIGN_CP(type, cfg.get<bool>("ccsds_type"));
  ASSIGN_CP(dataFieldHeaderFlag, cfg.get<bool>("ccsds_data_field_header_flag"));
  ASSIGN_CP(APID, cfg.get<int>("ccsds_APID"));
  ASSIGN_CP(segmented, cfg.get<bool>("ccsds_segmented"));

  RET_IF_ERR_MSG(versionNumber < 0 || versionNumber > 7, ErrorCode::CONFIG_FILE_ERROR,
                 "Config: ccsds_version_number must be between 0 and 7");
  RET_IF_ERR_MSG(APID < 0 || APID > static_cast<int>(IDLE_APID), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: ccsds_APID must be between 0 and 2047");

  FORWARD_RESULT(m_primaryHeader.setVersionNumber(static_cast<std::uint8_t>(versionNumber)));
  FORWARD_RESULT(m_primaryHeader.setType(static_cast<std::uint8_t>(type)));
  FORWARD_RESULT(m_primaryHeader.setDataFieldHeaderFlag(static_cast<std::uint8_t>(dataFieldHeaderFlag)));
  FORWARD_RESULT(m_primaryHeader.setAPID(static_cast<std::uint16_t>(APID)));

  if (segmented) {
    sequenceCount = 1U;
    sequenceFlag = FIRST_SEGMENT;
  } else {
    sequenceCount = 0U;
    sequenceFlag = UNSEGMENTED;
  }
  FORWARD_RESULT(m_primaryHeader.setSequenceFlags(sequenceFlag));
  FORWARD_RESULT(m_primaryHeader.setSequenceCount(sequenceCount));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (sequenceCount & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;

  if (cfg.isKey("ccsds_packet_error_control")) {
    std::string mode;
    ASSIGN_CP(mode, cfg.get<std::string>("ccsds_packet_error_control"));
    if (mode == "none" || mode == "None") {
      setPacketErrorControlMode(PacketErrorControlMode::None);
    } else if (mode == "crc16" || mode == "CRC16") {
      setPacketErrorControlMode(PacketErrorControlMode::CRC16);
    } else {
      return Error{ErrorCode::CONFIG_FILE_ERROR,
                   "Config: ccsds_packet_error_control must be 'none' or 'crc16'"};
    }
  }

  if (cfg.isKey("data_field_size")) {
    int dataFieldSize{};
    ASSIGN_CP(dataFieldSize, cfg.get<int>("data_field_size"));
    RET_IF_ERR_MSG(dataFieldSize < 0 || dataFieldSize > std::numeric_limits<std::uint16_t>::max(),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: data_field_size must be between 0 and 65535");
    m_dataField.setDataPacketSize(static_cast<std::uint16_t>(dataFieldSize));
  }

  if (cfg.isKey("define_secondary_header")) {
    bool secondaryHeaderFlag{false};
    ASSIGN_CP(secondaryHeaderFlag, cfg.get<bool>("define_secondary_header"));
    if (secondaryHeaderFlag) FORWARD_RESULT(m_dataField.setDataFieldHeader(cfg));
  }

  if (cfg.isKey("application_data")) {
    std::vector<std::uint8_t> applicationData{};
    ASSIGN_CP(applicationData, cfg.get<std::vector<std::uint8_t>>("application_data"));
    FORWARD_RESULT(m_dataField.setApplicationData(applicationData));
  }

  return true;
}
#endif

std::uint16_t CCSDS::Packet::getCRC() {
  return static_cast<const Packet &>(*this).getCRC();
}

std::uint16_t CCSDS::Packet::getCRC() const {
  if (getPacketErrorControlMode() == PacketErrorControlMode::None
      || m_primaryHeader.getHeaderStatus() == INVALID) return 0U;
  return m_CRC16;
}

std::uint16_t CCSDS::Packet::getDataFieldMaximumSize() const {
  return m_dataField.getDataFieldAvailableBytesSize();
}

bool CCSDS::Packet::getDataFieldHeaderFlag() {
  return static_cast<const Packet &>(*this).getDataFieldHeaderFlag();
}

bool CCSDS::Packet::getDataFieldHeaderFlag() const {
  return m_primaryHeader.getDataFieldHeaderFlag() != 0U;
}

std::vector<std::uint8_t> CCSDS::Packet::getCRCVectorBytes() {
  return static_cast<const Packet &>(*this).getCRCVectorBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getCRCVectorBytes() const {
  if (getPacketErrorControlMode() == PacketErrorControlMode::None
      || m_primaryHeader.getHeaderStatus() == INVALID) return {};
  return {static_cast<std::uint8_t>((m_CRC16 >> 8U) & 0xFFU),
          static_cast<std::uint8_t>(m_CRC16 & 0xFFU)};
}

CCSDS::DataField &CCSDS::Packet::getDataField() { return m_dataField; }
const CCSDS::DataField &CCSDS::Packet::getDataField() const { return m_dataField; }
CCSDS::Header &CCSDS::Packet::getPrimaryHeader() { return m_primaryHeader; }
const CCSDS::Header &CCSDS::Packet::getPrimaryHeader() const { return m_primaryHeader; }

std::uint64_t CCSDS::Packet::getPrimaryHeader64bit() {
  return static_cast<const Packet &>(*this).getPrimaryHeader64bit();
}

std::uint64_t CCSDS::Packet::getPrimaryHeader64bit() const {
  return m_primaryHeader.getFullHeader();
}

std::vector<std::uint8_t> CCSDS::Packet::getPrimaryHeaderBytes() {
  return static_cast<const Packet &>(*this).getPrimaryHeaderBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getPrimaryHeaderBytes() const {
  return m_primaryHeader.serialize();
}

std::vector<std::uint8_t> CCSDS::Packet::getDataFieldHeaderBytes() {
  return static_cast<const Packet &>(*this).getDataFieldHeaderBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getDataFieldHeaderBytes() const {
  return m_dataField.getDataFieldHeaderBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getApplicationDataBytes() {
  return static_cast<const Packet &>(*this).getApplicationDataBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getApplicationDataBytes() const {
  return m_dataField.getApplicationData();
}

std::vector<std::uint8_t> CCSDS::Packet::getFullDataFieldBytes() {
  return static_cast<const Packet &>(*this).getFullDataFieldBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getFullDataFieldBytes() const {
  auto data = m_dataField.getDataFieldHeaderBytes();
  const auto applicationData = m_dataField.getApplicationData();
  data.insert(data.end(), applicationData.begin(), applicationData.end());
  return data;
}

std::vector<std::uint8_t> CCSDS::Packet::serialize() {
  if (m_primaryHeader.getHeaderStatus() == INVALID) return {};

  const auto currentDataField = getFullDataFieldBytes();
  const auto currentPacketDataFieldSize = currentDataField.size() + getPacketErrorControlSize();
  constexpr auto maximumPacketDataFieldSize =
    static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;
  if (currentPacketDataFieldSize == 0U || currentPacketDataFieldSize > maximumPacketDataFieldSize) {
    return {};
  }

  update();
  const auto header = static_cast<const Header &>(m_primaryHeader).serialize();
  if (header.size() != 6U) return {};
  const auto dataField = getFullDataFieldBytes();

  std::vector<std::uint8_t> packet;
  packet.reserve(header.size() + dataField.size() + getPacketErrorControlSize());
  packet.insert(packet.end(), header.begin(), header.end());
  packet.insert(packet.end(), dataField.begin(), dataField.end());
  if (getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
    packet.push_back(static_cast<std::uint8_t>((m_CRC16 >> 8U) & 0xFFU));
    packet.push_back(static_cast<std::uint8_t>(m_CRC16 & 0xFFU));
  }
  return packet;
}

CCSDS::Result<std::size_t> CCSDS::Packet::deserializeBounded(
    const std::vector<std::uint8_t> &data) {
  std::size_t packetSize{};
  ASSIGN_CP(packetSize, declaredPacketSize(data));
  const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
  const std::vector<std::uint8_t> packetData(data.begin() + 6,
                                              data.begin() + static_cast<std::ptrdiff_t>(packetSize));
  const auto parseResult = deserialize(headerData, packetData);
  if (!parseResult) return parseResult.error();
  return packetSize;
}

CCSDS::Result<std::size_t> CCSDS::Packet::deserializeBounded(
    const std::vector<std::uint8_t> &data, const std::string &headerType,
    const std::int32_t headerSize) {
  RET_IF_ERR_MSG(headerType == "BufferHeader", ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: BufferHeader requires an explicit byte size.");
  RET_IF_ERR_MSG(!m_dataField.getDataFieldHeaderFactory().typeIsRegistered(headerType),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: unregistered secondary header: " + headerType);

  std::size_t packetSize{};
  ASSIGN_CP(packetSize, declaredPacketSize(data));
  const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
  const std::vector<std::uint8_t> packetData(data.begin() + 6,
                                              data.begin() + static_cast<std::ptrdiff_t>(packetSize));
  const auto validated = validatePacketBytes(headerData, packetData,
                                             getPacketErrorControlMode(), m_CRC16Config);
  if (!validated) return validated.error();

  auto parsed = validated.value();
  DataField parsedField = m_dataField;
  parsedField.clearContent();
  auto secondaryHeader = parsedField.getDataFieldHeaderFactory().create(headerType);
  RET_IF_ERR_MSG(!secondaryHeader, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: failed to create secondary header: " + headerType);

  std::size_t secondaryHeaderSize = secondaryHeader->getSize();
  if (headerType == "PusC" && headerSize > 0) secondaryHeaderSize = static_cast<std::size_t>(headerSize);
  RET_IF_ERR_MSG(secondaryHeaderSize > parsed.dataField.size(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: secondary header exceeds the packet data field.");

  const std::vector<std::uint8_t> secondaryBytes(
    parsed.dataField.begin(), parsed.dataField.begin() + static_cast<std::ptrdiff_t>(secondaryHeaderSize));
  const auto secondaryResult = secondaryHeader->deserialize(secondaryBytes);
  if (!secondaryResult) return secondaryResult.error();
  parsedField.setDataFieldHeader(secondaryHeader);

  const std::vector<std::uint8_t> applicationData(
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(secondaryHeaderSize), parsed.dataField.end());
  const auto applicationResult = parsedField.setApplicationData(applicationData);
  if (!applicationResult) return applicationResult.error();

  m_primaryHeader = parsed.header;
  m_dataField = std::move(parsedField);
  m_CRC16 = parsed.receivedCRC;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = true;
  return packetSize;
}

CCSDS::Result<std::size_t> CCSDS::Packet::deserializeBounded(
    const std::vector<std::uint8_t> &data, const std::uint16_t headerDataSizeBytes) {
  std::size_t packetSize{};
  ASSIGN_CP(packetSize, declaredPacketSize(data));
  const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
  const std::vector<std::uint8_t> packetData(data.begin() + 6,
                                              data.begin() + static_cast<std::ptrdiff_t>(packetSize));
  const auto validated = validatePacketBytes(headerData, packetData,
                                             getPacketErrorControlMode(), m_CRC16Config);
  if (!validated) return validated.error();

  auto parsed = validated.value();
  RET_IF_ERR_MSG(static_cast<std::size_t>(headerDataSizeBytes) > parsed.dataField.size(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: secondary-header size exceeds the packet data field.");

  DataField parsedField = m_dataField;
  parsedField.clearContent();
  const std::vector<std::uint8_t> secondaryBytes(
    parsed.dataField.begin(),
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(headerDataSizeBytes));
  if (!secondaryBytes.empty()) {
    const auto secondaryResult = parsedField.setDataFieldHeader(secondaryBytes);
    if (!secondaryResult) return secondaryResult.error();
  }
  const std::vector<std::uint8_t> applicationData(
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(headerDataSizeBytes), parsed.dataField.end());
  const auto applicationResult = parsedField.setApplicationData(applicationData);
  if (!applicationResult) return applicationResult.error();

  m_primaryHeader = parsed.header;
  m_dataField = std::move(parsedField);
  m_CRC16 = parsed.receivedCRC;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = true;
  return packetSize;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &data) {
  const auto result = deserializeBounded(data);
  if (!result) return result.error();
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &data,
                                              const std::string &headerType,
                                              const std::int32_t headerSize) {
  const auto result = deserializeBounded(data, headerType, headerSize);
  if (!result) return result.error();
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &data,
                                              const std::uint16_t headerDataSizeBytes) {
  const auto result = deserializeBounded(data, headerDataSizeBytes);
  if (!result) return result.error();
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &headerData,
                                              const std::vector<std::uint8_t> &data) {
  const auto validated = validatePacketBytes(headerData, data,
                                             getPacketErrorControlMode(), m_CRC16Config);
  if (!validated) return validated.error();

  auto parsed = validated.value();
  DataField parsedField = m_dataField;
  parsedField.clearContent();
  FORWARD_RESULT(parsedField.setApplicationData(parsed.dataField));

  m_primaryHeader = parsed.header;
  m_dataField = std::move(parsedField);
  m_CRC16 = parsed.receivedCRC;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = true;
  return true;
}

std::uint16_t CCSDS::Packet::getFullPacketLength() {
  return static_cast<const Packet &>(*this).getFullPacketLength();
}

std::uint16_t CCSDS::Packet::getFullPacketLength() const {
  return static_cast<std::uint16_t>(6U + m_dataField.getDataFieldUsedBytesSize()
                                    + getPacketErrorControlSize());
}

CCSDS::ResultBool CCSDS::Packet::setPrimaryHeader(const std::uint64_t data) {
  FORWARD_RESULT(m_primaryHeader.setData(data));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setPrimaryHeader(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_primaryHeader.deserialize(data));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setPrimaryHeader(const Header &header) {
  m_primaryHeader = header;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setPrimaryHeader(const PrimaryHeader data) {
  FORWARD_RESULT(m_primaryHeader.setData(data));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setDataFieldHeader(const std::shared_ptr<SecondaryHeaderAbstract> &header) {
  m_dataField.setDataFieldHeader(header);
  if (!m_primaryHeader.setDataFieldHeaderFlag(header ? 1U : 0U)) return;
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(
    const std::vector<std::uint8_t> &data, const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(data, headerType));
  FORWARD_RESULT(m_primaryHeader.setDataFieldHeaderFlag(1U));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::uint8_t *pData,
                                                     const std::size_t sizeData,
                                                     const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(pData, sizeData, headerType));
  FORWARD_RESULT(m_primaryHeader.setDataFieldHeaderFlag(1U));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(data));
  FORWARD_RESULT(m_primaryHeader.setDataFieldHeaderFlag(1U));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::uint8_t *pData,
                                                     const std::size_t sizeData) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(pData, sizeData));
  FORWARD_RESULT(m_primaryHeader.setDataFieldHeaderFlag(1U));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setApplicationData(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setApplicationData(data));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setApplicationData(const std::uint8_t *pData,
                                                     const std::size_t sizeData) {
  FORWARD_RESULT(m_dataField.setApplicationData(pData, sizeData));
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setSequenceFlags(const ESequenceFlag flags) {
  if (!m_primaryHeader.setSequenceFlags(flags)) return;
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setSequenceCount(const std::uint16_t count) {
  RET_IF_ERR_MSG(count > SEQUENCE_COUNT_MASK, ErrorCode::INVALID_HEADER_DATA,
                 "Unable to set sequence count above 16383");
  FORWARD_RESULT(m_primaryHeader.setSequenceCount(count));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) | count;
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setDataFieldSize(const std::uint16_t size) {
  m_dataField.setDataPacketSize(size);
  m_updateStatus = false;
}

void CCSDS::Packet::setUpdatePacketEnable(const bool enable) {
  m_enableUpdatePacket = enable;
  m_dataField.setDataFieldHeaderAutoUpdateStatus(enable);
}

void CCSDS::Packet::setPacketErrorControlMode(const PacketErrorControlMode mode) {
  if (mode == PacketErrorControlMode::None) {
    m_sequenceCounter |= PACKET_ERROR_CONTROL_DISABLED_MASK;
    m_CRC16 = 0U;
  } else {
    m_sequenceCounter &= static_cast<std::uint16_t>(~PACKET_ERROR_CONTROL_DISABLED_MASK);
  }
  m_updateStatus = false;
}
