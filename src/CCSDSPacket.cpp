// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSPacket.h"
#include "CCSDSDataField.h"
#include "CCSDSUtils.h"
#include <algorithm>
#include <limits>

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

void CCSDS::Packet::update() {
  if (!m_updateStatus && m_enableUpdatePacket) {
    if (m_primaryHeader.getHeaderStatus() == INVALID) {
      return;
    }

    const auto dataField = m_dataField.serialize();
    const auto packetDataFieldSize = dataField.size() + getPacketErrorControlSize();
    constexpr auto maximumPacketDataFieldSize =
      static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;

    if (packetDataFieldSize == 0U || packetDataFieldSize > maximumPacketDataFieldSize) {
      return;
    }

    const auto dataFieldHeaderFlag = m_dataField.getDataFieldHeaderFlag();
    m_primaryHeader.setDataLength(static_cast<std::uint16_t>(packetDataFieldSize - 1U));
    if (!m_primaryHeader.setDataFieldHeaderFlag(dataFieldHeaderFlag)) {
      return;
    }

    if (m_primaryHeader.getSequenceFlags() == UNSEGMENTED) {
      if (!m_primaryHeader.setSequenceCount(0)) {
        return;
      }
    } else if (!m_primaryHeader.setSequenceCount(m_sequenceCounter & SEQUENCE_COUNT_MASK)) {
      return;
    }

    if (getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
      auto crcInput = m_primaryHeader.serialize();
      if (crcInput.size() != 6U) {
        return;
      }
      crcInput.insert(crcInput.end(), dataField.begin(), dataField.end());
      m_CRC16 = crc16(crcInput, m_CRC16Config.polynomial, m_CRC16Config.initialValue,
                      m_CRC16Config.finalXorValue);
    } else {
      m_CRC16 = 0;
    }
    m_updateStatus = true;
  }
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
    sequenceCount = 1;
    sequenceFlag = FIRST_SEGMENT;
  } else {
    sequenceCount = 0;
    sequenceFlag = UNSEGMENTED;
  }
  FORWARD_RESULT(m_primaryHeader.setSequenceFlags(sequenceFlag));
  FORWARD_RESULT(m_primaryHeader.setSequenceCount(sequenceCount));
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
    if (secondaryHeaderFlag) {
      FORWARD_RESULT(m_dataField.setDataFieldHeader(cfg));
    }
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
  if (getPacketErrorControlMode() == PacketErrorControlMode::None
      || m_primaryHeader.getHeaderStatus() == INVALID) {
    return 0;
  }
  update();
  return m_CRC16;
}

std::uint16_t CCSDS::Packet::getDataFieldMaximumSize() const {
  return m_dataField.getDataFieldAvailableBytesSize();
}

bool CCSDS::Packet::getDataFieldHeaderFlag() {
  update();
  return m_primaryHeader.getDataFieldHeaderFlag();
}

std::vector<std::uint8_t> CCSDS::Packet::getCRCVectorBytes() {
  if (getPacketErrorControlMode() == PacketErrorControlMode::None
      || m_primaryHeader.getHeaderStatus() == INVALID) {
    return {};
  }

  const auto crcVar = getCRC();
  return {
    static_cast<std::uint8_t>((crcVar >> 8) & 0xFFU),
    static_cast<std::uint8_t>(crcVar & 0xFFU)
  };
}

CCSDS::DataField &CCSDS::Packet::getDataField() {
  update();
  return m_dataField;
}

CCSDS::Header &CCSDS::Packet::getPrimaryHeader() {
  update();
  return m_primaryHeader;
}

std::uint64_t CCSDS::Packet::getPrimaryHeader64bit() {
  update();
  return m_primaryHeader.getFullHeader();
}

std::vector<std::uint8_t> CCSDS::Packet::getPrimaryHeaderBytes() {
  update();
  return m_primaryHeader.serialize();
}

std::vector<std::uint8_t> CCSDS::Packet::getDataFieldHeaderBytes() {
  update();
  return m_dataField.getDataFieldHeaderBytes();
}

