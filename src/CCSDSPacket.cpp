#include "CCSDSPacket.h"
#include "CCSDSDataField.h"
#include "CCSDSUtils.h"

void CCSDS::Packet::update() {
  if (!m_updateStatus && m_enableUpdatePacket) {
    const auto dataField = m_dataField.getFullDataFieldBytes();
    const auto dataFiledSize = static_cast<uint16_t>(dataField.size());
    const auto dataFieldHeaderFlag(m_dataField.getDataFieldHeaderFlag());
    m_primaryHeader.setDataLength(dataFiledSize);
    m_primaryHeader.setDataFieldHeaderFlag(dataFieldHeaderFlag);
    // todo this part needs to be moved out of conditional updating
    if (m_primaryHeader.getSequenceFlags() == UNSEGMENTED) {
      m_primaryHeader.setSequenceCount(0);
    } else {
      m_primaryHeader.setSequenceCount(m_sequenceCounter);
    }
    m_CRC16 = crc16(dataField, m_CRC16Config.polynomial, m_CRC16Config.initialValue, m_CRC16Config.finalXorValue);
    m_updateStatus = true;
  }
}

uint16_t CCSDS::Packet::getCRC() {
  update();
  return m_CRC16;
}

uint16_t CCSDS::Packet::getDataFieldMaximumSize() {
  return m_dataField.getDataFieldAvailableBytesSize();
}

bool CCSDS::Packet::getDataFieldHeaderFlag() {
  update();
  return m_primaryHeader.getDataFieldHeaderFlag();
}

std::vector<uint8_t> CCSDS::Packet::getCRCVectorBytes() {
  std::vector<uint8_t> crc(2);
  const auto crcVar = getCRC();
  crc[0] = (crcVar >> 8) & 0xFF; // MSB (Most Significant Byte)
  crc[1] = crcVar & 0xFF; // LSB (Least Significant Byte)
  return crc;
}

CCSDS::DataField &CCSDS::Packet::getDataField() {
  update();
  return m_dataField;
}

CCSDS::Header &CCSDS::Packet::getPrimaryHeader() {
  update();
  return m_primaryHeader;
}

uint64_t CCSDS::Packet::getPrimaryHeader64bit() {
  update();
  return m_primaryHeader.getFullHeader();
};

std::vector<uint8_t> CCSDS::Packet::getPrimaryHeaderBytes() {
  update();
  return m_primaryHeader.serialize();
}

std::vector<uint8_t> CCSDS::Packet::getDataFieldHeaderBytes() {
  update();
  return m_dataField.getDataFieldHeaderBytes();
}

std::vector<uint8_t> CCSDS::Packet::getApplicationDataBytes() {
  update();
  return m_dataField.getApplicationData();
}

std::vector<uint8_t> CCSDS::Packet::getFullDataFieldBytes() {
  return m_dataField.getFullDataFieldBytes();
}

std::vector<uint8_t> CCSDS::Packet::serialize() {
  auto header = getPrimaryHeaderBytes();
  auto dataField = m_dataField.getFullDataFieldBytes();
  const auto crc = getCRCVectorBytes();

  std::vector<uint8_t> packet;
  packet.reserve(header.size() + dataField.size() + crc.size());
  packet.insert(packet.end(), header.begin(), header.end());
  if (!getFullDataFieldBytes().empty()) {
    packet.insert(packet.end(), dataField.begin(), dataField.end());
  }
  packet.insert(packet.end(), crc.begin(), crc.end());

  return packet;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.size() <= 7, ErrorCode::INVALID_HEADER_DATA,
                 "Cannot Deserialize Packet, Invalid Data provided data size must be at least 8 bytes");

  std::vector<uint8_t> dataFieldVector;
  std::copy(data.begin() + 6, data.end(), std::back_inserter(dataFieldVector));

  FORWARD_RESULT(deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector));

  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<uint8_t> &data, const std::string &headerType) {
  RET_IF_ERR_MSG(data.size() <= 8, ErrorCode::INVALID_DATA,
                 "Cannot Deserialize Packet, Invalid Data provided data size must be at least 8 bytes");
  RET_IF_ERR_MSG(headerType == "BufferHeader", ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot Deserialize Packet, BufferHeader is not of defined size");
  RET_IF_ERR_MSG(!m_dataField.getDataFieldHeaderFactory().typeIsRegistered(headerType), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Cannot Deserialize Packet, Unregistered Secondary header: " + headerType);

  const auto secondaryHeader = m_dataField.getDataFieldHeaderFactory().create(headerType);
  const auto headerDataSizeBytes = secondaryHeader->getSize();

  std::vector<uint8_t> dataFieldHeaderVector;
  std::copy_n(data.begin() + 6, headerDataSizeBytes , std::back_inserter(dataFieldHeaderVector));
  FORWARD_RESULT( secondaryHeader->deserialize(dataFieldHeaderVector ));
  setDataFieldHeader(secondaryHeader);

  if (data.size() > (8 + headerDataSizeBytes)) {
    std::vector<uint8_t> dataFieldVector;

    if (data.size() > (9 + headerDataSizeBytes)) {
      std::copy(data.begin() + 6 + headerDataSizeBytes, data.end(), std::back_inserter(dataFieldVector));
    }
    FORWARD_RESULT(deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector));
  }

  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<uint8_t> &data, const uint16_t headerDataSizeBytes) {
  RET_IF_ERR_MSG(data.size() <= (8 + headerDataSizeBytes), ErrorCode::INVALID_DATA,
                 "Cannot Serialize Packet, Invalid Data provided");

  std::vector<uint8_t> secondaryHeader;
  std::vector<uint8_t> dataFieldVector;
  std::copy(data.begin() + 6, data.begin() + 6 + headerDataSizeBytes, std::back_inserter(secondaryHeader));
  FORWARD_RESULT(m_dataField.setDataFieldHeader(secondaryHeader));
  if (data.size() > (7 + headerDataSizeBytes)) {
    std::copy(data.begin() + 6 + headerDataSizeBytes, data.end(), std::back_inserter(dataFieldVector));
  }
  FORWARD_RESULT(deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector));
  return true;
}

