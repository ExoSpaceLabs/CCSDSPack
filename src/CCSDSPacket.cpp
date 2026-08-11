// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSPacket.h"
#include "CCSDSDataField.h"
#include "CCSDSUtils.h"
#include "PusSecondaryHeaders.h"
#include <algorithm>
#include <limits>
#include <utility>

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

namespace {
  struct ParsedPacket {
    ccsds::Header header{};
    std::vector<std::uint8_t> dataField{};
    std::uint16_t receivedCRC{};
  };

  ccsds::ResultBool validatePacketForSerialization(
      const ccsds::Header &header,
      const ccsds::DataField &dataField) {
    RET_IF_ERR_MSG(header.getHeaderStatus() == ccsds::INVALID,
                   ccsds::ErrorCode::INVALID_HEADER_DATA,
                   "Cannot serialize packet: primary header is invalid.");
    RET_IF_ERR_MSG(header.getVersionNumber() != 0U,
                   ccsds::ErrorCode::INVALID_HEADER_DATA,
                   "Cannot serialize packet: unsupported CCSDS packet version.");

    if (header.getAPID() == ccsds::IDLE_APID) {
      RET_IF_ERR_MSG(header.getSecondaryHeaderFlag() != 0U,
                     ccsds::ErrorCode::INVALID_HEADER_DATA,
                     "Cannot serialize Idle Packet: secondary-header flag must be zero.");
      RET_IF_ERR_MSG(dataField.getSecondaryHeaderFlag(),
                     ccsds::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "Cannot serialize Idle Packet with a secondary header.");
      RET_IF_ERR_MSG(dataField.getApplicationDataBytesSize() == 0U,
                     ccsds::ErrorCode::INVALID_DATA,
                     "Cannot serialize Idle Packet without mission-defined idle user data.");
      return true;
    }

    const auto secondary = dataField.getSecondaryHeader();
    RET_IF_ERR_MSG((header.getSecondaryHeaderFlag() != 0U) != static_cast<bool>(secondary),
                   ccsds::ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Cannot serialize packet: primary-header flag and secondary-header state differ.");

    if (secondary && secondary->getDirection() != ccsds::PacketDirection::Unspecified) {
      RET_IF_ERR_MSG(header.getType()
                       != ccsds::packetTypeForDirection(secondary->getDirection()),
                     ccsds::ErrorCode::INVALID_HEADER_DATA,
                     "Cannot serialize packet: primary-header Packet Type differs from secondary-header direction.");
    }
    return true;
  }

  ccsds::Result<std::size_t> declaredPacketSize(const std::vector<std::uint8_t> &data) {
    if (data.size() < 6U) {
      return ccsds::Error{ccsds::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: truncated CCSDS primary header."};
    }
    const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
    ccsds::Header header;
    const auto headerResult = header.deserialize(headerData);
    if (!headerResult) return headerResult.error();
    if (header.getVersionNumber() != 0U) {
      return ccsds::Error{ccsds::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: unsupported CCSDS packet version."};
    }
    const auto packetSize = 6U + static_cast<std::size_t>(header.getDataLength()) + 1U;
    if (data.size() < packetSize) {
      return ccsds::Error{ccsds::ErrorCode::INVALID_DATA,
                          "Cannot deserialize packet: truncated packet body."};
    }
    return packetSize;
  }

  ccsds::Result<ParsedPacket> validatePacketBytes(
      const std::vector<std::uint8_t> &headerData,
      const std::vector<std::uint8_t> &packetData,
      const ccsds::PacketErrorControlMode mode,
      const ccsds::CRC16Config &crcConfig) {
    if (headerData.size() != 6U) {
      return ccsds::Error{ccsds::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: primary header must contain exactly six bytes."};
    }

    ParsedPacket parsed;
    const auto headerResult = parsed.header.deserialize(headerData);
    if (!headerResult) return headerResult.error();
    if (parsed.header.getVersionNumber() != 0U) {
      return ccsds::Error{ccsds::ErrorCode::INVALID_HEADER_DATA,
                          "Cannot deserialize packet: unsupported CCSDS packet version."};
    }

    const auto expectedDataSize = static_cast<std::size_t>(parsed.header.getDataLength()) + 1U;
    if (packetData.size() != expectedDataSize) {
      return ccsds::Error{ccsds::ErrorCode::INVALID_DATA,
                          "Cannot deserialize packet: packet body size does not match Packet Data Length."};
    }

    if (mode == ccsds::PacketErrorControlMode::CRC16) {
      if (packetData.size() < 2U) {
        return ccsds::Error{ccsds::ErrorCode::INVALID_DATA,
                            "Cannot deserialize packet: CRC16 mode requires two packet error-control bytes."};
      }
      parsed.receivedCRC = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(packetData[packetData.size() - 2U]) << 8U)
        | packetData.back());
      std::vector<std::uint8_t> crcInput = headerData;
      crcInput.insert(crcInput.end(), packetData.begin(), packetData.end() - 2);
      const auto expectedCRC = ccsds::crc16(crcInput, crcConfig.polynomial,
                                            crcConfig.initialValue, crcConfig.finalXorValue);
      if (expectedCRC != parsed.receivedCRC) {
        return ccsds::Error{ccsds::ErrorCode::INVALID_CHECKSUM,
                            "Cannot deserialize packet: CRC16 packet error-control mismatch."};
      }
      parsed.dataField.assign(packetData.begin(), packetData.end() - 2);
    } else {
      parsed.receivedCRC = 0U;
      parsed.dataField = packetData;
    }

    if (parsed.header.getAPID() == ccsds::IDLE_APID) {
      if (parsed.header.getSecondaryHeaderFlag() != 0U) {
        return ccsds::Error{ccsds::ErrorCode::INVALID_HEADER_DATA,
                            "Cannot deserialize Idle Packet: secondary-header flag must be zero."};
      }
      if (parsed.dataField.empty()) {
        return ccsds::Error{ccsds::ErrorCode::INVALID_DATA,
                            "Cannot deserialize Idle Packet: mission-defined idle user data is required."};
      }
    }
    return parsed;
  }

#ifndef CCSDS_MCU
  ccsds::Result<ccsds::PacketErrorControlMode> packetErrorControlFromConfig(
      const ccsds::Config &cfg) {
    if (!cfg.isKey("ccsds_packet_error_control")) return ccsds::PacketErrorControlMode::CRC16;
    const auto mode = cfg.get<std::string>("ccsds_packet_error_control");
    RET_IF_ERR_MSG(!mode, ccsds::ErrorCode::CONFIG_FILE_ERROR,
                   "Config: invalid ccsds_packet_error_control.");
    if (mode.value() == "crc16") return ccsds::PacketErrorControlMode::CRC16;
    if (mode.value() == "none") return ccsds::PacketErrorControlMode::None;
    return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR,
                        "Config: ccsds_packet_error_control must be 'none' or 'crc16'."};
  }
#endif
}

ccsds::ResultBool ccsds::Packet::update() {
  FORWARD_RESULT(validatePacketForSerialization(m_primaryHeader, m_dataField));
  if (m_updateStatus || !m_enableUpdatePacket) return true;

  std::vector<std::uint8_t> dataField;
  ASSIGN_MV(dataField, m_dataField.serialize());
  const auto packetDataFieldSize = dataField.size() + getPacketErrorControlSize();
  constexpr auto maximumPacketDataFieldSize =
    static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;

  RET_IF_ERR_MSG(packetDataFieldSize == 0U, ErrorCode::INVALID_DATA,
                 "Cannot finalize packet: Packet Data Field is empty.");
  RET_IF_ERR_MSG(packetDataFieldSize > maximumPacketDataFieldSize, ErrorCode::INVALID_DATA,
                 "Cannot finalize packet: Packet Data Field exceeds the CCSDS length field.");

  m_primaryHeader.setDataLength(static_cast<std::uint16_t>(packetDataFieldSize - 1U));
  FORWARD_RESULT(m_primaryHeader.setSequenceCount(m_sequenceCounter & SEQUENCE_COUNT_MASK));

  if (getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
    auto crcInput = static_cast<const Header &>(m_primaryHeader).serialize();
    RET_IF_ERR_MSG(crcInput.size() != 6U, ErrorCode::INVALID_HEADER_DATA,
                   "Cannot finalize packet: primary-header serialization failed.");
    crcInput.insert(crcInput.end(), dataField.begin(), dataField.end());
    m_CRC16 = ccsds::crc16(crcInput, m_CRC16Config.polynomial,
                           m_CRC16Config.initialValue, m_CRC16Config.finalXorValue);
  } else {
    m_CRC16 = 0U;
  }
  m_updateStatus = true;
  return true;
}

#ifndef CCSDS_MCU
ccsds::ResultBool ccsds::Packet::loadFromConfigFile(const std::string &configPath) {
  ccsds::Config cfg;
  FORWARD_RESULT(cfg.load(configPath));
  return loadFromConfig(cfg);
}

ccsds::ResultBool ccsds::Packet::loadFromConfig(const ccsds::Config &cfg) {
  int versionNumber{};
  int APID{};
  bool segmented{};
  bool configuredType{false};
  const bool hasConfiguredType = cfg.isKey("ccsds_type");

  RET_IF_ERR_MSG(!cfg.isKey("ccsds_version_number"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing int field: ccsds_version_number");
  RET_IF_ERR_MSG(!cfg.isKey("ccsds_APID"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing int field: ccsds_APID");
  RET_IF_ERR_MSG(!cfg.isKey("ccsds_segmented"), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Missing bool field: ccsds_segmented");

  ASSIGN_CP(versionNumber, cfg.get<int>("ccsds_version_number"));
  ASSIGN_CP(APID, cfg.get<int>("ccsds_APID"));
  ASSIGN_CP(segmented, cfg.get<bool>("ccsds_segmented"));
  if (hasConfiguredType) ASSIGN_CP(configuredType, cfg.get<bool>("ccsds_type"));

  RET_IF_ERR_MSG(versionNumber != 0, ErrorCode::CONFIG_FILE_ERROR,
                 "Config: ccsds_version_number must be 0 for CCSDS 133.0-B-2 Space Packets");
  RET_IF_ERR_MSG(APID < 0 || APID > static_cast<int>(IDLE_APID), ErrorCode::CONFIG_FILE_ERROR,
                 "Config: ccsds_APID must be between 0 and 2047");

  FORWARD_RESULT(m_primaryHeader.setVersionNumber(static_cast<std::uint8_t>(versionNumber)));
  if (hasConfiguredType) FORWARD_RESULT(m_primaryHeader.setType(static_cast<std::uint8_t>(configuredType)));
  FORWARD_RESULT(m_primaryHeader.setAPID(static_cast<std::uint16_t>(APID)));

  PacketErrorControlMode errorControl{};
  ASSIGN_CP(errorControl, packetErrorControlFromConfig(cfg));
  setPacketErrorControlMode(errorControl);

  const auto sequenceFlag = segmented ? FIRST_SEGMENT : UNSEGMENTED;
  const auto sequenceCount = static_cast<std::uint16_t>(segmented ? 1U : 0U);
  FORWARD_RESULT(m_primaryHeader.setSequenceFlags(sequenceFlag));
  FORWARD_RESULT(m_primaryHeader.setSequenceCount(sequenceCount));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (sequenceCount & SEQUENCE_COUNT_MASK);

  if (cfg.isKey("data_field_size")) {
    int dataFieldSize{};
    ASSIGN_CP(dataFieldSize, cfg.get<int>("data_field_size"));
    RET_IF_ERR_MSG(dataFieldSize < 0 || dataFieldSize > std::numeric_limits<std::uint16_t>::max(),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: data_field_size must be between 0 and 65535");
    m_dataField.setDataPacketSize(static_cast<std::uint16_t>(dataFieldSize));
  }

  bool defineSecondaryHeader{false};
  if (cfg.isKey("define_secondary_header")) {
    ASSIGN_CP(defineSecondaryHeader, cfg.get<bool>("define_secondary_header"));
  }

  if (defineSecondaryHeader) {
    FORWARD_RESULT(m_dataField.setSecondaryHeader(cfg));
    const auto secondary = m_dataField.getSecondaryHeader();
    RET_IF_ERR_MSG(!secondary, ErrorCode::CONFIG_FILE_ERROR,
                   "Config: define_secondary_header did not create a header.");
    const auto declaredDirection = secondary->getDirection();
    if (hasConfiguredType && declaredDirection != PacketDirection::Unspecified) {
      RET_IF_ERR_MSG(static_cast<std::uint8_t>(configuredType)
                       != packetTypeForDirection(declaredDirection),
                     ErrorCode::CONFIG_FILE_ERROR,
                     "Config: ccsds_type contradicts the concrete secondary-header direction.");
    }
    FORWARD_RESULT(setSecondaryHeader(secondary));
  } else {
    FORWARD_RESULT(setSecondaryHeader(nullptr));
    RET_IF_ERR_MSG(!hasConfiguredType && APID != static_cast<int>(IDLE_APID),
                   ErrorCode::CONFIG_FILE_ERROR,
                   "Config: direction-neutral packets require ccsds_type.");
  }

  if (cfg.isKey("application_data")) {
    std::vector<std::uint8_t> applicationData{};
    ASSIGN_CP(applicationData, cfg.get<std::vector<std::uint8_t>>("application_data"));
    FORWARD_RESULT(m_dataField.setApplicationData(applicationData));
  }

  if (m_primaryHeader.getAPID() == IDLE_APID) {
    const auto idleResult = validatePacketForSerialization(m_primaryHeader, m_dataField);
    if (!idleResult) {
      return Error{ErrorCode::CONFIG_FILE_ERROR,
                   "Config: invalid Idle Packet: " + idleResult.error().message()};
    }
  }
  m_updateStatus = false;
  return true;
}
#endif

std::uint16_t ccsds::Packet::getCRC() {
  return static_cast<const Packet &>(*this).getCRC();
}

std::uint16_t ccsds::Packet::getCRC() const {
  if (getPacketErrorControlMode() == PacketErrorControlMode::None
      || m_primaryHeader.getHeaderStatus() == INVALID) return 0U;
  return m_CRC16;
}

std::uint16_t ccsds::Packet::getDataFieldMaximumSize() const {
  return m_dataField.getDataFieldAvailableBytesSize();
}

bool ccsds::Packet::getSecondaryHeaderFlag() {
  return static_cast<const Packet &>(*this).getSecondaryHeaderFlag();
}

bool ccsds::Packet::getSecondaryHeaderFlag() const {
  return m_primaryHeader.getSecondaryHeaderFlag() != 0U;
}

std::vector<std::uint8_t> ccsds::Packet::getCRCVectorBytes() {
  return static_cast<const Packet &>(*this).getCRCVectorBytes();
}

std::vector<std::uint8_t> ccsds::Packet::getCRCVectorBytes() const {
  if (getPacketErrorControlMode() == PacketErrorControlMode::None
      || m_primaryHeader.getHeaderStatus() == INVALID) return {};
  return {static_cast<std::uint8_t>((m_CRC16 >> 8U) & 0xFFU),
          static_cast<std::uint8_t>(m_CRC16 & 0xFFU)};
}

ccsds::DataField &ccsds::Packet::getDataField() { return m_dataField; }
const ccsds::DataField &ccsds::Packet::getDataField() const { return m_dataField; }
ccsds::Header &ccsds::Packet::getPrimaryHeader() { return m_primaryHeader; }
const ccsds::Header &ccsds::Packet::getPrimaryHeader() const { return m_primaryHeader; }

std::uint64_t ccsds::Packet::getPrimaryHeader64bit() {
  return static_cast<const Packet &>(*this).getPrimaryHeader64bit();
}

std::uint64_t ccsds::Packet::getPrimaryHeader64bit() const {
  return m_primaryHeader.getFullHeader();
}

std::vector<std::uint8_t> ccsds::Packet::getPrimaryHeaderBytes() {
  return static_cast<const Packet &>(*this).getPrimaryHeaderBytes();
}

std::vector<std::uint8_t> ccsds::Packet::getPrimaryHeaderBytes() const {
  return m_primaryHeader.serialize();
}

std::shared_ptr<ccsds::SecondaryHeaderAbstract> ccsds::Packet::getSecondaryHeader() {
  return m_dataField.getSecondaryHeader();
}

std::shared_ptr<const ccsds::SecondaryHeaderAbstract> ccsds::Packet::getSecondaryHeader() const {
  return m_dataField.getSecondaryHeader();
}

std::vector<std::uint8_t> ccsds::Packet::getSecondaryHeaderBytes() {
  return static_cast<const Packet &>(*this).getSecondaryHeaderBytes();
}

std::vector<std::uint8_t> ccsds::Packet::getSecondaryHeaderBytes() const {
  return m_dataField.getSecondaryHeaderBytes();
}

std::vector<std::uint8_t> ccsds::Packet::getApplicationDataBytes() {
  return static_cast<const Packet &>(*this).getApplicationDataBytes();
}

std::vector<std::uint8_t> ccsds::Packet::getApplicationDataBytes() const {
  return m_dataField.getApplicationData();
}

std::vector<std::uint8_t> ccsds::Packet::getFullDataFieldBytes() {
  return static_cast<const Packet &>(*this).getFullDataFieldBytes();
}

std::vector<std::uint8_t> ccsds::Packet::getFullDataFieldBytes() const {
  auto data = m_dataField.getSecondaryHeaderBytes();
  const auto applicationData = m_dataField.getApplicationData();
  data.insert(data.end(), applicationData.begin(), applicationData.end());
  return data;
}

ccsds::ResultBuffer ccsds::Packet::serialize() {
  const auto updateResult = update();
  if (!updateResult) return updateResult.error();
  const auto header = static_cast<const Header &>(m_primaryHeader).serialize();
  RET_IF_ERR_MSG(header.size() != 6U, ErrorCode::INVALID_HEADER_DATA,
                 "Cannot serialize packet: primary-header serialization failed.");
  std::vector<std::uint8_t> dataField;
  ASSIGN_MV(dataField, m_dataField.serialize());

  const auto packetDataFieldSize = dataField.size() + getPacketErrorControlSize();
  constexpr auto maximumPacketDataFieldSize =
    static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;
  RET_IF_ERR_MSG(packetDataFieldSize == 0U, ErrorCode::INVALID_DATA,
                 "Cannot serialize packet: Packet Data Field is empty.");
  RET_IF_ERR_MSG(packetDataFieldSize > maximumPacketDataFieldSize, ErrorCode::INVALID_DATA,
                 "Cannot serialize packet: Packet Data Field exceeds the CCSDS length field.");
  RET_IF_ERR_MSG(static_cast<std::size_t>(m_primaryHeader.getDataLength()) + 1U
                   != packetDataFieldSize,
                 ErrorCode::INVALID_DATA,
                 "Cannot serialize packet: Packet Data Length does not match the finalized data field.");

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

ccsds::Result<std::size_t> ccsds::Packet::deserializeBoundedWithSecondaryHeader(
    const std::vector<std::uint8_t> &data,
    const std::shared_ptr<SecondaryHeaderAbstract> &prototype,
    const std::int32_t customHeaderSize) {
  RET_IF_ERR_MSG(!prototype, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: secondary-header prototype is null.");

  std::size_t packetSize{};
  ASSIGN_CP(packetSize, declaredPacketSize(data));
  const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
  const std::vector<std::uint8_t> packetData(
    data.begin() + 6, data.begin() + static_cast<std::ptrdiff_t>(packetSize));
  const auto validated = validatePacketBytes(headerData, packetData,
                                             getPacketErrorControlMode(), m_CRC16Config);
  if (!validated) return validated.error();
  auto parsed = validated.value();

  RET_IF_ERR_MSG(parsed.header.getSecondaryHeaderFlag() == 0U,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize typed secondary header: CCSDS secondary-header flag is zero.");
  if (prototype->getDirection() != PacketDirection::Unspecified) {
    RET_IF_ERR_MSG(parsed.header.getType()
                     != packetTypeForDirection(prototype->getDirection()),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Secondary-header direction does not match the CCSDS Packet Type.");
  }

  DataField parsedField = m_dataField;
  parsedField.clearContent();
  std::shared_ptr<SecondaryHeaderAbstract> secondaryHeader;
  if (prototype->isPusHeader()) {
    const auto &pusPrototype = static_cast<const pus::SecondaryHeader &>(*prototype);
    auto cloneResult = parsedField.getPusSecondaryHeaderFactory().clone(pusPrototype);
    if (!cloneResult) return cloneResult.error();
    secondaryHeader = cloneResult.value();
  } else {
    RET_IF_ERR_MSG(!parsedField.getSecondaryHeaderFactory().typeIsRegistered(prototype->getType()),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Cannot deserialize packet: custom secondary-header type is not registered: "
                     + prototype->getType());
    secondaryHeader = parsedField.getSecondaryHeaderFactory().create(prototype->getType());
  }
  RET_IF_ERR_MSG(!secondaryHeader, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: failed to create secondary-header parser.");

  std::size_t secondaryHeaderSize = secondaryHeader->getSize();
  if (!secondaryHeader->isPusHeader() && secondaryHeader->variableLength
      && customHeaderSize > 0) {
    secondaryHeaderSize = static_cast<std::size_t>(customHeaderSize);
  }
  RET_IF_ERR_MSG(secondaryHeaderSize > parsed.dataField.size(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: secondary header exceeds the packet data field.");

  const std::vector<std::uint8_t> secondaryBytes(
    parsed.dataField.begin(),
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(secondaryHeaderSize));
  FORWARD_RESULT(secondaryHeader->deserialize(secondaryBytes));
  FORWARD_RESULT(parsedField.setSecondaryHeader(secondaryHeader));

  const std::vector<std::uint8_t> applicationData(
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(secondaryHeaderSize),
    parsed.dataField.end());
  FORWARD_RESULT(parsedField.setApplicationData(applicationData));

  m_primaryHeader = parsed.header;
  m_dataField = std::move(parsedField);
  m_CRC16 = parsed.receivedCRC;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = true;
  return packetSize;
}

ccsds::Result<std::size_t> ccsds::Packet::deserializeBounded(
    const std::vector<std::uint8_t> &data) {
  if (const auto prototype = m_dataField.getSecondaryHeader()) {
    return deserializeBoundedWithSecondaryHeader(data, prototype);
  }

  std::size_t packetSize{};
  ASSIGN_CP(packetSize, declaredPacketSize(data));
  const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
  const std::vector<std::uint8_t> packetData(
    data.begin() + 6, data.begin() + static_cast<std::ptrdiff_t>(packetSize));
  const auto parseResult = deserialize(headerData, packetData);
  if (!parseResult) return parseResult.error();
  return packetSize;
}

ccsds::Result<std::size_t> ccsds::Packet::deserializeBounded(
    const std::vector<std::uint8_t> &data, const std::string &headerType,
    const std::int32_t headerSize) {
  RET_IF_ERR_MSG(headerType == "BufferHeader", ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: BufferHeader requires an explicit byte size.");
  const bool isPus = pus::SecondaryHeaderFactory::isPusSelector(headerType);
  RET_IF_ERR_MSG(isPus
                   ? !m_dataField.getPusSecondaryHeaderFactory().typeIsSupported(headerType)
                   : !m_dataField.getSecondaryHeaderFactory().typeIsRegistered(headerType),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: unsupported secondary header: " + headerType);

  std::shared_ptr<SecondaryHeaderAbstract> prototype;
  const auto installed = m_dataField.getSecondaryHeader();
  if (isPus && installed && installed->isPusHeader() && installed->getType() == headerType) {
    prototype = installed;
  } else if (isPus) {
    auto createResult = m_dataField.getPusSecondaryHeaderFactory().create(headerType);
    if (!createResult) return createResult.error();
    prototype = createResult.value();
  } else {
    prototype = m_dataField.getSecondaryHeaderFactory().create(headerType);
  }
  return deserializeBoundedWithSecondaryHeader(data, prototype, headerSize);
}

ccsds::Result<std::size_t> ccsds::Packet::deserializeBounded(
    const std::vector<std::uint8_t> &data, const std::uint16_t headerDataSizeBytes) {
  std::size_t packetSize{};
  ASSIGN_CP(packetSize, declaredPacketSize(data));
  const std::vector<std::uint8_t> headerData(data.begin(), data.begin() + 6);
  const std::vector<std::uint8_t> packetData(
    data.begin() + 6, data.begin() + static_cast<std::ptrdiff_t>(packetSize));
  const auto validated = validatePacketBytes(headerData, packetData,
                                             getPacketErrorControlMode(), m_CRC16Config);
  if (!validated) return validated.error();
  auto parsed = validated.value();
  RET_IF_ERR_MSG(parsed.header.getSecondaryHeaderFlag() == 0U && headerDataSizeBytes != 0U,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize opaque secondary header: CCSDS flag is zero.");
  RET_IF_ERR_MSG(static_cast<std::size_t>(headerDataSizeBytes) > parsed.dataField.size(),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot deserialize packet: secondary-header size exceeds the packet data field.");

  DataField parsedField = m_dataField;
  parsedField.clearContent();
  const std::vector<std::uint8_t> secondaryBytes(
    parsed.dataField.begin(),
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(headerDataSizeBytes));
  if (!secondaryBytes.empty()) FORWARD_RESULT(parsedField.setSecondaryHeader(secondaryBytes));
  const std::vector<std::uint8_t> applicationData(
    parsed.dataField.begin() + static_cast<std::ptrdiff_t>(headerDataSizeBytes),
    parsed.dataField.end());
  FORWARD_RESULT(parsedField.setApplicationData(applicationData));

  m_primaryHeader = parsed.header;
  m_dataField = std::move(parsedField);
  m_CRC16 = parsed.receivedCRC;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = true;
  return packetSize;
}

ccsds::ResultBool ccsds::Packet::deserialize(const std::vector<std::uint8_t> &data) {
  const auto result = deserializeBounded(data);
  if (!result) return result.error();
  return true;
}

ccsds::ResultBool ccsds::Packet::deserialize(const std::vector<std::uint8_t> &data,
                                              const std::string &headerType,
                                              const std::int32_t headerSize) {
  const auto result = deserializeBounded(data, headerType, headerSize);
  if (!result) return result.error();
  return true;
}

ccsds::ResultBool ccsds::Packet::deserialize(const std::vector<std::uint8_t> &data,
                                              const std::uint16_t headerDataSizeBytes) {
  const auto result = deserializeBounded(data, headerDataSizeBytes);
  if (!result) return result.error();
  return true;
}

ccsds::ResultBool ccsds::Packet::deserialize(const std::vector<std::uint8_t> &headerData,
                                              const std::vector<std::uint8_t> &data) {
  const auto validated = validatePacketBytes(headerData, data,
                                             getPacketErrorControlMode(), m_CRC16Config);
  if (!validated) return validated.error();
  auto parsed = validated.value();
  RET_IF_ERR_MSG(parsed.header.getSecondaryHeaderFlag() != 0U,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Packet declares a secondary header; install a parser or use a typed/explicit-size overload.");

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

std::uint16_t ccsds::Packet::getFullPacketLength() {
  return static_cast<const Packet &>(*this).getFullPacketLength();
}

std::uint16_t ccsds::Packet::getFullPacketLength() const {
  return static_cast<std::uint16_t>(
    std::min<std::size_t>(getSerializedSize(), std::numeric_limits<std::uint16_t>::max()));
}

ccsds::ResultBool ccsds::Packet::setPrimaryHeader(const std::uint64_t data) {
  FORWARD_RESULT(m_primaryHeader.setData(data));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
  return true;
}

ccsds::ResultBool ccsds::Packet::setPrimaryHeader(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_primaryHeader.deserialize(data));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
  return true;
}

void ccsds::Packet::setPrimaryHeader(const Header &header) {
  m_primaryHeader = header;
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
}

ccsds::ResultBool ccsds::Packet::setPrimaryHeader(const PrimaryHeader data) {
  FORWARD_RESULT(m_primaryHeader.setData(data));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);
  m_updateStatus = false;
  return true;
}

ccsds::ResultBool ccsds::Packet::setSecondaryHeader(
    const std::shared_ptr<SecondaryHeaderAbstract> &header) {
  FORWARD_RESULT(m_dataField.setSecondaryHeader(header));
  FORWARD_RESULT(m_primaryHeader.setSecondaryHeaderFlag(header ? 1U : 0U));
  if (header && header->getDirection() != PacketDirection::Unspecified) {
    FORWARD_RESULT(m_primaryHeader.setType(packetTypeForDirection(header->getDirection())));
  }
  m_updateStatus = false;
  return true;
}

ccsds::ResultBool ccsds::Packet::setSecondaryHeader(
    const std::vector<std::uint8_t> &data, const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setSecondaryHeader(data, headerType));
  return setSecondaryHeader(m_dataField.getSecondaryHeader());
}

ccsds::ResultBool ccsds::Packet::setSecondaryHeader(const std::uint8_t *pData,
                                                     const std::size_t sizeData,
                                                     const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setSecondaryHeader(pData, sizeData, headerType));
  return setSecondaryHeader(m_dataField.getSecondaryHeader());
}

ccsds::ResultBool ccsds::Packet::setSecondaryHeader(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setSecondaryHeader(data));
  return setSecondaryHeader(m_dataField.getSecondaryHeader());
}

ccsds::ResultBool ccsds::Packet::setSecondaryHeader(const std::uint8_t *pData,
                                                     const std::size_t sizeData) {
  FORWARD_RESULT(m_dataField.setSecondaryHeader(pData, sizeData));
  return setSecondaryHeader(m_dataField.getSecondaryHeader());
}

ccsds::ResultBool ccsds::Packet::setApplicationData(
    const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setApplicationData(data));
  m_updateStatus = false;
  return true;
}

ccsds::ResultBool ccsds::Packet::setApplicationData(const std::uint8_t *pData,
                                                     const std::size_t sizeData) {
  FORWARD_RESULT(m_dataField.setApplicationData(pData, sizeData));
  m_updateStatus = false;
  return true;
}

void ccsds::Packet::setSequenceFlags(const ESequenceFlag flags) {
  if (!m_primaryHeader.setSequenceFlags(flags)) return;
  m_updateStatus = false;
}

ccsds::ResultBool ccsds::Packet::setSequenceCount(const std::uint16_t count) {
  RET_IF_ERR_MSG(count > SEQUENCE_COUNT_MASK, ErrorCode::INVALID_HEADER_DATA,
                 "Unable to set sequence count above 16383");
  FORWARD_RESULT(m_primaryHeader.setSequenceCount(count));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) | count;
  m_updateStatus = false;
  return true;
}

void ccsds::Packet::setDataFieldSize(const std::uint16_t size) {
  m_dataField.setDataPacketSize(size);
  m_updateStatus = false;
}

void ccsds::Packet::setUpdatePacketEnable(const bool enable) {
  m_enableUpdatePacket = enable;
  m_dataField.setSecondaryHeaderAutoUpdateStatus(enable);
}

void ccsds::Packet::setPacketErrorControlMode(const PacketErrorControlMode mode) {
  if (mode == PacketErrorControlMode::None) {
    m_sequenceCounter |= PACKET_ERROR_CONTROL_DISABLED_MASK;
    m_CRC16 = 0U;
  } else {
    m_sequenceCounter &= static_cast<std::uint16_t>(~PACKET_ERROR_CONTROL_DISABLED_MASK);
  }
  m_updateStatus = false;
}
