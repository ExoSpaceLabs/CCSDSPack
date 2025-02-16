
#include "CCSDSManager.h"

#include <utility>
#include "CCSDSUtils.h"
void CCSDS::Manager::setPacketTemplate(CCSDS::Packet packet){
  m_packetTemplate = std::move(packet);
}

void CCSDS::Manager::setApplicationData(const std::vector<uint8_t>& data) {
  if (!m_packets.empty()) {
    m_packets.clear();
  }
  const auto maxBytesPerPacket = m_packetTemplate.getDataFieldMaximumSize();
  const auto dataBytesSize = data.size();
  int i = 0;
  auto remainderBytes = static_cast<int>( dataBytesSize );
  auto sequenceFlag = UNSEGMENTED;
  uint16_t sequenceCount = 1;
  while (i < dataBytesSize) {
    Packet newPacket = m_packetTemplate;
    std::vector<uint8_t> tmp;
    if (remainderBytes > maxBytesPerPacket) {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + maxBytesPerPacket);
      remainderBytes -= maxBytesPerPacket;
      if (i == 0) {
        sequenceFlag = FIRST_SEGMENT;
      }else {
        sequenceFlag = CONTINUING_SEGMENT;
      }
      i += maxBytesPerPacket;
      newPacket.setApplicationData(tmp);
      newPacket.setSequenceCount(sequenceCount);
      newPacket.setSequenceFlags(sequenceFlag);
    } else {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + remainderBytes);
      i += remainderBytes;
      if ( sequenceFlag != UNSEGMENTED) {
        newPacket.setSequenceCount(sequenceCount);
        newPacket.setSequenceFlags(LAST_SEGMENT);
      }
      newPacket.setApplicationData(tmp);
    }
    m_packets.push_back(std::move(newPacket));
    sequenceCount++;
  }
}

void CCSDS::Manager::setAutoUpdateEnable(const bool enable) {
  m_updateEnable = enable;
  for (auto& packet : m_packets) {
    packet.setUpdatePacketEnable(enable);
  }
}

CCSDS::Result<std::vector<unsigned char>> CCSDS::Manager::getPacketTemplate() {
  auto data = m_packetTemplate.serialize();
  RETURN_IF_ERROR(data.empty(), ErrorCode::NO_DATA);
  return data;
}

CCSDS::Result<std::vector<uint8_t>> CCSDS::Manager::getPacketAtIndex(const uint16_t index) {
  RETURN_IF_ERROR(index < 0, ErrorCode::INVALID_DATA);
  RETURN_IF_ERROR(index >= m_packets.size(), ErrorCode::INVALID_DATA);
  return m_packets[index].serialize();
}

CCSDS::Result<std::vector<uint8_t>> CCSDS::Manager::getApplicationData() const {
  RETURN_IF_ERROR(m_packets.empty(), ErrorCode::NO_DATA);
  std::vector<uint8_t> data;
  for (auto packet : m_packets) {
    auto applicationData = packet.getApplicationData();
    data.insert(data.end(),    applicationData.begin(),    applicationData.end());
  }
  return data;
}

CCSDS::Result<std::vector<uint8_t>> CCSDS::Manager::getApplicationDataAtIndex(const uint16_t index) {
  RETURN_IF_ERROR(index < 0, ErrorCode::INVALID_DATA);
  RETURN_IF_ERROR(index >= m_packets.size(), ErrorCode::INVALID_DATA);
  return m_packets[index].getApplicationData();
}

CCSDS::Result<unsigned short> CCSDS::Manager::getTotalPackets() const {
  RETURN_IF_ERROR(m_packets.empty(), ErrorCode::NO_DATA);
  return m_packets.size();
}

CCSDS::Result<std::vector<CCSDS::Packet>> CCSDS::Manager::getPackets() {
  RETURN_IF_ERROR(m_packets.empty(), ErrorCode::NO_DATA);
  return m_packets;
}


