#include "CCSDSManager.h"
#include <utility>
#include "CCSDSUtils.h"

CCSDS::ResultBool CCSDS::Manager::setPacketTemplate(Packet packet) {
  RET_IF_ERR_MSG(m_templateIsSet, ErrorCode::SOMETHING_WENT_WRONG, "Cannot set Template as it is already set, please clear Manager first");
  m_templatePacket = std::move(packet);
  m_validator.setTemplatePacket(m_templatePacket);
  m_validateEnable = true;
  m_templateIsSet = true;
  return true;
}

void CCSDS::Manager::setDatFieldSize(const uint16_t size) {
  m_templatePacket.setDataFieldSize(size);
}

CCSDS::ResultBool CCSDS::Manager::setApplicationData(const std::vector<uint8_t> &data) {
  RET_IF_ERR_MSG(data.empty(), ErrorCode::NO_DATA, "Cannot set Application data, Provided data is empty");
  RET_IF_ERR_MSG(!m_templateIsSet, ErrorCode::INVALID_HEADER_DATA, "Cannot set Application data, No template has been set");

  const auto maxBytesPerPacket = m_templatePacket.getDataFieldMaximumSize();
  const auto dataBytesSize = data.size();

  if (!m_packets.empty()) {
    m_packets.clear();
  }

  int i = 0;
  auto remainderBytes = static_cast<int>(dataBytesSize);
  auto sequenceFlag = UNSEGMENTED;
  while (i < dataBytesSize) {
    Packet newPacket = m_templatePacket;
    std::vector<uint8_t> tmp;
    if (remainderBytes > maxBytesPerPacket) {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + maxBytesPerPacket);
      remainderBytes -= maxBytesPerPacket;
      if (i == 0) {
        sequenceFlag = FIRST_SEGMENT;
        m_sequenceCount++;
      } else {
        sequenceFlag = CONTINUING_SEGMENT;
      }
      i += maxBytesPerPacket;
      FORWARD_RESULT(newPacket.setApplicationData(tmp));
      newPacket.setSequenceFlags(sequenceFlag);
      FORWARD_RESULT(newPacket.setSequenceCount(m_sequenceCount));
    } else {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + remainderBytes);
      i += remainderBytes;
      if (sequenceFlag != UNSEGMENTED) {
        newPacket.setSequenceFlags(LAST_SEGMENT);
        FORWARD_RESULT(newPacket.setSequenceCount(m_sequenceCount));
      }
      FORWARD_RESULT(newPacket.setApplicationData(tmp));
    }
    newPacket.setUpdatePacketEnable(m_updateEnable);
    m_packets.push_back(std::move(newPacket));
    m_sequenceCount++;
  }
  return true;
}

void CCSDS::Manager::setAutoUpdateEnable(const bool enable) {
  m_updateEnable = enable;
  for (auto &packet: m_packets) {
    packet.setUpdatePacketEnable(enable);
  }
}

void CCSDS::Manager::setAutoValidateEnable(const bool enable) {
  m_validateEnable = enable;
}

CCSDS::ResultBuffer CCSDS::Manager::getPacketTemplate() {
  auto data = m_templatePacket.serialize();
  RET_IF_ERR_MSG(data.empty(), ErrorCode::NO_DATA, "Cannot get Packet template data, data is empty (impossible)"); // possibly redundant.
  return data;
}

CCSDS::ResultBuffer CCSDS::Manager::getPacketBufferAtIndex(const uint16_t index) {
  RET_IF_ERR_MSG(index < 0 || index >= m_packets.size(), ErrorCode::INVALID_DATA,
                 "Cannot get packet, index is out of bounds");
  if (m_validateEnable) {
    m_packets[index].update();
    const std::string errorMessage = "Validation failure for packet at index " + std::to_string(index);
    RET_IF_ERR_MSG(!m_validator.validate(m_packets[index]), ErrorCode::VALIDATION_FAILURE,
                   errorMessage);
  }
  return m_packets[index].serialize();
}

std::vector<uint8_t> CCSDS::Manager::getPacketsBuffer() const {
  std::vector<uint8_t> buffer;
  for (auto packet : m_packets) {
    std::vector<uint8_t> packetBuffer = packet.serialize();
    buffer.insert(buffer.end(), packetBuffer.begin(), packetBuffer.end());
  }
  return buffer;
}

CCSDS::ResultBuffer CCSDS::Manager::getApplicationDataBuffer() const {
  RET_IF_ERR_MSG(m_packets.empty(), ErrorCode::NO_DATA, "Cannot get Application data, no packets have been set.");
  std::vector<uint8_t> data;
  for (auto packet: m_packets) {
    auto applicationData = packet.getApplicationDataBytes();
    data.insert(data.end(), applicationData.begin(), applicationData.end());
  }
  return data;
}

CCSDS::ResultBuffer CCSDS::Manager::getApplicationDataBufferAtIndex(const uint16_t index) {
  RET_IF_ERR_MSG(index < 0 || index >= m_packets.size(), ErrorCode::INVALID_DATA,
                 "Cannot get Application data, index is out of bounds");
  return m_packets[index].getApplicationDataBytes();
}

uint16_t CCSDS::Manager::getTotalPackets() const {
  return m_packets.size();
}

std::vector<CCSDS::Packet> CCSDS::Manager::getPackets() {
  return m_packets;
}

CCSDS::ResultBool CCSDS::Manager::addPacket(Packet packet) {

  if (m_validateEnable && !m_updateEnable) {
    if (!m_templateIsSet) {
      m_validator.configure(true, true, false);
    }
    RET_IF_ERR_MSG(m_validator.validate(packet), ErrorCode::VALIDATION_FAILURE, "packet is not valid");
  }
  packet.setUpdatePacketEnable(m_updateEnable);
  m_packets.push_back(std::move(packet));
  return true;
}

CCSDS::ResultBool CCSDS::Manager::addPacketFromBuffer(const std::vector<uint8_t>& packetBuffer) {
  Packet packet;
  FORWARD_RESULT(packet.deserialize(packetBuffer));
  FORWARD_RESULT(addPacket(packet));
  return true;
}


[[nodiscard]] CCSDS::ResultBool CCSDS::Manager::load(const std::vector<Packet>& packets) {

  for (const auto& packet: packets) {
    FORWARD_RESULT(addPacket(packet));
  }
  return true;
}

[[nodiscard]] CCSDS::ResultBool CCSDS::Manager::load(const std::vector<uint8_t>& packetsBuffer) {
  RET_IF_ERR_MSG(packetsBuffer.size() < 8, ErrorCode::INVALID_DATA, "invalid packet buffer size");
  int offset{0};
  while (offset < packetsBuffer.size()) {
    std::vector<uint8_t> headerData;
    headerData.clear();
    copy_n(packetsBuffer.begin() + offset, 6, std::back_inserter(headerData));
    Header header;
    FORWARD_RESULT( header.deserialize(headerData));

    const uint16_t packetSize = header.getDataLength() + 8;
    std::vector<uint8_t>packetData;
    packetData.clear();
    copy_n(packetsBuffer.begin() + offset, packetSize, std::back_inserter(packetData));
    FORWARD_RESULT(addPacketFromBuffer(packetData));
    offset += packetSize;
  }
  return true;
}

void CCSDS::Manager::clear() {
  clearPackets();
  m_templateIsSet = false;
  m_templatePacket= {};
}

void CCSDS::Manager::clearPackets() {
  m_packets.clear();
  m_sequenceCount = 0;
  m_validator.clear();
}