std::vector<std::uint8_t> CCSDS::Packet::getApplicationDataBytes() {
  update();
  return m_dataField.getApplicationData();
}

std::vector<std::uint8_t> CCSDS::Packet::getFullDataFieldBytes() {
  return m_dataField.serialize();
}

std::vector<std::uint8_t> CCSDS::Packet::serialize() {
  if (m_primaryHeader.getHeaderStatus() == INVALID) {
    return {};
  }

  const auto dataField = m_dataField.serialize();
  const auto packetDataFieldSize = dataField.size() + getPacketErrorControlSize();
  constexpr auto maximumPacketDataFieldSize =
    static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;

  if (packetDataFieldSize == 0U || packetDataFieldSize > maximumPacketDataFieldSize) {
    return {};
  }

  update();
  const auto header = m_primaryHeader.serialize();
  if (header.size() != 6U) {
    return {};
  }

  std::vector<std::uint8_t> packet;
  packet.reserve(header.size() + dataField.size() + getPacketErrorControlSize());
  packet.insert(packet.end(), header.begin(), header.end());
  packet.insert(packet.end(), dataField.begin(), dataField.end());

  if (getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
    packet.push_back(static_cast<std::uint8_t>((m_CRC16 >> 8) & 0xFFU));
    packet.push_back(static_cast<std::uint8_t>(m_CRC16 & 0xFFU));
  }

  return packet;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &data) {
  const auto minimumPacketSize = static_cast<std::size_t>(6U + getPacketErrorControlSize());
  RET_IF_ERR_MSG(data.size() < minimumPacketSize, ErrorCode::INVALID_HEADER_DATA,
                 "Cannot Deserialize Packet, data is shorter than the primary header and configured packet error control");

  std::vector<std::uint8_t> dataFieldVector;
  std::copy(data.begin() + 6, data.end(), std::back_inserter(dataFieldVector));

  FORWARD_RESULT(deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector));
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &data,
                                              const std::string &headerType,
                                              const std::int32_t headerSize) {
  const auto minimumPacketSize = static_cast<std::size_t>(6U + getPacketErrorControlSize());
  RET_IF_ERR_MSG(data.size() < minimumPacketSize, ErrorCode::INVALID_DATA,
                 "Cannot Deserialize Packet, invalid data provided");
  RET_IF_ERR_MSG(headerType == "BufferHeader", ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot Deserialize Packet, BufferHeader is not of defined size");
  RET_IF_ERR_MSG(!m_dataField.getDataFieldHeaderFactory().typeIsRegistered(headerType),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot Deserialize Packet, Unregistered Secondary header: " + headerType);

  std::uint16_t headerDataSizeBytes{0};
  const auto secondaryHeader = m_dataField.getDataFieldHeaderFactory().create(headerType);
  if (headerType == "PusC" && headerSize > 0) {
    headerDataSizeBytes = static_cast<std::uint16_t>(headerSize);
  } else {
    headerDataSizeBytes = secondaryHeader->getSize();
  }

  RET_IF_ERR_MSG(data.size() < 6U + headerDataSizeBytes + getPacketErrorControlSize(),
                 ErrorCode::INVALID_DATA,
                 "Cannot Deserialize Packet, data is shorter than the configured secondary header and packet error control");

  std::vector<std::uint8_t> dataFieldHeaderVector;
  std::copy_n(data.begin() + 6, headerDataSizeBytes, std::back_inserter(dataFieldHeaderVector));
  FORWARD_RESULT(secondaryHeader->deserialize(dataFieldHeaderVector));
  setDataFieldHeader(secondaryHeader);

  std::vector<std::uint8_t> dataFieldVector;
  if (data.size() > 6U + headerDataSizeBytes) {
    std::copy(data.begin() + 6 + headerDataSizeBytes, data.end(), std::back_inserter(dataFieldVector));
  }
  FORWARD_RESULT(deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector));
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &data,
                                              const std::uint16_t headerDataSizeBytes) {
  RET_IF_ERR_MSG(data.size() < 6U + headerDataSizeBytes + getPacketErrorControlSize(),
                 ErrorCode::INVALID_DATA, "Cannot Deserialize Packet, Invalid Data provided");

  std::vector<std::uint8_t> secondaryHeader;
  std::vector<std::uint8_t> dataFieldVector;
  std::copy(data.begin() + 6, data.begin() + 6 + headerDataSizeBytes,
            std::back_inserter(secondaryHeader));
  FORWARD_RESULT(m_dataField.setDataFieldHeader(secondaryHeader));
  if (data.size() > 6U + headerDataSizeBytes) {
    std::copy(data.begin() + 6 + headerDataSizeBytes, data.end(), std::back_inserter(dataFieldVector));
  }
  FORWARD_RESULT(deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector));
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<std::uint8_t> &headerData,
                                              const std::vector<std::uint8_t> &data) {
  RET_IF_ERR_MSG(headerData.size() != 6U, ErrorCode::INVALID_HEADER_DATA,
                 "Cannot Deserialize Packet, Invalid Header Data provided.");
  FORWARD_RESULT(m_primaryHeader.deserialize(headerData));
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK)
                      | (m_primaryHeader.getSequenceCount() & SEQUENCE_COUNT_MASK);

  std::vector<std::uint8_t> dataCopy;
  if (getPacketErrorControlMode() == PacketErrorControlMode::CRC16) {
    RET_IF_ERR_MSG(data.size() < 2U, ErrorCode::INVALID_DATA,
                   "Cannot Deserialize Packet, at least two packet error-control bytes are required in CRC16 mode.");
    m_CRC16 = (static_cast<std::uint16_t>(data[data.size() - 2]) << 8) + data.back();
    if (data.size() > 2U) {
      std::copy(data.begin(), data.end() - 2, std::back_inserter(dataCopy));
    }
  } else {
    m_CRC16 = 0;
    dataCopy = data;
  }

  if (!dataCopy.empty()) {
    FORWARD_RESULT(m_dataField.setApplicationData(dataCopy));
  }

  m_updateStatus = false;
  return true;
}