CCSDS::ResultBool CCSDS::Packet::deserialize(const std::vector<uint8_t> &headerData, const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(headerData.size() != 6, ErrorCode::INVALID_HEADER_DATA,
                 "Cannot Deserialize Packet, Invalid Header Data provided.");
  FORWARD_RESULT(m_primaryHeader.deserialize( headerData ));
  m_sequenceCounter = m_primaryHeader.getSequenceCount();

  RET_IF_ERR_MSG(data.size() < 2, ErrorCode::INVALID_DATA,
                 "Cannot Deserialize Packet, Invalid Data provided, at least CRC is required.");

  std::vector<uint8_t> dataCopy;
  m_CRC16 = (data[data.size() - 2] << 8) + data.back();

  if (data.size() == 2) return true; // returns since no application data is to be written.

  std::copy(data.begin(), data.end() - 2, std::back_inserter(dataCopy));
  FORWARD_RESULT(m_dataField.setApplicationData(dataCopy));

  return true;;
}

uint16_t CCSDS::Packet::getFullPacketLength() {
  // where 8 is derived from 6 bytes for Primary header and 2 bytes for CRC16.
  return 8 + m_dataField.getDataFieldUsedBytesSize();
}

CCSDS::ResultBool CCSDS::Packet::setPrimaryHeader(const uint64_t data) {
  FORWARD_RESULT(m_primaryHeader.setData( data ));
  m_sequenceCounter = m_primaryHeader.getSequenceCount();
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setPrimaryHeader(const std::vector<uint8_t> &data) {
  FORWARD_RESULT(m_primaryHeader.deserialize( data ));
  m_sequenceCounter = m_primaryHeader.getSequenceCount();
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setPrimaryHeader(const Header &header) {
  m_primaryHeader = header;
}

void CCSDS::Packet::setPrimaryHeader(const PrimaryHeader data) {
  m_primaryHeader.setData(data);
  m_sequenceCounter = m_primaryHeader.getSequenceCount();
  m_updateStatus = false;
}

void CCSDS::Packet::setDataFieldHeader(const std::shared_ptr<SecondaryHeaderAbstract> &header) {
  m_dataField.setDataFieldHeader(header);
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::vector<uint8_t> &data, const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader( data, headerType ));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const uint8_t *pData, const size_t sizeData,
                                                    const std::string &headerType) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader( pData, sizeData, headerType ));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const std::vector<uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader( data ));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setDataFieldHeader(const uint8_t *pData, const size_t sizeData) {
  FORWARD_RESULT(m_dataField.setDataFieldHeader( pData,sizeData ));
  m_updateStatus = false;
  return true;;
}

CCSDS::ResultBool CCSDS::Packet::setApplicationData(const std::vector<uint8_t> &data) {
  FORWARD_RESULT(m_dataField.setApplicationData( data ));
  m_updateStatus = false;
  return true;
}

CCSDS::ResultBool CCSDS::Packet::setApplicationData(const uint8_t *pData, const size_t sizeData) {
  FORWARD_RESULT(m_dataField.setApplicationData( pData,sizeData ));
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setSequenceFlags(const ESequenceFlag flags) {
  m_primaryHeader.setSequenceFlags(flags);
  m_updateStatus = false;
}

CCSDS::ResultBool CCSDS::Packet::setSequenceCount(const uint16_t count) {
  RET_IF_ERR_MSG(m_primaryHeader.getSequenceFlags() == UNSEGMENTED && count != 0, ErrorCode::INVALID_DATA,
                 "Unable to set non 0 value for UNSEGMENTED packet");
  m_sequenceCounter = count;
  m_updateStatus = false;
  return true;
}

void CCSDS::Packet::setDataFieldSize(const uint16_t size) {
  m_dataField.setDataPacketSize(size);
}

void CCSDS::Packet::setUpdatePacketEnable(const bool enable) {
  m_enableUpdatePacket = enable;
  m_dataField.setDataFieldHeaderAutoUpdateStatus(enable);
}