std::uint16_t CCSDS::Packet::getFullPacketLength() {
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
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::vector<std::uint8_t> &data,
                                                     const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(data, headerType));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::uint8_t *pData,
                                                     const std::size_t sizeData,
                                                     const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(pData, sizeData, headerType));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::vector<std::uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(data));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::uint8_t *pData,
                                                     const std::size_t sizeData) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader(pData, sizeData));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setApplicationData(const std::vector<std::uint8_t> &data) {
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
  if (!m_primaryHeader.setSequenceFlags(flags)) {
    return;
  }
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setSequenceCount(const std::uint16_t count) {
  RET_IF_ERR_MSG(m_primaryHeader.getSequenceFlags() == UNSEGMENTED && count != 0,
                 ErrorCode::INVALID_DATA, "Unable to set non 0 value for UNSEGMENTED packet");
  RET_IF_ERR_MSG(count > SEQUENCE_COUNT_MASK, ErrorCode::INVALID_HEADER_DATA,
                 "Unable to set sequence count above 16383");
  m_sequenceCounter = (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) | count;
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setDataFieldSize(const std::uint16_t size) {
  m_dataField.setDataPacketSize(size);
}

void CCSDS::Packet::setUpdatePacketEnable(const bool enable) {
  m_enableUpdatePacket = enable;
  m_dataField.setDataFieldHeaderAutoUpdateStatus(enable);
}

void CCSDS::Packet::setPacketErrorControlMode(const PacketErrorControlMode mode) {
  if (mode == PacketErrorControlMode::None) {
    m_sequenceCounter |= PACKET_ERROR_CONTROL_DISABLED_MASK;
    m_CRC16 = 0;
  } else {
    m_sequenceCounter &= static_cast<std::uint16_t>(~PACKET_ERROR_CONTROL_DISABLED_MASK);
  }
  m_updateStatus = false;
}
